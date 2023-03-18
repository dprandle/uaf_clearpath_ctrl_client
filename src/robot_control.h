#pragma once

#include "input.h"
#include "typedefs.h"
#include "network.h"
#include "joystick.h"
#include "mapping.h"

namespace Urho3D
{
class Context;
class Engine;
} // namespace Urho3D

struct robot_control_ctxt
{
    urho::Context *urho_ctxt{};
    urho::Engine *urho_engine{};

    ui_info ui_inf;

    joystick_panel js_panel;
    map_panel mpanel;
    input_data inp;

    net_connection conn;

    ss_router router;
};

bool robot_ctrl_init(robot_control_ctxt *ctxt, const urho::StringVector &args);
void robot_ctrl_term(robot_control_ctxt *ctxt);
void robot_ctrl_exec(robot_control_ctxt *ctxt);
