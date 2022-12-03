#pragma once

#include "Urho3D/Graphics/BillboardSet.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/UI/View3D.h"

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

struct camera_move_zoom_widget
{
    urho::UIElement * widget;
    urho::Button *zoom_in;
    urho::Button *zoom_out;
    vec3 loc_trans {};
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

struct map_panel
{
    bool js_enabled {false};
    urho::View3D *view {};
    camera_move_control_widget cam_move_widget;
    camera_move_zoom_widget cam_zoom_widget;

    urho::Node * front_laser {};
    urho::BillboardSet * scan_bb {};
    
    occ_grid_map map {};
    occ_grid_map glob_cmap {};

    urho::Node * base_link {};
    urho::Node * odom {};
    ss_router router;

    std::unordered_map<std::string, urho::Node*> node_lut;
};

void map_panel_init(map_panel *jspanel, const ui_info &ui_inf, net_connection *conn, input_data * inp);
void map_panel_term(map_panel *jspanel);