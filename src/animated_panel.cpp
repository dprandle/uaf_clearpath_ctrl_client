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

vec2 get_max_anchor(animated_panel *panel)
{
    return panel->widget->GetMaxAnchor();
}

vec2 get_min_anchor(animated_panel *panel)
{
    return panel->widget->GetMinAnchor();
}

bool animated_panel_run_frame(animated_panel *panel, float dt, const ui_info &ui_inf, const urho::String &prefix)
{
    if (panel->anim_state == PANEL_ANIM_INACTIVE)
        return false;

    // By default we animate the element's max anchor, but if the rest point is greater than the set point we animate
    // the min anchor. This allows the panel to show and hide correctly no matter the direction.
    auto set_anchor = &set_max_anchor;
    auto get_anchor = &get_max_anchor;
    if (panel->anchor_rest_point > panel->anchor_set_point) {
        set_anchor = &set_min_anchor;
        get_anchor = &get_min_anchor;
    }

    // Get our normalized animation state from 0 to 1 - 0 being just starting and 1 being animation complete. If we are
    // hiding the panel, 0 is complete and 1 is just starting.
    float mult = panel->cur_anim_time / panel->max_anim_time;
    if (panel->anim_state == PANEL_ANIM_HIDE)
        mult = 1 - mult;

    // Interpolate our current anchor position using the above multiplier
    float cur_anchor = panel->anchor_rest_point + mult * (panel->anchor_set_point - panel->anchor_rest_point);

    // If the panel is vertical, only animate the y component and just pass in the unchanged x component
    // If the panel is horizontal, do the opposite.
    if (panel->anim_dir == PANEL_ANIM_VERTICAL)
        set_anchor(panel, get_anchor(panel).x_, cur_anchor);
    else
        set_anchor(panel, cur_anchor, get_anchor(panel).y_);

    // Increment our time by the frame delta (dt)
    panel->cur_anim_time += dt;
    
    // If the animation time has reached or exceeded the max animation time, set the panel accordingly and mark the
    // animation as complete
    if (panel->cur_anim_time >= panel->max_anim_time) {
        if (panel->anim_state == PANEL_ANIM_SHOW) {
            cur_anchor = panel->anchor_set_point;
            panel->hide_show_panel->SetStyle(prefix + "Hide", ui_inf.style);
        }
        else {
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
