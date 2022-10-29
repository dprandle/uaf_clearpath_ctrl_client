#pragma once

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Math/Vector3.h>
#include <vector>

#include "input.h"
#include "ss_router.h"
#include "typedefs.h"

namespace Urho3D
{
class Node;
class Scene;
class Sprite;
class Button;
}


class Input_Map;
struct Input_Context;

struct command_packet_header
{
    u8 command_type;
    u8 future;
    u8 future2;
    u8 future3;
};

struct velocity_info
{
    float linear {0.0};
    float angular {0.0};
};

struct command_velocity
{
    command_packet_header header {0};
    velocity_info vinfo {};
};

struct jctrl_uevent_handlers : urho::Object
{
    URHO3D_OBJECT(jctrl_uevent_handlers, urho::Object)
    jctrl_uevent_handlers(urho::Context * ctxt);

    void subscribe();
    void unsubscribe();
    bool debug_draw {false};
};

struct joystick_panel
{
    urho::BorderImage * frame {};
    urho::BorderImage * outer_ring {};
    urho::Button * js {};
    ivec2 cached_js_pos {};
    vec2 cached_mouse_pos {};
};

struct jackal_control_ctxt
{
    urho::Context* urho_ctxt {};

    urho::Engine * urho_engine {};
    urho::Scene * scene {};    
    urho::Node * cam_node {};

    joystick_panel js_panel {};
    input_context_stack input_dispatch {};
    input_keymap input_map {};

    jctrl_uevent_handlers *event_handler;
    ss_router router;

    command_velocity vel_cmd;
    int socket_fd {0};
    float dev_pixel_ratio_inv {1.0};
};

void jctrl_alloc(jackal_control_ctxt * ctxt, urho::Context* urho_ctxt);
void jctrl_free(jackal_control_ctxt * ctxt);

bool jctrl_init(jackal_control_ctxt * ctxt);
void jctrl_terminate(jackal_control_ctxt * ctxt);
void jctrl_run_frame(jackal_control_ctxt * ctxt);
void jctrl_exec(jackal_control_ctxt * ctxt);
