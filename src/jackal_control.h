#pragma once

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Math/Vector3.h>
#include <vector>

#include "Urho3D/Resource/XMLFile.h"
#include "input.h"
#include "ss_router.h"
#include "typedefs.h"
#include "network.h"
#include "joystick.h"

namespace Urho3D
{
class Node;
class Scene;
class Sprite;
class Button;
class UI;
}

class Input_Map;
struct Input_Context;

struct ui_info
{
    urho::UI * ui_sys {};
    urho::XMLFile * style {};
    float dev_pixel_ratio_inv {1.0};
};

struct jackal_control_ctxt
{
    urho::Context* urho_ctxt {};
    urho::Engine * urho_engine {};
    urho::Scene * scene {};    
    urho::Node * cam_node {};

    ui_info ui_inf;
    joystick_panel js_panel;
    input_context_stack input_dispatch;
    input_keymap input_map;
    net_connection conn;

    ss_router router;
};

void jctrl_alloc(jackal_control_ctxt * ctxt, urho::Context* urho_ctxt);
void jctrl_free(jackal_control_ctxt * ctxt);
bool jctrl_init(jackal_control_ctxt * ctxt);
void jctrl_terminate(jackal_control_ctxt * ctxt);

void jctrl_exec(jackal_control_ctxt * ctxt);
void jctrl_run_frame(jackal_control_ctxt * ctxt);
