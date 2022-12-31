#pragma once

#include "math_utils.h"

namespace Urho3D
{
class UIElement;
class Button;
class Text;
class ListView;
} // namespace Urho3D

enum text_notic_anim_state
{
    TEXT_NOTICE_ANIM_INACTIVE,
    TEXT_NOTICE_ANIM_SHOW,
    TEXT_NOTICE_ANIM_HIDE,
};

struct text_notice_widget
{
    urho::UIElement * widget;
    urho::ListView * sview;
    urho::UIElement * hide_show_btn_bg;
    urho::Button * hide_show_panel;
    float max_y_anchor {};
    float cur_anim_time {0.0f};
    float max_anim_time {0.2f};

    float cur_open_time {0.0f};
    float max_open_timer_time {5.0f};
    text_notic_anim_state anim_state {TEXT_NOTICE_ANIM_INACTIVE};
};

struct accept_param_input_widget
{
    urho::UIElement * widget;
    urho::Button * send_btn;
    urho::Text * send_btn_text;
    urho::Button * get_btn;
    urho::Text * get_btn_text;
};

struct map_panel;
struct ui_info;
struct net_connection;

void param_init(map_panel * mp, net_connection * conn, const ui_info &ui_inf);
void param_term(map_panel * mp);
void param_toggle_input_visible(map_panel * mp);
void param_handle_click_end(map_panel *mp, urho::UIElement * elem, net_connection *conn);
