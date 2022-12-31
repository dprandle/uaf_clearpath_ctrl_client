#pragma once

#include "math_utils.h"

namespace Urho3D
{
class UIElement;
class Button;
} // namespace Urho3D

struct toolbar_widget
{
    urho::UIElement * widget;
    urho::Button *add_goal;
    urho::Button *cancel_goal;
    urho::Button *clear_maps;
    urho::Button *set_params;
};

struct map_panel;
struct ui_info;
struct net_connection;

void toolbar_init(map_panel * mp, const ui_info &ui_inf);
void toolbar_term(map_panel * mp);
void toolbar_handle_click_end(map_panel *mp, urho::UIElement * elem, net_connection *conn);

