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
    vec3 current_translation {};
};

struct map_panel
{
    bool js_enabled {false};
    urho::View3D *view {};
    camera_move_control_widget cam_move_widget;

    urho::Node * front_laser {};
    urho::BillboardSet * scan_bb {};
    
    urho::Node * map {};
    urho::Texture2D * map_text {};
    urho::Image * map_image {};
    urho::BillboardSet * occ_grid_bb {};

    urho::Node * base_link {};
    urho::Node * odom {};
    ss_router router;

    std::unordered_map<std::string, urho::Node*> node_lut;
};

void map_panel_init(map_panel *jspanel, const ui_info &ui_inf, net_connection *conn, input_data * inp);
void map_panel_term(map_panel *jspanel);