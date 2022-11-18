#include <cassert>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>

#include "pack_unpack.h"
#include "ss_router.h"
#include "typedefs.h"
#include "network.h"
#include "logging.h"
#include <poll.h>

static constexpr int MAX_RX_PACKET_SIZE = 10 * MB_SIZE;

struct net_rx_buffer
{
    binary_fixed_buffer_archive<MAX_RX_PACKET_SIZE> read_buf{PACK_DIR_IN};
    sizet available;
};

intern net_rx_buffer rx_buf;

// #define SCAN_DEBUG
// #define OCC_DEBUG
// #define TFROM_DEBUG
// #define PACKET_DEBUG

#if defined(TFROM_DEBUG)
#define tform_dlog dlog
#define TFORM_PACK_LOG_ENABLE log_set_level(urho::LOG_TRACE);
#define TFORM_PACK_LOG_DISABLE log_set_level(urho::LOG_DEBUG);
#else
#define tform_dlog(...)
#define TFORM_PACK_LOG_ENABLE
#define TFORM_PACK_LOG_DISABLE
#endif

#if defined(OCC_DEBUG)
#define occ_dlog dlog
#define OCC_PACK_LOG_ENABLE log_set_level(urho::LOG_TRACE);
#define OCC_PACK_LOG_DISABLE log_set_level(urho::LOG_DEBUG);
#else
#define occ_dlog(...)
#define OCC_PACK_LOG_ENABLE
#define OCC_PACK_LOG_DISABLE
#endif

#if defined(SCAN_DEBUG)
#define scan_dlog dlog
#define SCAN_PACK_LOG_ENABLE log_set_level(urho::LOG_TRACE);
#define SCAN_PACK_LOG_DISABLE log_set_level(urho::LOG_DEBUG);
#else
#define scan_dlog(...)
#define SCAN_PACK_LOG_ENABLE
#define SCAN_PACK_LOG_DISABLE
#endif

#if defined(PACKET_DEBUG)
#define packet_dlog dlog
#else
#define packet_dlog(...)
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten/websocket.h>
inline const char *SERVER_URL = "ws://192.168.1.103:8080";

/*
#define EMSCRIPTEN_RESULT_NOT_SUPPORTED       -1
#define EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED -2
#define EMSCRIPTEN_RESULT_INVALID_TARGET      -3
#define EMSCRIPTEN_RESULT_UNKNOWN_TARGET      -4
#define EMSCRIPTEN_RESULT_INVALID_PARAM       -5
#define EMSCRIPTEN_RESULT_FAILED              -6
#define EMSCRIPTEN_RESULT_NO_DATA             -7
#define EMSCRIPTEN_RESULT_TIMED_OUT           -8
*/

intern const char *em_result_err_str(EMSCRIPTEN_RESULT code)
{
    static constexpr int BUF_SIZE = 40;
    static char buff[BUF_SIZE] = {};
    switch (code)
    {
    case (EMSCRIPTEN_RESULT_NOT_SUPPORTED):
        snprintf(buff, BUF_SIZE, "Not supported");
        break;
    case (EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED):
        snprintf(buff, BUF_SIZE, "Failed not deffered");
        break;
    case (EMSCRIPTEN_RESULT_INVALID_TARGET):
        snprintf(buff, BUF_SIZE, "Invalid target");
        break;
    case (EMSCRIPTEN_RESULT_UNKNOWN_TARGET):
        snprintf(buff, BUF_SIZE, "Unknown target");
        break;
    case (EMSCRIPTEN_RESULT_INVALID_PARAM):
        snprintf(buff, BUF_SIZE, "Invalid param");
        break;
    case (EMSCRIPTEN_RESULT_FAILED):
        snprintf(buff, BUF_SIZE, "Failed");
        break;
    case (EMSCRIPTEN_RESULT_NO_DATA):
        snprintf(buff, BUF_SIZE, "No data");
        break;
    case (EMSCRIPTEN_RESULT_TIMED_OUT):
        snprintf(buff, BUF_SIZE, "Timed out");
        break;
    default:
        snprintf(buff, BUF_SIZE, "Unknown");
        break;
    }
    return buff;
}

intern EM_BOOL em_ws_on_error(int event_type, const EmscriptenWebSocketErrorEvent *ws_event, void *user_data)
{
    auto conn = (net_connection *)user_data;
    wlog("Got socket err on %d", ws_event->socket);
    return true;
}

