#pragma once

#include <Urho3D/Engine/Application.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Math/Vector3.h>

#include "input.h"
#include "typedefs.h"
#include "network.h"
#include "joystick.h"
#include "mapping.h"

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

bool jctrl_init(jackal_control_ctxt * ctxt, const urho::StringVector & args);
void jctrl_term(jackal_control_ctxt * ctxt);

void jctrl_exec(jackal_control_ctxt * ctxt);
void jctrl_run_frame(jackal_control_ctxt * ctxt);
