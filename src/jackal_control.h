#pragma once

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Math/Vector3.h>

#include "input.h"
#include "ss_router.h"
#include "typedefs.h"

namespace Urho3D
{
class Node;
class Scene;
class Sprite;
}


class Input_Map;
struct Input_Context;

struct jctrl_uevent_handlers : urho::Object
{
    URHO3D_OBJECT(jctrl_uevent_handlers, urho::Object)
    jctrl_uevent_handlers(urho::Context * ctxt);

    void subscribe();
    void unsubscribe();
    bool debug_draw {false};
};

struct jackal_control_ctxt
{
    urho::Context* urho_ctxt {};

    urho::Engine * urho_engine {};
    urho::Scene * scene {};    
    urho::Node * cam_node {};

    input_context_stack input_dispatch {};
    input_keymap input_map {};

    jctrl_uevent_handlers *event_handler;
    ss_router router;
};

void jctrl_alloc(jackal_control_ctxt * ctxt, urho::Context* urho_ctxt);
void jctrl_free(jackal_control_ctxt * ctxt);

bool jctrl_init(jackal_control_ctxt * ctxt);
void jctrl_terminate(jackal_control_ctxt * ctxt);
void jctrl_exec(jackal_control_ctxt * ctxt);