intern EM_BOOL em_ws_on_message(int event_type, const EmscriptenWebSocketMessageEvent *ws_event, void *user_data)
{
    auto conn = (net_connection *)user_data;
    assert((MAX_RX_PACKET_SIZE - rx_buf.read_buf.cur_offset - rx_buf.available) > ws_event->numBytes);

    memcpy(rx_buf.read_buf.data + rx_buf.read_buf.cur_offset + rx_buf.available, ws_event->data, ws_event->numBytes);
    rx_buf.available += ws_event->numBytes;
    if (ws_event->numBytes > 0)
    {
        packet_dlog("Added %d bytes to available - result:%d", ws_event->numBytes, rx_buf.available);
    }
    else
    {
        elog("Got ws message with %d bytes....", ws_event->numBytes);
    }
    return true;
}

intern EM_BOOL em_ws_on_open(int event_type, const EmscriptenWebSocketOpenEvent *ws_event, void *user_data)
{
    auto conn = (net_connection *)user_data;
    wlog("WebSocket opened... shouldnt ever");
    return true;
}

intern EM_BOOL em_ws_on_close(int event_type, const EmscriptenWebSocketCloseEvent *ws_event, void *user_data)
{
    auto conn = (net_connection *)user_data;
    wlog("WebSocket closed - cleanly:%d code:%d reason:%s", ws_event->wasClean, ws_event->code, ws_event->reason);
    emscripten_websocket_delete(conn->socket_handle);
    conn->socket_handle = 0;
    return true;
}

intern void em_net_write(const net_connection &conn, const u8 *data, sizet data_size)
{
    int res = emscripten_websocket_send_binary(conn.socket_handle, (void *)data, data_size);
    if (res < 0)
    {
        elog("Could not write %d bytes to websocket handle %d: %s", data_size, conn.socket_handle, em_result_err_str(res));
    }
}

intern void em_net_connect(net_connection *conn)
{
    if (!emscripten_websocket_is_supported())
    {
        elog("Websockets not supported in this browser");
        return;
    }

    EmscriptenWebSocketCreateAttributes attribs{};
    attribs.url = SERVER_URL;
    attribs.createOnMainThread = true;
    conn->socket_handle = emscripten_websocket_new(&attribs);
    if (conn->socket_handle <= 0)
    {
        elog("Could not create web socket: %s", em_result_err_str(conn->socket_handle));
        conn->socket_handle = -1;
        return;
    }

    ilog("Successfully created web socket with handle %d to url %s", conn->socket_handle, SERVER_URL);

    emscripten_websocket_set_onclose_callback(conn->socket_handle, conn, em_ws_on_close);
    emscripten_websocket_set_onerror_callback(conn->socket_handle, conn, em_ws_on_error);
    emscripten_websocket_set_onopen_callback(conn->socket_handle, conn, em_ws_on_open);
    emscripten_websocket_set_onmessage_callback(conn->socket_handle, conn, em_ws_on_message);
}
#endif

void net_connect(net_connection *conn, int max_timeout_ms)
{
#if defined(__EMSCRIPTEN__)
    em_net_connect(conn);
#else
    struct pollfd sckt_fd[1];
    int pret;
    sckt_fd[0].fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sckt_fd[0].fd == -1)
    {
        elog("Failed to create socket");
        return;
    }
    else
    {
        ilog("Created socket with fd %d", sckt_fd[0].fd);
    }

    fcntl(sckt_fd[0].fd, F_SETFL, O_NONBLOCK);
    sckt_fd[0].events = POLLOUT;

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (!inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr))
    {
        elog("Failed PTON");
        goto cleanup;
    }

    ilog("Connecting to server at %s on port %d", SERVER_IP, SERVER_PORT);
    if (connect(sckt_fd[0].fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0 && errno != EINPROGRESS)
    {
        elog("Failed connecting to server at %s on port %d - resulting fd %d - error %s", SERVER_IP, SERVER_PORT, sckt_fd[0].fd, strerror(errno));
        goto cleanup;
    }

    pret = poll(sckt_fd, 1, max_timeout_ms);
    if (pret == 0)
    {
        elog("Poll timed out connecting to server %s on port %d for fd %d", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
        goto cleanup;
    }
    else if (pret == -1)
    {
        elog("Failed poll on trying to connect to server at %s on port %d - resulting fd %d - error %s", SERVER_IP, SERVER_PORT, sckt_fd[0].fd, strerror(errno));
        goto cleanup;
    }
    else
    {
        if (test_flags(sckt_fd[0].revents, POLLERR))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLERR", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
            goto cleanup;
        }
        else if (test_flags(sckt_fd[0].revents, POLLHUP))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLHUP", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
            goto cleanup;
        }
        else if (test_flags(sckt_fd[0].revents, POLLNVAL))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLNVAL", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
            goto cleanup;
        }
    }
    conn->socket_handle = sckt_fd[0].fd;
    ilog("Successfully connected to server at %s on port %d - resulting fd %d", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
    return;

cleanup:
    close(sckt_fd[0].fd);
    conn->socket_handle = -1;
#endif
}

