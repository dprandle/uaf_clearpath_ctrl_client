#include <Urho3D/UI/Button.h>

#include "mapping.h"
#include "animated_panel.h"

void set_max_anchor(animated_panel *panel, float x, float y)
{
    panel->widget->SetMaxAnchor(x, y);
}

void set_min_anchor(animated_panel *panel, float x, float y)
{
    panel->widget->SetMinAnchor(x, y);
}

vec2 get_max_anchor(animated_panel * panel)
{
    return panel->widget->GetMaxAnchor();
}

vec2 get_min_anchor(animated_panel * panel)
{
    return panel->widget->GetMinAnchor();
}

bool animated_panel_run_frame(animated_panel *panel, float dt, const ui_info &ui_inf, const urho::String &prefix)
{
    if (panel->anim_state == PANEL_ANIM_INACTIVE)
        return false;

    auto set_anchor = &set_max_anchor;
    auto get_anchor = &get_max_anchor;
    if (panel->anchor_rest_point > panel->anchor_set_point)
    {
        set_anchor = &set_min_anchor;
        get_anchor = &get_min_anchor;
    }

    float mult = panel->cur_anim_time / panel->max_anim_time;
    if (panel->anim_state == PANEL_ANIM_HIDE)
        mult = 1 - mult;

    float cur_anchor = panel->anchor_rest_point + mult * (panel->anchor_set_point - panel->anchor_rest_point);

    if (panel->anim_dir == PANEL_ANIM_VERTICAL)
        set_anchor(panel, get_anchor(panel).x_, cur_anchor);
    else
        set_anchor(panel, cur_anchor, get_anchor(panel).y_);

    panel->cur_anim_time += dt;
    // ilog("Cur anchor: %f", cur_anchor);
    // ilog("Set Point: %f", panel->anchor_set_point);
    // ilog("Rest Point: %f", panel->anchor_rest_point);
    // ilog("Mult: %f", mult);

    if (panel->cur_anim_time >= panel->max_anim_time)
    {
        if (panel->anim_state == PANEL_ANIM_SHOW)
        {
            cur_anchor = panel->anchor_set_point;
            panel->hide_show_panel->SetStyle(prefix + "Hide", ui_inf.style);
        }
        else
        {
            cur_anchor = panel->anchor_rest_point;
            panel->hide_show_panel->SetStyle(prefix + "Show", ui_inf.style);
        }

        if (panel->anim_dir == PANEL_ANIM_VERTICAL)
            set_anchor(panel, get_anchor(panel).x_, cur_anchor);
        else
            set_anchor(panel, cur_anchor, get_anchor(panel).y_);

        panel->anim_state = PANEL_ANIM_INACTIVE;
        panel->cur_anim_time = 0.0f;
    }
    return true;
}

void animated_panel_hide_show_pressed(animated_panel *panel)
{
    if (panel->anim_state != PANEL_ANIM_INACTIVE)
        return;

    auto get_anchor = &get_max_anchor;
    if (panel->anchor_rest_point > panel->anchor_set_point)
        get_anchor = &get_min_anchor;

    float cur_anch = get_anchor(panel).y_;
    if (panel->anim_dir == PANEL_ANIM_HORIZONTAL)
        cur_anch = get_anchor(panel).x_;

    if (fequals(cur_anch, panel->anchor_set_point))
        panel->anim_state = PANEL_ANIM_HIDE;
    else
        panel->anim_state = PANEL_ANIM_SHOW;
}