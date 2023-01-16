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

// #define PACKET_DEBUG

#if defined(PACKET_DEBUG)
#define packet_dlog dlog
#else
#define packet_dlog(...)
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten/websocket.h>

EM_JS(char*, get_browser_url, (), {
    let host = window.location.host;
    let length = lengthBytesUTF8(host) + 1;
    let str = _malloc(length);
    stringToUTF8(host, str, length);
    console.log(`Should be returning  ${length} bytes for ${host}`);
    return str;
});

EM_JS(char*, get_browser_url_path, (), {
    let path = window.location.pathname;
    let length = lengthBytesUTF8(path) + 1;
    let str = _malloc(length);
    stringToUTF8(path, str, length);
    console.log(`Should be returning  ${length} bytes for ${path}`);
    return str;
});

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
    assert((net_rx_buffer::MAX_PACKET_SIZE - conn->rx_buf->read_buf.cur_offset - conn->rx_buf->available) > ws_event->numBytes);

    memcpy(conn->rx_buf->read_buf.data + conn->rx_buf->read_buf.cur_offset + conn->rx_buf->available, ws_event->data, ws_event->numBytes);
    conn->rx_buf->available += ws_event->numBytes;
    if (ws_event->numBytes > 0)
    {
        packet_dlog("Added %d bytes to available - result:%d", ws_event->numBytes, conn->rx_buf->available);
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
    char SERVER_URL[64] = "ws://";

    if (!emscripten_websocket_is_supported())
    {
        elog("Websockets not supported in this browser");
        return;
    }

    char *url = get_browser_url();
    strncat(SERVER_URL, url, 64);
    free(url);

    char *url_path = get_browser_url_path();
    conn->can_control = strncmp(url_path, "/control", 8) == 0;
    ilog("URL PATH: %s", url_path);
    free(url);

    ilog("URL: %s", SERVER_URL);

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

intern void alloc_connection(net_connection *conn)
{
    conn->rx_buf = (net_rx_buffer*)malloc(sizeof(net_rx_buffer));
    conn->pckts.scan = (sicklms_laser_scan*)malloc(sizeof(sicklms_laser_scan));
    conn->pckts.ntf = (node_transform*)malloc(sizeof(node_transform));
    conn->pckts.gu = (occ_grid_update *)malloc(sizeof(occ_grid_update));
    conn->pckts.navp = (nav_path *)malloc(sizeof(nav_path));
    conn->pckts.cur_goal_stat = (current_goal_status *)malloc(sizeof(current_goal_status));
    conn->pckts.txt = (text_block *)malloc(sizeof(text_block));
    conn->pckts.cmdp = (command_set_params *)malloc(sizeof(command_set_params));
    conn->pckts.img = (compressed_image *)malloc(sizeof(compressed_image));
    
    memset(conn->rx_buf, 0, sizeof(net_rx_buffer));
    memset(conn->pckts.scan, 0, sizeof(sicklms_laser_scan));
    memset(conn->pckts.ntf, 0, sizeof(node_transform));
    memset(conn->pckts.gu, 0, sizeof(occ_grid_update));
    memset(conn->pckts.navp, 0, sizeof(nav_path));
    memset(conn->pckts.cur_goal_stat, 0, sizeof(current_goal_status));
    memset(conn->pckts.txt, 0, sizeof(text_block));
    memset(conn->pckts.cmdp, 0, sizeof(command_set_params));
    memset(conn->pckts.img, 0, sizeof(compressed_image));
}

intern void free_connection(net_connection *conn)
{
    free(conn->rx_buf);
    free(conn->pckts.scan);
    free(conn->pckts.ntf);
    free(conn->pckts.gu);
    free(conn->pckts.navp);
    free(conn->pckts.cur_goal_stat);
    free(conn->pckts.txt);
    free(conn->pckts.cmdp);
    free(conn->pckts.img);
}

void net_connect(net_connection *conn, const char *ip, int port, int max_timeout_ms)
{
    alloc_connection(conn);
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
    server_addr.sin_port = htons(port);

    if (!inet_pton(AF_INET, ip, &server_addr.sin_addr.s_addr))
    {
        elog("Failed PTON for %s and port %d", ip, port);
        goto cleanup;
    }

    ilog("Connecting to server at %s on port %d", ip, port);
    if (connect(sckt_fd[0].fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0 && errno != EINPROGRESS)
    {
        elog("Failed connecting to server at %s on port %d - resulting fd %d - error %s", ip, port, sckt_fd[0].fd, strerror(errno));
        goto cleanup;
    }

    pret = poll(sckt_fd, 1, max_timeout_ms);
    if (pret == 0)
    {
        elog("Poll timed out connecting to server %s on port %d for fd %d", ip, port, sckt_fd[0].fd);
        goto cleanup;
    }
    else if (pret == -1)
    {
        elog("Failed poll on trying to connect to server at %s on port %d - resulting fd %d - error %s", ip, port, sckt_fd[0].fd, strerror(errno));
        goto cleanup;
    }
    else
    {
        if (test_flags(sckt_fd[0].revents, POLLERR))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLERR", ip, port, sckt_fd[0].fd);
            goto cleanup;
        }
        else if (test_flags(sckt_fd[0].revents, POLLHUP))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLHUP", ip, port, sckt_fd[0].fd);
            goto cleanup;
        }
        else if (test_flags(sckt_fd[0].revents, POLLNVAL))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLNVAL", ip, port, sckt_fd[0].fd);
            goto cleanup;
        }
    }
    conn->socket_handle = sckt_fd[0].fd;
    ilog("Successfully connected to server at %s on port %d - resulting fd %d", ip, port, sckt_fd[0].fd);
    return;

