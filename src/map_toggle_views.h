#pragma once

#include <vector>
#include "animated_panel.h"

namespace Urho3D
{
class CheckBox;
}

struct nav_path_view;
struct map_panel;
struct ui_info;
struct net_connection;

struct check_box_text_element
{
    urho::UIElement *widget{};
    urho::CheckBox *cb{};
    urho::Text *txt{};
    urho::Node *node{};
    urho::UIElement *elem{};
    nav_path_view *npview{};
};

struct map_toggle_views_panel
{
    animated_panel apanel;
    std::vector<check_box_text_element> elems;
};

void map_toggle_views_init(map_panel *mp, const ui_info &ui_inf);
void map_toggle_views_handle_resize(map_panel *mp, const ui_info &ui_inf);
void map_toggle_views_term(map_panel *mp);
void map_toggle_views_handle_mouse_released(map_panel *mp, urho::UIElement *elem);
void map_toggle_views_handle_toggle(map_panel *mp, urho::UIElement *elem, net_connection *conn);
