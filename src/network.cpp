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

static constexpr int MAX_RX_PACKET_SIZE = 1 * MB_SIZE;

void net_connect(net_connection * conn, int max_timeout_ms)
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

bool packet_matching_id_up_to(u8 *packet, const char *packet_id, int up_to_this_many_chars)
{
    return (strncmp((const char*)packet, packet_id, up_to_this_many_chars) == 0);
}

template<class T>
const T* cast_packet_if_matching(const char * packet_id, u8 *packet, int cur_ind)
{
    if (packet_matching_id_up_to(packet, packet_id, PACKET_STR_ID_LEN) && cur_ind == sizeof(T))
    {
        auto casted_packet = (const T *)packet;
        return casted_packet;
    }
    return nullptr;
}

// Add different packet types to these two functions
intern bool matches_any_headers_so_far(u8 * packet, int cur_ind)
{
    return (packet_matching_id_up_to(packet, "SCAN_PCKT_ID", cur_ind + 1));
}

intern bool dispatch_received_packet_if_ready(u8 *packet, int cur_ind, net_connection *conn)
{
    auto scan_packet = cast_packet_if_matching<jackal_laser_scan_packet>("SCAN_PCKT_ID", packet, cur_ind);
    if (scan_packet)
    {
        conn->scan_received(0, *scan_packet);
        return true;
    }
    return false;
}


void net_rx(net_connection *conn)
{
    static u8 read_buf[MAX_RX_PACKET_SIZE] {};
    static i32 cur_packet_ind {0};

    if (conn->socket_fd <= 0)
        return;
    
    int rd_cnt = read(conn->socket_fd, read_buf, MAX_RX_PACKET_SIZE);
    int cur_rd{0};
    while (cur_rd < rd_cnt)
    {
        if (cur_packet_ind < PACKET_STR_ID_LEN)
        {
            if (matches_any_headers_so_far(read_buf, cur_packet_ind ))
                ++cur_packet_ind;
            else
                cur_packet_ind = 0;
        }
        else
        {
            ++cur_packet_ind;
            if (dispatch_received_packet_if_ready(read_buf, cur_packet_ind, conn))
                cur_packet_ind = 0;
        }
        ++cur_rd;
    }

    if (rd_cnt < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        elog("Got read error %s", strerror(errno));
    }
}

void net_tx(const net_connection & conn, const void*data, sizet data_size)
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

void net_disconnect(net_connection * conn)
{
    if (conn->socket_fd > 0)
        close(conn->socket_fd);
    conn->socket_fd = 0;
}
