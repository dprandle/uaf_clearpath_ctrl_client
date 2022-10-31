#pragma once

#include "Urho3D/UI/View3D.h"
#include "math_utils.h"

namespace Urho3D
{
class Scene;
class BorderImage;
class Button;
class Context;
class View3D;
} // namespace Urho3D

struct ui_info;
struct net_connection;

struct map_panel
{
    urho::View3D *view{};
};

void map_panel_run_frame(map_panel *jspanel, net_connection *conn);
void map_panel_init(map_panel *jspanel, const ui_info &ui_inf, net_connection *conn);
void map_panel_term(map_panel *jspanel);