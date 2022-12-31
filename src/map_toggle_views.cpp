#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/Button.h>

#include "map_toggle_views.h"
#include "mapping.h"

intern void run_frame(map_panel *mp, float dt, const ui_info &ui_inf)
{
    animated_panel_run_frame(&mp->views_panel.apanel, dt, ui_inf, "ViewToggle");
}

intern void setup_view_checkboxes(map_panel * mp, const ui_info & ui_inf)
{
    auto uctxt = ui_inf.ui_sys->GetContext();

    mp->views_panel.apanel.widget = ui_inf.ui_sys->GetRoot()->CreateChild<urho::UIElement>();
    mp->views_panel.apanel.widget->SetStyle("ViewToggleAnimatedPanel", ui_inf.style);

    mp->views_panel.apanel.sview = mp->views_panel.apanel.widget->CreateChild<urho::ListView>();
    mp->views_panel.apanel.sview->SetStyle("AnimatedPanelListView", ui_inf.style);

    mp->views_panel.apanel.hide_show_btn_bg = mp->views_panel.apanel.widget->CreateChild<urho::BorderImage>();
    mp->views_panel.apanel.hide_show_btn_bg->SetStyle("ViewToggleHideShowBG", ui_inf.style);

    mp->views_panel.apanel.hide_show_panel = mp->views_panel.apanel.hide_show_btn_bg->CreateChild<urho::Button>();
    mp->views_panel.apanel.hide_show_panel->SetStyle("ViewToggleShow", ui_inf.style);
    auto offset = mp->views_panel.apanel.hide_show_panel->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv;
    mp->views_panel.apanel.hide_show_panel->SetMaxOffset(offset);
    mp->views_panel.apanel.hide_show_btn_bg->SetMaxOffset(offset);
    mp->views_panel.apanel.anim_dir = PANEL_ANIM_HORIZONTAL;

    mp->views_panel.apanel.anchor_set_point = 0.6f;
    mp->views_panel.apanel.anchor_rest_point = mp->views_panel.apanel.widget->GetMaxAnchor().x_;
}

void map_toggle_views_init(map_panel * mp, const ui_info & ui_inf)
{
    ilog("Initializing map toggle views");

    setup_view_checkboxes(mp, ui_inf);

    mp->views_panel.apanel.widget->SubscribeToEvent(urho::E_UPDATE, [mp, ui_inf](urho::StringHash type, urho::VariantMap &ev_data) {
        auto dt = ev_data[urho::Update::P_TIMESTEP].GetFloat();
        run_frame(mp, dt, ui_inf);
    });
}

void map_toggle_views_term(map_panel * mp)
{
    ilog("Terminating map toggle views");
}

void map_toggle_views_handle_click_end(map_panel *mp, urho::UIElement * elem)
{
    if (elem == mp->views_panel.apanel.hide_show_panel)
    {
        animated_panel_hide_show_pressed(&mp->views_panel.apanel);
    }
}