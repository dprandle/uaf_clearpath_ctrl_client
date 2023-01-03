#pragma once

#include "animated_panel.h"

namespace Urho3D
{
    class CheckBox;
}

struct nav_path_view;
struct map_panel;
struct ui_info;

struct check_box_text_element
{
    urho::UIElement * widget{};
    urho::CheckBox * cb{};
    urho::Text * txt{};
    urho::Node * node{};
    nav_path_view * npview{};
};

struct map_toggle_views_panel
{
    animated_panel apanel;
    std::vector<check_box_text_element> elems;
};

void map_toggle_views_init(map_panel * mp, const ui_info & ui_inf);
void map_toggle_views_term(map_panel * mp);
void map_toggle_views_handle_click_end(map_panel *mp, urho::UIElement * elem);

