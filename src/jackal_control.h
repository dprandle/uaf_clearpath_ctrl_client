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
#include "mapping.h"

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

struct input_data
{
    input_context_stack dispatch;
    input_keymap map;
};

struct jackal_control_ctxt
{
    urho::Context* urho_ctxt {};
    urho::Engine * urho_engine {};

    ui_info ui_inf;

    joystick_panel js_panel;
    map_panel mpanel;
    input_data inp;

    net_connection conn;

    ss_router router;
};

bool jctrl_init(jackal_control_ctxt * ctxt);
void jctrl_term(jackal_control_ctxt * ctxt);

void jctrl_exec(jackal_control_ctxt * ctxt);
void jctrl_run_frame(jackal_control_ctxt * ctxt);
