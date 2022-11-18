#pragma once

#include "Urho3D/Math/Quaternion.h"
#include "ss_router.h"
#include "math_utils.h"
#include "pack_unpack.h"

#if defined(__EMSCRIPTEN__)
inline const int SERVER_PORT = 8080;
#else
inline const int SERVER_PORT = 4000;
#endif
inline const char *SERVER_IP = "192.168.1.103";

inline const char *SCAN_PACKET_ID = "SCAN_PCKT_ID";
inline const char *OC_GRID_PCKT_ID = "OC_GRID_PCKT_ID";
inline const char *TFORM_PCKT_ID = "TFORM_PCKT_ID";

struct dvec3
{
    static constexpr int size = 24;
    double x{};
    double y{};
    double z{};
};

pup_func(dvec3)
{
    pup_member(x);
    pup_member(y);
    pup_member(z);
}

struct dquat
{
    static constexpr int size = 32;
    double x{};
    double y{};
    double z{};
    double w{};
};

pup_func(dquat)
{
    pup_member(x);
    pup_member(y);
    pup_member(z);
    pup_member(w);
}

inline vec3 vec3_from(const dvec3 &vec)
{
    return {(float)vec.x, (float)vec.y, -(float)vec.z};
}

inline quat quat_from(const dquat &q)
{
    return {(float)q.w, (float)q.x, (float)q.y, (float)q.z};
}

struct packet_header
{
    static constexpr int size = 16;
    char type[size];
};

pup_func(packet_header)
{
    pup_member(type);
}

struct velocity_info
{
    float linear{0.0};
    float angular{0.0};
};

pup_func(velocity_info)
{
    pup_member(linear);
    pup_member(angular);
}

struct command_velocity
{
    packet_header header{"VEL_CMD_PCKT_ID"};
    velocity_info vinfo{};
};

pup_func(command_velocity)
{
    pup_member(header);
    pup_member(vinfo);
}

struct sicklms_laser_scan_meta
{
    float angle_min;
    float angle_max;
    float angle_increment;
    float range_min;
    float range_max;
};

inline sizet sicklms_get_range_count(const sicklms_laser_scan_meta & meta) {
    return (sizet)((meta.angle_max - meta.angle_min)/meta.angle_increment) + 1;
}

pup_func(sicklms_laser_scan_meta)
{
    pup_member(angle_min);
    pup_member(angle_max);
    pup_member(angle_increment);
    pup_member(range_min);
    pup_member(range_max);
}

struct sicklms_laser_scan
{
    static constexpr int MAX_SCAN_POINTS = 1000;
    packet_header header{"SCAN_PCKT_ID"};
    sicklms_laser_scan_meta meta;
    float ranges[MAX_SCAN_POINTS];
};

pup_func(sicklms_laser_scan)
{
    pup_member(header);
    pup_member(meta);
    pup_member(ranges);
}

struct node_transform
{
    static constexpr int NODE_NAME_SIZE = 32;
    packet_header header{"TFORM_PCKT_ID"};
    char parent_name[NODE_NAME_SIZE];
    char name[NODE_NAME_SIZE];
    dvec3 pos;
    dquat orientation;
};

pup_func(node_transform)
{
    pup_member(header);
    pup_member(parent_name);
    pup_member(name);
    pup_member(pos);
    pup_member(orientation);
}

struct occ_grid_meta
{
    static constexpr int size = 4 * 4 + dvec3::size + dquat::size;
    float resolution;
    u32 width;
    u32 height;
    dvec3 origin_pos;
    dquat origin_orientation;
    u32 change_elem_count;
};

pup_func(occ_grid_meta)
{
    pup_member(resolution);
    pup_member(width);
    pup_member(height);
    pup_member(origin_pos);
    pup_member(origin_orientation);
    pup_member(change_elem_count);
}

struct occupancy_grid_update
{
    static constexpr int MAX_CHANGE_ELEMS = 10000000;
    static constexpr int size = packet_header::size + occ_grid_meta::size + MAX_CHANGE_ELEMS;
    packet_header header{};
    occ_grid_meta meta;
    u32 change_elems[MAX_CHANGE_ELEMS];
};

pup_func(occupancy_grid_update)
{
    pup_member(header);
    pup_member(meta);
    pup_member_meta(change_elems, pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &val.meta.change_elem_count);
}

struct net_connection
{
    int socket_handle{0};

    ss_signal<const sicklms_laser_scan &> scan_received;
    ss_signal<const occupancy_grid_update &> occ_grid_update_received;
    ss_signal<const node_transform &> transform_updated;
};

void net_connect(net_connection *conn, int max_timeout_ms = -1);

inline bool net_connected(const net_connection &conn)
{
    return conn.socket_handle > 0;
}

void net_rx(net_connection *conn);

void net_tx(const net_connection &conn, const u8 *data, sizet data_size);

template<class T>
void net_tx(const net_connection &conn, const T &packet)
{
    binary_fixed_buffer_archive<sizeof(T)> buf {PACK_DIR_OUT};
    auto no_const = const_cast<T&>(packet);
    pack_unpack(buf, no_const, {});
    net_tx(conn, buf.data, buf.cur_offset);
}

void net_disconnect(net_connection *conn);