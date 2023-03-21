#pragma once

#include "Urho3D/Math/Quaternion.h"
#include "ss_router.h"
#include "math_utils.h"
#include "pack_unpack.h"

inline const char *SCAN_PACKET_ID = "SCAN_PCKT_ID";
inline const char *MAP_PCKT_ID = "MAP_PCKT_ID";
inline const char *GLOB_CM_PCKT_ID = "GLOB_CM_PCKT_ID";
inline const char *LOC_CM_PCKT_ID = "LOC_CM_PCKT_ID";
inline const char *ENABLE_IMG_CMD_ID = "ENABLE_IMG_CMD_ID";
inline const char *DISABLE_IMG_CMD_ID = "DISABLE_IMG_CMD_ID";
inline const char *TFORM_PCKT_ID = "TFORM_PCKT_ID";
inline const char *GLOB_NAVP_PCKT_ID = "GLOB_NAVP_PCKT_ID";
inline const char *LOC_NAVP_PCKT_ID = "LOC_NAVP_PCKT_ID";
inline const char *GOAL_STAT_PCKT_ID = "GOAL_STAT_PCKT_ID";
inline const char *COMP_IMG_PCKT_ID = "COMP_IMG_PCKT_ID";
inline const char *MISC_STATS_PCKT_ID = "MISC_STATS_PCKT_ID";

inline const char *SET_PARAMS_RESP_CMD_PCKT_ID = "SET_PARAMS_RESP_CMD_PCKT_ID";
inline const char *GET_PARAMS_RESP_CMD_PCKT_ID = "GET_PARAMS_RESP_CMD_PCKT_ID";

inline const char *VEL_CMD_HEADER = "VEL_CMD_PCKT_ID";
inline const char *GOAL_CMD_HEADER = "GOAL_CMD_PCKT_ID";
inline const char *STOP_CMD_HEADER = "STOP_CMD_PCKT_ID";
inline const char *CLEAR_MAPS_CMD_HEADER = "CLEAR_MAPS_PCKT_ID";
inline const char *SET_PARAMS_CMD_HEADER = "SET_PARAMS_CMD_PCKT_ID";
inline const char *GET_PARAMS_CMD_HEADER = "GET_PARAMS_CMD_PCKT_ID";