cleanup:
    close(sckt_fd[0].fd);
    conn->socket_handle = -1;
#endif
}

intern void handle_scan_packet(binary_fixed_buffer_archive<net_rx_buffer::MAX_PACKET_SIZE> &read_buf, sizet available, sizet cached_offset, net_connection *conn)
{
    pack_unpack(read_buf, conn->pckts.scan->header, {"header"});
    pack_unpack(read_buf, conn->pckts.scan->meta, {"meta"});

    sizet meta_and_header_size = read_buf.cur_offset - cached_offset;
    sizet range_count = (sizet)((conn->pckts.scan->meta.angle_max - conn->pckts.scan->meta.angle_min) / conn->pckts.scan->meta.angle_increment) + 1;
    sizet total_packet_size = range_count * sizeof(float) + meta_and_header_size;

    if (available >= total_packet_size)
    {
        pack_unpack(read_buf, conn->pckts.scan->ranges, {"change_elems", {pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &range_count}});
        conn->scan_received(0, *conn->pckts.scan);
    }
    else
    {
        // Not all bytes have come in for packet - set back the cur_offset to what it was before reading the meta data
        read_buf.cur_offset = cached_offset;
    }
}

intern void handle_occ_grid_pckt(binary_fixed_buffer_archive<net_rx_buffer::MAX_PACKET_SIZE> &read_buf, sizet available, sizet cached_offset, occ_grid_update * gu, ss_signal<const occ_grid_update&> &sig)
{    
    pack_unpack(read_buf, gu->header, {"header"});
    pack_unpack(read_buf, gu->meta, {"meta"});

    sizet meta_and_header_size = read_buf.cur_offset - cached_offset;
    sizet total_packet_size = gu->meta.change_elem_count * sizeof(u32) + meta_and_header_size;

    if (available >= total_packet_size)
    {
        pack_unpack(read_buf, gu->change_elems, {"change_elems", {pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &gu->meta.change_elem_count}});
        sig(0, *gu);
    }
    else
    {
        // Not all bytes have come in for packet - set back the cur_offset to what it was before reading the meta data
        read_buf.cur_offset = cached_offset;
    }
}

intern void handle_nav_path_packet(binary_fixed_buffer_archive<net_rx_buffer::MAX_PACKET_SIZE> &read_buf, sizet available, sizet cached_offset, nav_path *npckt, ss_signal<const nav_path &> & sig)
{
    static sizet pose_size = packed_sizeof<pose>();
    pack_unpack(read_buf, npckt->header, {"header"});
    pack_unpack(read_buf, npckt->path_cnt, {"path_cnt"});

    sizet meta_and_header_size = read_buf.cur_offset - cached_offset;
    sizet total_packet_size = npckt->path_cnt * pose_size + meta_and_header_size;

    if (available >= total_packet_size)
    {
        pack_unpack(read_buf, npckt->path, {"path", {pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &npckt->path_cnt}});
        sig(0, *npckt);
    }
    else
    {
        // Not all bytes have come in for packet - set back the cur_offset to what it was before reading the meta data
        read_buf.cur_offset = cached_offset;
    }
}

