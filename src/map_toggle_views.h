#pragma once

#include "animated_panel.h"

struct map_toggle_views_panel
{
    animated_panel apanel;
};

struct map_panel;
struct ui_info;

void map_toggle_views_init(map_panel * mp, const ui_info & ui_inf);
void map_toggle_views_term(map_panel * mp);
void map_toggle_views_handle_click_end(map_panel *mp, urho::UIElement * elem);

