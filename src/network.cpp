#include <cassert>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>

#include "ss_router.h"
#include "typedefs.h"
#include "network.h"
#include "logging.h"
#include <poll.h>

static constexpr int MAX_RX_PACKET_SIZE = 30 * MB_SIZE;

#define SCAN_DEBUG

#if defined(OCC_DEBUG)
#define occ_dlog dlog
#define OCC_PACK_LOG_ENABLE log_set_level(urho::LOG_TRACE);
#define OCC_PACK_LOG_DISABLE log_set_level(urho::LOG_DEBUG);
#else
#define occ_dlog
#define OCC_PACK_LOG_ENABLE
#define OCC_PACK_LOG_DISABLE
#endif

#if defined(SCAN_DEBUG)
#define scan_dlog dlog
#define SCAN_PACK_LOG_ENABLE log_set_level(urho::LOG_TRACE);
#define SCAN_PACK_LOG_DISABLE log_set_level(urho::LOG_DEBUG);
#else
#define scan_dlog
#define SCAN_PACK_LOG_ENABLE
#define SCAN_PACK_LOG_DISABLE
#endif

void net_connect(net_connection *conn, int max_timeout_ms)
{
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

#ifndef __EMSCRIPTEN__
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
#endif
    conn->socket_fd = sckt_fd[0].fd;
    ilog("Successfully connected to server at %s on port %d - resulting fd %d", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
    return;

cleanup:
    close(sckt_fd[0].fd);
    conn->socket_fd = -1;
}

bool matches_packet_id(const char *packet_id, const u8 *data)
{
    int result = strncmp((const char *)data, packet_id, packet_header::size);
    return (result == 0);
}

// Add different packet types to these two functions using helper macros
intern sizet matching_packet_size(u8 *data)
{
    if (matches_packet_id(SCAN_PACKET_ID, data))
    {
        return packed_sizeof<jackal_laser_scan>();
    }
    else if (matches_packet_id(OC_GRID_PCKT_ID, data))
    {
        return packed_sizeof<occupancy_grid_update>();
    }
    return 0;
}

intern sizet dispatch_received_packet(binary_fixed_buffer_archive<MAX_RX_PACKET_SIZE> &read_buf, sizet available, net_connection *conn)
{
    sizet cached_offset = read_buf.cur_offset;
    if (matches_packet_id(SCAN_PACKET_ID, read_buf.data + read_buf.cur_offset))
    {
        SCAN_PACK_LOG_ENABLE
        static jackal_laser_scan scan{};
        pack_unpack(read_buf, scan, {});
        scan_dlog("Got scan packet - %d available bytes and %d packet size - new offset is %d",
             available,
             read_buf.cur_offset - cached_offset,
             read_buf.cur_offset);
        conn->scan_received(0, scan);
        SCAN_PACK_LOG_DISABLE
    }
    else if (matches_packet_id(OC_GRID_PCKT_ID, read_buf.data + read_buf.cur_offset))
    {
        static occupancy_grid_update gu{};
        
        OCC_PACK_LOG_ENABLE
        
        pack_unpack(read_buf, gu.header, {"header"});
        pack_unpack(read_buf, gu.meta, {"meta"});

        sizet meta_and_header_size = read_buf.cur_offset - cached_offset;
        sizet total_packet_size = gu.meta.change_elem_count * sizeof(u32) + meta_and_header_size;
        
        occ_dlog("Got occ grid packet header/meta - %d available bytes and %d calculated packet size (%d header/meta)",
             available,
             total_packet_size,
             meta_and_header_size);

        if (available >= total_packet_size)
        {
            occ_dlog("Received complete occgrid packet");
            pack_unpack(read_buf, gu.change_elems, {"change_elems", {pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &gu.meta.change_elem_count}});
            conn->occ_grid_update_received(0, gu);
        }
        else
        {
            occ_dlog("Not enough available bytes - restoring offset %d to cached offset %d", read_buf.cur_offset, cached_offset);
            read_buf.cur_offset = cached_offset;
        }
        OCC_PACK_LOG_DISABLE
    }
    return read_buf.cur_offset - cached_offset;
}

void net_rx(net_connection *conn)
{
    static binary_fixed_buffer_archive<MAX_RX_PACKET_SIZE> read_buf{PACK_DIR_IN};
    static sizet current_packet_size{0};
    static sizet available{0};

    if (conn->socket_fd <= 0)
        return;

    assert(MAX_RX_PACKET_SIZE - available > 0 && "Read buffer size is too small - can't receive complete packet");

    int rd_cnt = read(conn->socket_fd, read_buf.data + available, MAX_RX_PACKET_SIZE - available);
    if (rd_cnt > 0)
    {
        available += rd_cnt;
        dlog("Got %d more bytes total available %d (current packet size: %d)", rd_cnt, available, current_packet_size);
    }
    else if (rd_cnt < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        elog("Got read error %s", strerror(errno));
        return;
    }

    while (available >= packet_header::size && current_packet_size == 0)
    {
        current_packet_size = matching_packet_size(read_buf.data);
        if (current_packet_size == 0)
        {
            --available;
            ++read_buf.cur_offset;
        }
        else
        {
            dlog("Found packet header for packet size %d (available:%d  Readbuf offset:%d)", current_packet_size, available, read_buf.cur_offset);
        }
    }

    if (current_packet_size > 0 && available >= current_packet_size)
    {
        sizet bytes_processed = dispatch_received_packet(read_buf, available, conn);
        if (bytes_processed > 0)
        {
            available -= bytes_processed;
            dlog("Read entire packet of %d bytes starting at offset %d (packet size before was %d) - there are %d remaining available bytes to be read",
                 bytes_processed,
                 read_buf.cur_offset,
                 current_packet_size,
                 available);
            current_packet_size = 0;
            read_buf.cur_offset = 0;
        }
    }
}

void net_tx(const net_connection &conn, const u8 *data, sizet data_size)
{
    if (conn.socket_fd <= 0)
        return;

    sizet bwritten = write(conn.socket_fd, data, data_size);
    if (bwritten == -1)
    {
        elog("Got write error %d", strerror(errno));
    }
    else if (data_size != bwritten)
    {
        wlog("Bytes written was only %d when tried to write %d", bwritten, data_size);
    }
}

void net_disconnect(net_connection *conn)
{
    if (conn->socket_fd > 0)
        close(conn->socket_fd);
    conn->socket_fd = 0;
}
