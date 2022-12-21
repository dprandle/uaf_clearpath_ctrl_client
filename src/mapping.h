#pragma once

#include "math_utils.h"
#include "ss_router.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Urho3D
{
class View3D;
class BillboardSet;
class Node;
class Text;
} // namespace Urho3D

struct ui_info;
struct net_connection;
struct input_data;

struct updated_tform
{
    vec3 pos;
    quat orientation;
    bool dirty {false};
    float wait_timer {0.0f};
};

struct camera_move_control_widget
{
    urho::UIElement * widget;
    urho::Button *forward;
    urho::Button *back;
    urho::Button *left;
    urho::Button *right;
    vec3 world_trans {};
};

struct text_notice_widget
{
    urho::UIElement * widget;
};

struct camera_move_zoom_widget
{
    urho::UIElement * widget;
    urho::Button *zoom_in;
    urho::Button *zoom_out;
    vec3 loc_trans {};
};

struct cam_control_widget
{
    camera_move_control_widget cam_move_widget;
    camera_move_zoom_widget cam_zoom_widget;
    urho::UIElement * root_element;
};

struct accept_param_input_widget
{
    urho::UIElement * widget;
    urho::Button * btn;
    urho::Text * btn_text;
};

enum occ_grid_type
{
    OCC_GRID_TYPE_MAP,
    OCC_GRID_TYPE_COSTMAP
};

struct occ_grid_map
{
    urho::Node * node {};
    urho::Texture2D * rend_texture {};
    urho::Image * image {};
    urho::BillboardSet * bb_set {};

    urho::Color undiscovered {0, 0.7, 0.7, 1};
    urho::Color lethal{1,0,1,0.7};
    urho::Color inscribed{0,1,1,0.7};
    urho::Color possibly_circumscribed {1,0,0,0.7};
    urho::Color no_collision {0,1,0,0.7};
    urho::Color free_space{0,0,0,0};

    int map_type {OCC_GRID_TYPE_MAP};
};

struct nav_goals
{
    vec3 cur_goal {};
    i32 cur_goal_status {-1};
    std::vector<vec3> queued_goals {};
};

struct toolbar_widget
{
    urho::UIElement * widget;
    urho::Button *add_goal;
    urho::Button *cancel_goal;
    urho::Button *clear_maps;
    urho::Button *set_params;
};

struct goal_marker_info
{
    float min_rad {0.25f};
    float max_rad {0.35f};
    float loop_anim_time {0.5f};
    float cur_anim_time {0.0f};
    urho::Color color{0,0.6,0.6,1};
    urho::Color queued_color{0,0.6,0.6,1};
};

struct nav_path_view
{
    static constexpr sizet MAX_LINE_ENTRIES = 10000;
    urho::Color color{0,0.6,0,1};
    goal_marker_info goal_marker;
    sizet entry_count;
    vec3 path_entries[MAX_LINE_ENTRIES];
};

struct map_panel
{
    bool js_enabled {false};
    urho::View3D *view {};

    cam_control_widget cam_cwidget{};
    toolbar_widget toolbar{};
    accept_param_input_widget accept_inp{};

    urho::Node * front_laser {};
    urho::BillboardSet * scan_bb {};
    
    occ_grid_map map {};
    occ_grid_map glob_cmap {};
    nav_path_view npview{};
    
    nav_goals goals{};

    urho::Node * base_link {};
    urho::Node * odom {};
    ss_router router;

    std::unordered_map<std::string, urho::Node*> node_lut;
};

void map_clear_occ_grid(occ_grid_map * ocg);
void map_panel_init(map_panel *jspanel, const ui_info &ui_inf, net_connection *conn, input_data * inp);
void map_panel_term(map_panel *jspanel);