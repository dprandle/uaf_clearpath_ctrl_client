#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Timer.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/View3D.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

#include "toolbar.h"
#include "mapping.h"
#include "network.h"

intern const float UI_ADDITIONAL_TOOLBAR_SCALING = 1.00f;

template<class T>
intern void add_toolbar_button(T ** btn, urho::UIElement * parent, urho::Context * uctxt, const urho::String &style, const ui_info &ui_inf)
{
    *btn = parent->CreateChild<T>();
    (*btn)->SetStyle(style, ui_inf.style);
    auto offset = (*btn)->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_TOOLBAR_SCALING;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_TOOLBAR_SCALING;
    (*btn)->SetMaxOffset(offset);
}

void toolbar_init(map_panel *mp, const ui_info &ui_inf, bool can_control)
{
    ilog("Initializing toolbar");
    auto uctxt = ui_inf.ui_sys->GetContext();

    mp->toolbar.widget = new urho::BorderImage(uctxt);
    ui_inf.ui_sys->GetRoot()->AddChild(mp->toolbar.widget);
    mp->toolbar.widget->SetStyle("Toolbar", ui_inf.style);

    add_toolbar_button(&mp->toolbar.enable_follow, mp->toolbar.widget, uctxt, "EnableFollow", ui_inf);

    if (can_control)
    {
        add_toolbar_button(&mp->toolbar.add_goal, mp->toolbar.widget, uctxt, "AddGoalButton", ui_inf);
        add_toolbar_button(&mp->toolbar.cancel_goal, mp->toolbar.widget, uctxt, "CancelGoalsButton", ui_inf);
        add_toolbar_button(&mp->toolbar.clear_maps, mp->toolbar.widget, uctxt, "ClearMapsButton", ui_inf);
        add_toolbar_button(&mp->toolbar.set_params, mp->toolbar.widget, uctxt, "SetParamsButton", ui_inf);
    }

    add_toolbar_button(&mp->toolbar.enable_measure, mp->toolbar.widget, uctxt, "EnableMeasure", ui_inf);
    add_toolbar_button(&mp->toolbar.delete_measure, mp->toolbar.widget, uctxt, "DeleteMeasure", ui_inf);

    ivec2 tb_offset = {mp->toolbar.enable_follow->GetMaxOffset().x_, 0};
    float anchor_spacing = 1.0f / mp->toolbar.widget->GetNumChildren();
    for (int i = 0; i < mp->toolbar.widget->GetNumChildren(); ++i)
    {
        auto child = mp->toolbar.widget->GetChild(i);
        child->SetMinAnchor(0, i*anchor_spacing);
        child->SetMaxAnchor(0, i*anchor_spacing);
        tb_offset.y_ += child->GetMaxOffset().y_ + 10;
        ilog("Addijng offset for %s of %d (total now %d)", child->GetName().CString(), child->GetMaxOffset().y_ + 10, tb_offset.y_);
    }
    mp->toolbar.widget->SetMaxOffset(tb_offset);
}

void toolbar_term(map_panel *mp)
{
    ilog("Terminating toolbar");
}

void toolbar_handle_toggle(map_panel *mp, urho::UIElement *elem)
{
    auto ctxt = elem->GetContext();
    auto time = ctxt->GetSubsystem<urho::Time>();

    if (elem == mp->toolbar.add_goal)
    {
        mp->toolbar.last_frame_checked = time->GetFrameNumber();
        if (mp->toolbar.add_goal->IsChecked())
            mp->toolbar.enable_measure->SetChecked(false);
    }
    else if (elem == mp->toolbar.set_params)
    {
        param_toggle_input_visible(mp);
    }
    else if (elem == mp->toolbar.enable_follow)
    {

        auto cam_node = mp->view->GetCameraNode();
        if (mp->toolbar.enable_follow->IsChecked())
        {
            cam_node->SetParent(mp->base_link->GetChild("jackal_follow_cam"));
            auto cam_pos = cam_node->GetPosition();
            cam_pos.x_ = 0.0f;
            cam_pos.y_ = 0.0f;
            cam_node->SetPosition(cam_pos);
            cam_node->SetRotation({});
        }
        else
        {
            cam_node->SetParent(mp->view->GetScene());
        }
    }
    else if (elem == mp->toolbar.enable_measure)
    {
        mp->toolbar.last_frame_checked = time->GetFrameNumber();
        if (mp->toolbar.enable_measure->IsChecked())
        {
            mp->toolbar.add_goal->SetChecked(false);
        }
    }
}

void toolbar_handle_mouse_released(map_panel *mp, urho::UIElement *elem, net_connection *conn)
{    
    if (elem == mp->toolbar.cancel_goal)
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
    else if (elem == mp->toolbar.delete_measure)
    {
        if (mp->mpoints.entry_count > 2)
            --mp->mpoints.entry_count;
        else
            mp->mpoints.entry_count = 0;
    }
}