intern void handle_text_block_packet(binary_fixed_buffer_archive<net_rx_buffer::MAX_PACKET_SIZE> &read_buf, sizet available, sizet cached_offset, text_block *txt_pckt, ss_signal<const text_block &> &sig)
{
    pack_unpack(read_buf, txt_pckt->header, {"header"});
    pack_unpack(read_buf, txt_pckt->txt_size, {"txt_size"});

    sizet meta_and_header_size = read_buf.cur_offset - cached_offset;
    sizet total_packet_size = txt_pckt->txt_size + meta_and_header_size;

    if (available >= total_packet_size)
    {
        pack_unpack(read_buf, txt_pckt->text, {"text", {pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &txt_pckt->txt_size}});
        sig(0, *txt_pckt);
    }
    else
    {
        // Not all bytes have come in for packet - set back the cur_offset to what it was before reading the meta data
        read_buf.cur_offset = cached_offset;
    }
}


intern void handle_comp_img_packet(binary_fixed_buffer_archive<net_rx_buffer::MAX_PACKET_SIZE> &read_buf, sizet available, sizet cached_offset, compressed_image * img, ss_signal<const compressed_image&> &sig)
{    
    pack_unpack(read_buf, img->header, {"header"});
    pack_unpack(read_buf, img->meta, {"meta"});

    sizet meta_and_header_size = read_buf.cur_offset - cached_offset;
    sizet total_packet_size = img->meta.data_size + meta_and_header_size;

    if (available >= total_packet_size)
    {
        pack_unpack(read_buf, img->data, {"data", {pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &img->meta.data_size}});
        sig(0, *img);
    }
    else
    {
        // Not all bytes have come in for packet - set back the cur_offset to what it was before reading the meta data
        read_buf.cur_offset = cached_offset;
    }
}


intern void handle_goal_status_packet(binary_fixed_buffer_archive<net_rx_buffer::MAX_PACKET_SIZE> &read_buf, net_connection *conn)
{
    pack_unpack(read_buf, *conn->pckts.cur_goal_stat, {});
    conn->goal_status_updated(0, *conn->pckts.cur_goal_stat);
}


intern void handle_tform_packet(binary_fixed_buffer_archive<net_rx_buffer::MAX_PACKET_SIZE> &read_buf, net_connection *conn)
{
    pack_unpack(read_buf, *conn->pckts.ntf, {});
    if (conn->pckts.ntf->conn_count != conn->connection_count)
    {
        conn->connection_count = conn->pckts.ntf->conn_count;
        conn->connection_count_change(0, i8(conn->connection_count));
    }
    conn->transform_updated(0, *conn->pckts.ntf);
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
    static sizet nav_path_meta = sizeof(u32);
    static sizet goal_status = packed_sizeof<current_goal_status>();
    static sizet txt_block_sz = packed_sizeof<text_block>();
    static sizet img_meta = packed_sizeof<compressed_image_meta>();

    if (matches_packet_id(SCAN_PACKET_ID, data))
    {
        return scan_size;
    }
    else if (matches_packet_id(LOC_CM_PCKT_ID, data) || matches_packet_id(MAP_PCKT_ID, data) || matches_packet_id(GLOB_CM_PCKT_ID, data))
    {
        return occ_meta;
    }
    else if (matches_packet_id(GLOB_NAVP_PCKT_ID, data) || matches_packet_id(LOC_NAVP_PCKT_ID, data))
    {
        return nav_path_meta;
    }
    else if (matches_packet_id(TFORM_PCKT_ID, data))
    {
        return node_tform;
    }
    else if (matches_packet_id(GOAL_STAT_PCKT_ID, data))
    {
        return goal_status;
    }
    else if (matches_packet_id(SET_PARAMS_RESP_CMD_PCKT_ID, data) || matches_packet_id(GET_PARAMS_RESP_CMD_PCKT_ID, data))
    {
        return txt_block_sz;
    }
    else if (matches_packet_id(COMP_IMG_PCKT_ID, data))
    {
        return img_meta;
    }
    return 0;
}

