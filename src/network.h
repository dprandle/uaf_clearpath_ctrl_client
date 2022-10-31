#pragma once

#include "ss_router.h"

#if defined(__EMSCRIPTEN__)
inline const int SERVER_PORT = 8080;
#else
inline const int SERVER_PORT = 4000;
#endif
inline const char *SERVER_IP = "192.168.1.103";

inline const int PACKET_STR_ID_LEN = 16;


struct packet_header
{
    char type[PACKET_STR_ID_LEN];
};

struct velocity_info
{
    float linear{0.0};
    float angular{0.0};
};

struct command_velocity
{
    packet_header header{"VEL_CMD_PCKT_ID"};
    velocity_info vinfo{};
};

struct jackal_laser_scan_packet
{
    packet_header header{"SCAN_PCKT_ID"};
    float angle_min;
    float angle_max;
    float angle_increment;
    float range_min;
    float range_max;
    float scan[720];
};

struct net_connection
{
    int socket_fd{0};
    ss_signal<const jackal_laser_scan_packet &> scan_received;
};

void net_connect(net_connection *conn, int max_timeout_ms = -1);

inline bool net_connected(const net_connection &conn)
{
    return conn.socket_fd > 0;
}

void net_rx(net_connection *conn);

void net_tx(const net_connection &conn, const void *data, sizet data_size);

template<class T>
void net_tx(const net_connection &conn, const T &packet)
{
    net_tx(conn, &packet, sizeof(T));
}

void net_disconnect(net_connection *conn);