intern void handle_scan_packet(binary_fixed_buffer_archive<MAX_RX_PACKET_SIZE> &read_buf, sizet available, sizet cached_offset, net_connection *conn)
{
    static sicklms_laser_scan scan{};

    pack_unpack(read_buf, scan.header, {"header"});
    pack_unpack(read_buf, scan.meta, {"meta"});

    sizet meta_and_header_size = read_buf.cur_offset - cached_offset;
    sizet range_count = (sizet)((scan.meta.angle_max - scan.meta.angle_min) / scan.meta.angle_increment) + 1;
    sizet total_packet_size = range_count * sizeof(float) + meta_and_header_size;

    if (available >= total_packet_size)
    {
        pack_unpack(read_buf, scan.ranges, {"change_elems", {pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &range_count}});

#if defined(SCAN_DEBUG)
        scan_dlog("Got scan packet - %d available bytes and %d packet size (%d ranges) - new offset is %d",
                  available,
                  total_packet_size,
                  range_count,
                  read_buf.cur_offset);
        SCAN_PACK_LOG_ENABLE
        static u8 buf[100];
        binary_buffer_archive ba{buf, PACK_DIR_OUT};
        pack_unpack(ba, scan.meta, {"meta"});
        SCAN_PACK_LOG_DISABLE
#endif

        conn->scan_received(0, scan);
    }
    else
    {
        // Not all bytes have come in for packet - set back the cur_offset to what it was before reading the meta data
        read_buf.cur_offset = cached_offset;
    }
}

intern void handle_occ_grid_packet(binary_fixed_buffer_archive<MAX_RX_PACKET_SIZE> &read_buf, sizet available, sizet cached_offset, net_connection *conn)
{
    static occupancy_grid_update gu{};

    pack_unpack(read_buf, gu.header, {"header"});
    pack_unpack(read_buf, gu.meta, {"meta"});

    sizet meta_and_header_size = read_buf.cur_offset - cached_offset;
    sizet total_packet_size = gu.meta.change_elem_count * sizeof(u32) + meta_and_header_size;

    if (available >= total_packet_size)
    {
        pack_unpack(read_buf, gu.change_elems, {"change_elems", {pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &gu.meta.change_elem_count}});

#if defined(OCC_DEBUG)
        occ_dlog("Got occ grid packet - %d available bytes and %d calculated packet size (%d header/meta), new offset is %d",
                 available,
                 total_packet_size,
                 meta_and_header_size,
                 read_buf.cur_offset);
        OCC_PACK_LOG_ENABLE
        static u8 buf[100];
        binary_buffer_archive ba{buf, PACK_DIR_OUT};
        pack_unpack(ba, gu.meta, {"meta"});
        OCC_PACK_LOG_DISABLE
#endif

        conn->occ_grid_update_received(0, gu);
    }
    else
    {
        // Not all bytes have come in for packet - set back the cur_offset to what it was before reading the meta data
        read_buf.cur_offset = cached_offset;
    }
}

intern void handle_tform_packet(binary_fixed_buffer_archive<MAX_RX_PACKET_SIZE> &read_buf, sizet available, sizet cached_offset, net_connection *conn)
{
    TFORM_PACK_LOG_ENABLE
    static node_transform tform{};
    pack_unpack(read_buf, tform, {});
#if defined(TFROM_DEBUG)
    tform_dlog("Got tform packet - %d available bytes and %d packet size - new offset is %d", available, read_buf.cur_offset - cached_offset, read_buf.cur_offset);
#endif
    conn->transform_updated(0, tform);
    TFORM_PACK_LOG_DISABLE
}

intern bool matches_packet_id(const char *packet_id, const u8 *data)
{
    int result = strncmp((const char *)data, packet_id, packet_header::size);
    return (result == 0);
}

// Add different packet types to these two functions using helper macros
intern sizet matching_packet_size(u8 *data)
{
    static sizet scan_size = packed_sizeof<sicklms_laser_scan_meta>();
    static sizet occ_meta = packed_sizeof<occ_grid_meta>();
    static sizet node_tform = packed_sizeof<node_transform>();

    if (matches_packet_id(SCAN_PACKET_ID, data))
    {
        return scan_size;
    }
    else if (matches_packet_id(OC_GRID_PCKT_ID, data))
    {
        return occ_meta;
    }
    else if (matches_packet_id(TFORM_PCKT_ID, data))
    {
        return node_tform;
    }
    return 0;
}

