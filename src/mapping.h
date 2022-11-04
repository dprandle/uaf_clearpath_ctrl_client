#pragma once

#include "Urho3D/Graphics/BillboardSet.h"
#include "Urho3D/UI/View3D.h"

#include "math_utils.h"
#include "ss_router.h"

namespace Urho3D
{
class View3D;
class BillboardSet;
class Node;
} // namespace Urho3D

struct ui_info;
struct net_connection;
struct input_data;

struct map_panel
{
    urho::View3D *view{};
    urho::Node * scan_node;
    urho::BillboardSet * scan_bb;

    urho::Node * jackal_node;

    ss_router router;
};

void map_panel_run_frame(map_panel *jspanel, net_connection *conn);
void map_panel_init(map_panel *jspanel, const ui_info &ui_inf, net_connection *conn, input_data * inp);
void map_panel_term(map_panel *jspanel);