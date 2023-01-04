#pragma once

#include "math_utils.h"

namespace Urho3D
{
class UIElement;
class Button;
class CheckBox;
} // namespace Urho3D

struct toolbar_widget
{
    urho::UIElement * widget{};
    urho::CheckBox *add_goal{};
    urho::Button *cancel_goal{};
    urho::Button *clear_maps{};
    urho::CheckBox *set_params{};
    urho::CheckBox *enable_follow{};
    urho::CheckBox *enable_measure{};
    urho::Button *delete_measure{};
};

struct map_panel;
struct ui_info;
struct net_connection;

void toolbar_init(map_panel * mp, const ui_info &ui_inf, bool can_control);
void toolbar_term(map_panel * mp);

void toolbar_handle_mouse_released(map_panel *mp, urho::UIElement * elem, net_connection *conn);
void toolbar_handle_toggle(map_panel *mp, urho::UIElement * elem);

