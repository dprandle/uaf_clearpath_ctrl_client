#pragma once

#include "animated_panel.h"

struct text_notice_widget
{
    animated_panel apanel;
    float cur_open_time{0.0f};
    float max_open_timer_time{5.0f};
};

struct accept_param_input_widget
{
    urho::UIElement *widget;
    urho::Button *send_btn;
    urho::Text *send_btn_text;
    urho::Button *get_btn;
    urho::Text *get_btn_text;
};

struct map_panel;
struct ui_info;
struct net_connection;

void param_init(map_panel *mp, net_connection *conn, const ui_info &ui_inf);
void param_term(map_panel *mp);
void param_toggle_input_visible(map_panel *mp);
void param_handle_mouse_released(map_panel *mp, urho::UIElement *elem, net_connection *conn);
