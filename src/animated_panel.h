#pragma once

#include "math_utils.h"

namespace Urho3D
{
class UIElement;
class Button;
class Text;
class ListView;
} // namespace Urho3D

enum panel_anim_state
{
    PANEL_ANIM_INACTIVE,
    PANEL_ANIM_SHOW,
    PANEL_ANIM_HIDE
};
enum panel_anim_direction
{
    PANEL_ANIM_HORIZONTAL,
    PANEL_ANIM_VERTICAL
};

struct animated_panel
{
    urho::UIElement *widget;
    urho::ListView *sview;
    urho::UIElement *hide_show_btn_bg;
    urho::Button *hide_show_panel;
    float anchor_set_point{};
    float anchor_rest_point{};
    float cur_anim_time{0.0f};
    float max_anim_time{0.2f};
    panel_anim_state anim_state{PANEL_ANIM_INACTIVE};
    panel_anim_direction anim_dir{};
};

struct ui_info;

void animated_panel_hide_show_pressed(animated_panel *panel);
bool animated_panel_run_frame(animated_panel *panel, float dt, const ui_info &ui_inf, const urho::String &prefix);