intern sizet dispatch_received_packet(binary_fixed_buffer_archive<net_rx_buffer::MAX_PACKET_SIZE> &read_buf, sizet available, net_connection *conn)
{
    sizet cached_offset = read_buf.cur_offset;
    if (matches_packet_id(SCAN_PACKET_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_scan_packet(read_buf, available, cached_offset, conn);
    }
    else if (matches_packet_id(TFORM_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_tform_packet(read_buf, conn);
    }
    else if (matches_packet_id(GOAL_STAT_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_goal_status_packet(read_buf, conn);
    }
    else if (matches_packet_id(LOC_CM_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_occ_grid_pckt(read_buf, available, cached_offset, conn->pckts.gu, conn->loc_cm_update_received);
    }
    else if (matches_packet_id(GLOB_CM_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_occ_grid_pckt(read_buf, available, cached_offset, conn->pckts.gu, conn->glob_cm_update_received);
    }
    else if (matches_packet_id(MAP_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_occ_grid_pckt(read_buf, available, cached_offset, conn->pckts.gu, conn->map_update_received);
    }
    else if (matches_packet_id(LOC_NAVP_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_nav_path_packet(read_buf, available, cached_offset, conn->pckts.navp, conn->loc_nav_path_updated);
    }
    else if (matches_packet_id(GLOB_NAVP_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_nav_path_packet(read_buf, available, cached_offset, conn->pckts.navp, conn->glob_nav_path_updated);
    }
    else if (matches_packet_id(SET_PARAMS_RESP_CMD_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_text_block_packet(read_buf, available, cached_offset, conn->pckts.txt, conn->param_set_response_received);
    }
    else if (matches_packet_id(GET_PARAMS_RESP_CMD_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_text_block_packet(read_buf, available, cached_offset, conn->pckts.txt, conn->param_get_response_received);
    }
    else if (matches_packet_id(COMP_IMG_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        handle_comp_img_packet(read_buf, available, cached_offset, conn->pckts.img, conn->image_update);
    }
    return read_buf.cur_offset - cached_offset;
}

intern bool net_socket_read(net_connection *conn)
{
    int rd_cnt = read(conn->socket_handle,
                      conn->rx_buf->read_buf.data + conn->rx_buf->read_buf.cur_offset + conn->rx_buf->available,
                      net_rx_buffer::MAX_PACKET_SIZE - conn->rx_buf->read_buf.cur_offset - conn->rx_buf->available);
    if (rd_cnt > 0)
    {
        conn->rx_buf->available += rd_cnt;
        packet_dlog("Added %d bytes to available - result:%d", rd_cnt, conn->rx_buf->available);
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

    assert(net_rx_buffer::MAX_PACKET_SIZE - conn->rx_buf->available > 0 && "Read buffer size is too small - can't receive complete packet");

#if !defined(__EMSCRIPTEN__)
    if (!net_socket_read(conn))
        return;
#endif

    bool need_more_data = false;
    while (conn->rx_buf->available >= packet_header::size && !need_more_data)
    {
        if (current_packet_size == 0)
        {
            current_packet_size = matching_packet_size(conn->rx_buf->read_buf.data + conn->rx_buf->read_buf.cur_offset);
            if (current_packet_size == 0)
            {
                --conn->rx_buf->available;
                ++conn->rx_buf->read_buf.cur_offset;
            }
            else
            {
                packet_dlog("Found packet header for packet size %d (conn->rx_buf->available:%d  Readbuf offset:%d)",
                            current_packet_size,
                            conn->rx_buf->available,
                            conn->rx_buf->read_buf.cur_offset);
            }
        }
        else if (conn->rx_buf->available >= current_packet_size)
        {
            sizet bytes_processed = dispatch_received_packet(conn->rx_buf->read_buf, conn->rx_buf->available, conn);
            if (bytes_processed > 0)
            {
                conn->rx_buf->available -= bytes_processed;
                packet_dlog(
                    "Read entire packet of %d bytes - ended at offset %d (packet size before was %d) - there are %d remaining conn->rx_buf->available bytes to be read",
                    bytes_processed,
                    conn->rx_buf->read_buf.cur_offset,
                    current_packet_size,
                    conn->rx_buf->available);

                current_packet_size = 0;
            }
            else
            {
                packet_dlog("Waiting on more data for packet header size %d - cur available %d and cur offset %d",
                            current_packet_size,
                            conn->rx_buf->available,
                            conn->rx_buf->read_buf.cur_offset);
                need_more_data = true;
            }
        }
        else
        {
            packet_dlog("Waiting on more data for packet header size %d - cur available %d and cur offset %d",
                        current_packet_size,
                        conn->rx_buf->available,
                        conn->rx_buf->read_buf.cur_offset);
            need_more_data = true;
        }
    }

    if (conn->rx_buf->available == 0)
    {
        conn->rx_buf->read_buf.cur_offset = 0;
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
    free_connection(conn);
}