static constexpr int MAX_MAP_SIZE = 4000;
static constexpr int MAX_IMAGE_SIZE = 1024;
struct dvec3
{
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

pup_func(dvec3)
{
    pup_member(x);
    pup_member(y);
    pup_member(z);
}

struct dquat
{
    double x{0.0};
    double y{0.0};
    double z{0.0};
    double w{1.0};
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
    return {(float)q.w, (float)q.x, (float)-q.y, (float)q.z};
}

inline dvec3 dvec3_from(const vec3 &vec)
{
    return {(float)vec.x_, (float)vec.y_, -(float)vec.z_};
}

inline dquat quat_from(const quat &q)
{
    return {(float)q.w_, (float)q.x_, (float)-q.y_, (float)q.z_};
}

struct packet_header
{
    static constexpr int size = 32;
    char type[size];
};

struct pose
{
    dvec3 pos;
    dquat orientation;
};

pup_func(pose)
{
    pup_member(pos);
    pup_member(orientation);
}

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

struct command_goal
{
    packet_header header{"GOAL_CMD_PCKT_ID"};
    pose goal_p;
};

pup_func(command_goal)
{
    pup_member(header);
    pup_member(goal_p);
}

struct command_stop
{
    packet_header header{"STOP_CMD_PCKT_ID"};
};

pup_func(command_stop)
{
    pup_member(header);
}

struct command_clear_maps
{
    packet_header header{"CLEAR_MAPS_PCKT_ID"};
};

pup_func(command_clear_maps)
{
    pup_member(header);
}

struct command_get_params
{
    packet_header header{"GET_PARAMS_CMD_PCKT_ID"};
};

pup_func(command_get_params)
{
    pup_member(header);
}

struct command_enable_image
{
    packet_header header{"ENABLE_IMG_CMD_ID"};
};

pup_func(command_enable_image)
{
    pup_member(header);
}

struct command_disable_image
{
    packet_header header{"DISABLE_IMG_CMD_ID"};
};

pup_func(command_disable_image)
{
    pup_member(header);
}

struct command_set_params
{
    static constexpr sizet MAX_STR_SIZE = std::numeric_limits<u16>::max();
    packet_header header{"SET_PARAMS_CMD_PCKT_ID"};
    u32 blob_size;
    u8 blob_data[MAX_STR_SIZE];
};

pup_func(command_set_params)
{
    pup_member(header);
    pup_member(blob_size);
    pup_member_meta(blob_data, pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &val.blob_size);
}

struct lidar_scan_meta
{
    float angle_min;
    float angle_max;
    float angle_increment;
    float range_min;
    float range_max;
};

inline sizet lidar_get_range_count(const lidar_scan_meta &meta)
{
    return (sizet)((meta.angle_max - meta.angle_min) / meta.angle_increment) + 1;
}

pup_func(lidar_scan_meta)
{
    pup_member(angle_min);
    pup_member(angle_max);
    pup_member(angle_increment);
    pup_member(range_min);
    pup_member(range_max);
}

struct lidar_scan
{
    static constexpr int MAX_SCAN_POINTS = 1000;
    packet_header header{};
    lidar_scan_meta meta;
    float ranges[MAX_SCAN_POINTS];
};

pup_func(lidar_scan)
{
    pup_member(header);
    pup_member(meta);
    pup_member(ranges);
}

struct misc_stats
{
    packet_header header{};
    u8 conn_count{0};
    float cur_bw_mbps{};
    float avg_bw_mbps{};
};

pup_func(misc_stats)
{
    pup_member(header);
    pup_member(conn_count);
    pup_member(cur_bw_mbps);
    pup_member(avg_bw_mbps);
}

struct node_transform
{
    static constexpr int NODE_NAME_SIZE = 32;
    packet_header header{};
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

struct compressed_image_meta
{
    u8 format; // This is really a placeholder in case the jackal uses a different format
    u32 data_size;
};

pup_func(compressed_image_meta)
{
    pup_member(format);
    pup_member(data_size);
};

struct compressed_image
{
    static constexpr int MAX_PIXEL_COUNT = MAX_IMAGE_SIZE * MAX_IMAGE_SIZE;
    packet_header header{};
    compressed_image_meta meta{};
    u8 data[MAX_PIXEL_COUNT * 2]; // max 2 bytes per pixel - if an image was basically uncompressable
};

pup_func(compressed_image)
{
    pup_member(header);
    pup_member(meta);
    pup_member_meta(data, pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &val.meta.data_size);
};

struct text_block
{
    static constexpr int MAX_TXT_SIZE = 30000;
    packet_header header{};
    u32 txt_size{0};
    char text[MAX_TXT_SIZE];
};

pup_func(text_block)
{
    pup_member(header);
    pup_member(txt_size);
    pup_member_meta(text, pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &val.txt_size);
}

struct occ_grid_meta
{
    float resolution;
    u32 width;
    u32 height;
    pose origin_p;
    i8 reset_map;
    u32 change_elem_count;
};

pup_func(occ_grid_meta)
{
    pup_member(resolution);
    pup_member(width);
    pup_member(height);
    pup_member(origin_p);
    pup_member(reset_map);
    pup_member(change_elem_count);
}

struct occ_grid_update
{
    static constexpr int MAX_CHANGE_ELEMS = MAX_MAP_SIZE * MAX_MAP_SIZE;
    packet_header header{};
    occ_grid_meta meta;
    u32 change_elems[MAX_CHANGE_ELEMS];
};

pup_func(occ_grid_update)
{
    pup_member(header);
    pup_member(meta);
    pup_member_meta(change_elems, pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &val.meta.change_elem_count);
}

struct nav_path
{
    static constexpr int MAX_PATH_ELEMS = 10000;
    packet_header header{};
    u32 path_cnt;
    pose path[MAX_PATH_ELEMS];
};

pup_func(nav_path)
{
    pup_member(header);
    pup_member(path_cnt);
    pup_member_meta(path, pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE, &val.path_cnt);
}

struct current_goal_status
{
    packet_header header{};
    i32 status;
    pose goal_p;
};

pup_func(current_goal_status)
{
    pup_member(header);
    pup_member(status);
    pup_member(goal_p);
}

/// Only malloc these once and reuse on every time a packet comes in
struct reusable_packets
{
    // Packets for receiving
    occ_grid_update *gu{};
    lidar_scan *scan{};
    node_transform *ntf{};
    nav_path *navp{};
    current_goal_status *cur_goal_stat{};
    text_block *txt{};
    compressed_image *img{};
    misc_stats *ms{};

    // Packets for sending
    command_set_params *cmdp{};
};

struct net_rx_buffer
{
    static constexpr int MAX_PACKET_SIZE = occ_grid_update::MAX_CHANGE_ELEMS * 4 + 1000;
    binary_fixed_buffer_archive<MAX_PACKET_SIZE> read_buf{PACK_DIR_IN};
    sizet available;
};

struct net_connection
{
    int socket_handle{0};

    net_rx_buffer *rx_buf{};
    reusable_packets pckts{};
    bool can_control{true};

    ss_signal<const lidar_scan &> scan_received;
    ss_signal<const occ_grid_update &> map_update_received;
    ss_signal<const occ_grid_update &> glob_cm_update_received;
    ss_signal<const occ_grid_update &> loc_cm_update_received;
    ss_signal<const node_transform &> transform_updated;
    ss_signal<const nav_path &> glob_nav_path_updated;
    ss_signal<const nav_path &> loc_nav_path_updated;
    ss_signal<const current_goal_status &> goal_status_updated;
    ss_signal<const text_block &> param_set_response_received;
    ss_signal<const text_block &> param_get_response_received;
    ss_signal<const compressed_image &> image_update;
    ss_signal<const misc_stats &> meta_stats_update;
};

void net_connect(net_connection *conn, const char *ip, int port, int max_timeout_ms = -1);

inline bool net_connected(const net_connection &conn)
{
    return conn.socket_handle > 0;
}

void net_rx(net_connection *conn);

void net_tx(const net_connection &conn, const u8 *data, sizet data_size);

template<class T>
void net_tx(const net_connection &conn, const T &packet)
{
    binary_fixed_buffer_archive<sizeof(T)> buf{PACK_DIR_OUT};
    auto no_const = const_cast<T &>(packet);
    pack_unpack(buf, no_const, {});
    net_tx(conn, buf.data, buf.cur_offset);
}

void net_disconnect(net_connection *conn);
