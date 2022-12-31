#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>

#include "toolbar.h"
#include "mapping.h"
#include "network.h"

intern const float UI_ADDITIONAL_TOOLBAR_SCALING = 1.00f;

intern void setup_toolbar_widget(map_panel *mp, const ui_info &ui_inf)
{
    auto uctxt = ui_inf.ui_sys->GetContext();

    mp->toolbar.widget = new urho::BorderImage(uctxt);
    ui_inf.ui_sys->GetRoot()->AddChild(mp->toolbar.widget);
    mp->toolbar.widget->SetStyle("Toolbar", ui_inf.style);

    mp->toolbar.add_goal = new urho::Button(uctxt);
    mp->toolbar.widget->AddChild(mp->toolbar.add_goal);
    mp->toolbar.add_goal->SetStyle("AddGoalButton", ui_inf.style);
    auto offset = mp->toolbar.add_goal->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_TOOLBAR_SCALING;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_TOOLBAR_SCALING;
    mp->toolbar.add_goal->SetMaxOffset(offset);

    mp->toolbar.cancel_goal = new urho::Button(uctxt);
    mp->toolbar.widget->AddChild(mp->toolbar.cancel_goal);
    mp->toolbar.cancel_goal->SetStyle("CancelGoalsButton", ui_inf.style);
    offset = mp->toolbar.cancel_goal->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_TOOLBAR_SCALING;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_TOOLBAR_SCALING;
    mp->toolbar.cancel_goal->SetMaxOffset(offset);

    mp->toolbar.clear_maps = new urho::Button(uctxt);
    mp->toolbar.widget->AddChild(mp->toolbar.clear_maps);
    mp->toolbar.clear_maps->SetStyle("ClearMapsButton", ui_inf.style);
    offset = mp->toolbar.clear_maps->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_TOOLBAR_SCALING;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_TOOLBAR_SCALING;
    mp->toolbar.clear_maps->SetMaxOffset(offset);
    mp->toolbar.clear_maps->SetMinOffset({});

    mp->toolbar.set_params = new urho::Button(uctxt);
    mp->toolbar.widget->AddChild(mp->toolbar.set_params);
    mp->toolbar.set_params->SetStyle("SetParamsButton", ui_inf.style);
    offset = mp->toolbar.set_params->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_TOOLBAR_SCALING;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_TOOLBAR_SCALING;
    mp->toolbar.set_params->SetMaxOffset(offset);

    ivec2 tb_offset = {offset.x_, 0};
    for (int i = 0; i < mp->toolbar.widget->GetNumChildren(); ++i)
    {
        auto child = mp->toolbar.widget->GetChild(i);
        tb_offset.y_ += child->GetMaxOffset().y_ + 10;
    }
    mp->toolbar.widget->SetMaxOffset(tb_offset);
}

void toolbar_init(map_panel *mp, const ui_info &ui_inf)
{
    ilog("Initializing toolbar");
    setup_toolbar_widget(mp, ui_inf);
}

void toolbar_term(map_panel *mp)
{
    ilog("Terminating toolbar");
}

void toolbar_handle_click_end(map_panel *mp, urho::UIElement *elem, net_connection *conn)
{
    if (elem == mp->toolbar.add_goal)
    {
        auto r = mp->toolbar.add_goal->GetImageRect() + irect{128, 0, 128, 0};
        mp->toolbar.add_goal->SetImageRect(r);
        mp->toolbar.add_goal->SetEnabled(false);
    }
    else if (elem == mp->toolbar.cancel_goal)
    {
        if (!mp->toolbar.add_goal->IsEnabled())
        {
            auto r = mp->toolbar.add_goal->GetImageRect() + irect{-128, 0, -128, 0};
            mp->toolbar.add_goal->SetImageRect(r);
            mp->toolbar.add_goal->SetEnabled(true);
        }
        command_stop stop{};
        mp->goals.queued_goals.clear();
        net_tx(*conn, stop);
    }
    else if (elem == mp->toolbar.clear_maps)
    {
        if (!mp->toolbar.add_goal->IsEnabled())
        {
            auto r = mp->toolbar.add_goal->GetImageRect() + irect{-128, 0, -128, 0};
            mp->toolbar.add_goal->SetImageRect(r);
            mp->toolbar.add_goal->SetEnabled(true);
        }
        command_clear_maps cm{};
        net_tx(*conn, cm);
    }
    else if (elem == mp->toolbar.set_params)
    {
        param_toggle_input_visible(mp);
    }
}