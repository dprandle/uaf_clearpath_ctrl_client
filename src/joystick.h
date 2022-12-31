#pragma once

#include "math_utils.h"
#include "ss_router.h"

namespace Urho3D{
    class BorderImage;
    class Button;
}

struct ui_info;
struct net_connection;

struct joystick_panel
{
    /// Created in jostick_panel_alloc call
    urho::BorderImage * frame {};
    urho::BorderImage * outer_ring {};
    urho::Button * js {};

    ivec2 cached_js_pos;
    vec2 cached_mouse_pos;
    vec2 velocity;
    ss_signal<bool> in_use;
};

void joystick_panel_init(joystick_panel*jspanel, const ui_info & ui_inf, net_connection * conn);
void joystick_panel_term(joystick_panel*jspanel);

