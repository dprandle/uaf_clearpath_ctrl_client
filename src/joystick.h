#pragma once

#include "Urho3D/Core/Context.h"
#include "Urho3D/UI/BorderImage.h"
#include "math_utils.h"

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
};

void joystick_panel_run_frame(joystick_panel*jspanel, net_connection * conn);
void joystick_panel_init(joystick_panel*jspanel, const ui_info & ui_inf, net_connection * conn);
void joystick_panel_term(joystick_panel*jspanel);