intern sizet dispatch_received_packet(binary_fixed_buffer_archive<MAX_RX_PACKET_SIZE> &read_buf, sizet available, net_connection *conn)
{
    sizet cached_offset = read_buf.cur_offset;
    if (matches_packet_id(SCAN_PACKET_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_scan_packet(read_buf, available, cached_offset, conn);
    }
    else if (matches_packet_id(OC_GRID_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_occ_grid_packet(read_buf, available, cached_offset, conn);
    }
    else if (matches_packet_id(TFORM_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_tform_packet(read_buf, available, cached_offset, conn);
    }
    return read_buf.cur_offset - cached_offset;
}

intern bool net_socket_read(net_connection *conn)
{
    int rd_cnt = read(conn->socket_handle,
                      rx_buf.read_buf.data + rx_buf.read_buf.cur_offset + rx_buf.available,
                      MAX_RX_PACKET_SIZE - rx_buf.read_buf.cur_offset - rx_buf.available);
    if (rd_cnt > 0)
    {
        rx_buf.available += rd_cnt;
        packet_dlog("Added %d bytes to available - result:%d", rd_cnt, rx_buf.available);
    }
    else if (rd_cnt < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        elog("Got read error %s", strerror(errno));
        return false;
    }
    return true;
}

void net_rx(net_connection *conn)
{
    static sizet current_packet_size{0};

    if (conn->socket_handle <= 0)
        return;

    assert(MAX_RX_PACKET_SIZE - rx_buf.available > 0 && "Read buffer size is too small - can't receive complete packet");

#if !defined(__EMSCRIPTEN__)
    if (!net_socket_read(conn))
        return;
#endif

    bool need_more_data = false;
    while (rx_buf.available >= packet_header::size && !need_more_data)
    {
        if (current_packet_size == 0)
        {
            current_packet_size = matching_packet_size(rx_buf.read_buf.data + rx_buf.read_buf.cur_offset);
            if (current_packet_size == 0)
            {
                --rx_buf.available;
                ++rx_buf.read_buf.cur_offset;
            }
            else
            {
                packet_dlog("Found packet header for packet size %d (rx_buf.available:%d  Readbuf offset:%d)",
                            current_packet_size,
                            rx_buf.available,
                            rx_buf.read_buf.cur_offset);
            }
        }
        else if (rx_buf.available >= current_packet_size)
        {
            sizet bytes_processed = dispatch_received_packet(rx_buf.read_buf, rx_buf.available, conn);
            if (bytes_processed > 0)
            {
                rx_buf.available -= bytes_processed;
                packet_dlog(
                    "Read entire packet of %d bytes - ended at offset %d (packet size before was %d) - there are %d remaining rx_buf.available bytes to be read",
                    bytes_processed,
                    rx_buf.read_buf.cur_offset,
                    current_packet_size,
                    rx_buf.available);

                // Copy any remaining rx_buf.available bytes to the beginning of the buffer - otherwise if rx_buf.available is greater than the packet header size
                // not only will we lose those bytes, but we will also mistakenly reconsider the first "rx_buf.available" count of bytes as if they just came
                // in - ie we would see whatever we just processed packet header again and wait until all the bytes came through... This gets ugly
                // if we have info in the packet to determine how many bytes we should later expect

                // memcpy(rx_buf.read_buf.data, rx_buf.read_buf.data + rx_buf.read_buf.cur_offset, rx_buf.available);
                // ilog("Copying %d available", rx_buf.available);

                current_packet_size = 0;
                // rx_buf.read_buf.cur_offset = 0;
            }
            else
            {
                packet_dlog("Waiting on more data for packet header size %d - cur available %d and cur offset %d",
                            current_packet_size,
                            rx_buf.available,
                            rx_buf.read_buf.cur_offset);
                need_more_data = true;
            }
        }
        else
        {
            packet_dlog("Waiting on more data for packet header size %d - cur available %d and cur offset %d",
                        current_packet_size,
                        rx_buf.available,
                        rx_buf.read_buf.cur_offset);
            need_more_data = true;
        }
    }

    if (rx_buf.available == 0)
    {
        rx_buf.read_buf.cur_offset = 0;
        packet_dlog("Read all available data on channel - moving pointer to buffer start");
    }
}

void net_tx(const net_connection &conn, const u8 *data, sizet data_size)
{
    if (conn.socket_handle <= 0)
        return;

#if defined(__EMSCRIPTEN__)
    em_net_write(conn, data, data_size);
#else
    sizet bwritten = write(conn.socket_handle, data, data_size);
    if (bwritten == -1)
    {
        elog("Got write error %d", strerror(errno));
    }
    else if (data_size != bwritten)
    {
        wlog("Bytes written was only %d when tried to write %d", bwritten, data_size);
    }
#endif
}

void net_disconnect(net_connection *conn)
{
#if defined(__EMSCRIPTEN__)
    emscripten_websocket_close(conn->socket_handle, 0, "Disconnected");
    emscripten_websocket_delete(conn->socket_handle);
#else
    if (conn->socket_handle > 0)
        close(conn->socket_handle);
#endif
    conn->socket_handle = 0;
}