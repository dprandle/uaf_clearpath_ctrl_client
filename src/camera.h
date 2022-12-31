#pragma once

#include "math_utils.h"

namespace Urho3D
{
class UIElement;
class Button;
} // namespace Urho3D

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

struct cam_control_widget
{
    camera_move_control_widget cam_move_widget;
    camera_move_zoom_widget cam_zoom_widget;
    urho::UIElement * root_element;
};

struct map_panel;
struct input_data;
struct ui_info;

void cam_init(map_panel *mp, input_data *inp, const ui_info &ui_inf);
void cam_term(map_panel *mp);
void cam_handle_click_end(map_panel *mp, urho::UIElement * elem);

