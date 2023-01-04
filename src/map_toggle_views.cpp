#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/View3D.h>

#include <Urho3D/Scene/Node.h>

#include "map_toggle_views.h"
#include "mapping.h"

intern void run_frame(map_panel *mp, float dt, const ui_info &ui_inf)
{
    animated_panel_run_frame(&mp->views_panel.apanel, dt, ui_inf, "ViewToggle");
}

intern vec2 get_required_panel_dims(map_toggle_views_panel *vp)
{
    vec2 ret{};
    for (int i = 0; i < vp->elems.size(); ++i)
    {
        auto sz = vp->elems[i].widget->GetSize();
        if (sz.x_ > ret.x_)
            ret.x_ = sz.x_;
        ret.y_ += sz.y_;
    }
    return ret;
}

intern check_box_text_element
create_cbox_item(urho::ListView *lview, urho::Node *node, nav_path_view *npview, const urho::String &text, urho::Context *uctxt, const ui_info &ui_inf)
{
    check_box_text_element ret;
    ret.widget = new urho::UIElement(uctxt);
    ret.widget->SetStyle("MapCheckBoxRoot", ui_inf.style);
    vec4 r = vec4{20.0f, 10.0f, 20.0f, 10.0f}*ui_inf.dev_pixel_ratio_inv;
    ret.widget->SetLayoutBorder({(int)r.x_, (int)r.y_, (int)r.z_, (int)r.w_});
    ret.widget->SetLayoutSpacing(24.0f*ui_inf.dev_pixel_ratio_inv);

    ret.cb = ret.widget->CreateChild<urho::CheckBox>();
    ret.cb->SetStyle("MapCheckBox", ui_inf.style);
    auto sz = vec2{64, 64}*ui_inf.dev_pixel_ratio_inv;
    ivec2 szi = {(int)sz.x_, (int)sz.y_};
    ret.cb->SetMinSize(szi);
    ret.cb->SetMaxSize(szi);

    ret.txt = ret.widget->CreateChild<urho::Text>();
    ret.txt->SetStyle("MapCheckBoxText", ui_inf.style);
    ret.txt->SetFontSize(32 * ui_inf.dev_pixel_ratio_inv);
    ret.txt->SetText(text);

    ret.node = node;
    ret.npview = npview;
    lview->AddItem(ret.widget);
    return ret;
}

intern void setup_view_checkboxes(map_panel *mp, const ui_info &ui_inf)
{
    static ivec2 cb_size = {64, 64};
    auto vp = &mp->views_panel;
    auto uctxt = ui_inf.ui_sys->GetContext();

    vp->apanel.widget = ui_inf.ui_sys->GetRoot()->CreateChild<urho::UIElement>();
    vp->apanel.widget->SetStyle("ViewToggleAnimatedPanel", ui_inf.style);

    vp->apanel.sview = vp->apanel.widget->CreateChild<urho::ListView>();
    vp->apanel.sview->SetStyle("AnimatedPanelListView", ui_inf.style);

    vp->apanel.hide_show_btn_bg = vp->apanel.widget->CreateChild<urho::BorderImage>();
    vp->apanel.hide_show_btn_bg->SetStyle("ViewToggleHideShowBG", ui_inf.style);

    vp->apanel.hide_show_panel = vp->apanel.hide_show_btn_bg->CreateChild<urho::Button>();
    vp->apanel.hide_show_panel->SetStyle("ViewToggleShow", ui_inf.style);
    auto offset = vp->apanel.hide_show_panel->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv;
    vp->apanel.hide_show_panel->SetMaxOffset(offset);
    vp->apanel.hide_show_btn_bg->SetMaxOffset(offset);
    vp->apanel.anim_dir = PANEL_ANIM_HORIZONTAL;

    vp->apanel.anchor_rest_point = 1.0f;

    vp->elems.emplace_back(create_cbox_item(vp->apanel.sview, mp->map.node, nullptr, "Map", uctxt, ui_inf));

    // Set the first item to have a top border that's double so the spacing matches the in between spacing
    auto rect = vp->elems.back().widget->GetLayoutBorder();
    rect.top_ *= 2;
    vp->elems.back().widget->SetLayoutBorder(rect);

    vp->elems.emplace_back(create_cbox_item(vp->apanel.sview, mp->front_laser, nullptr, "Laser Scan", uctxt, ui_inf));
    vp->elems.emplace_back(create_cbox_item(vp->apanel.sview, mp->glob_cmap.node, nullptr, "Global Costmap", uctxt, ui_inf));
    vp->elems.emplace_back(create_cbox_item(vp->apanel.sview, mp->loc_cmap.node, nullptr, "Local Costmap", uctxt, ui_inf));
    vp->elems.emplace_back(create_cbox_item(vp->apanel.sview, nullptr, &mp->glob_npview, "Global Nav Path", uctxt, ui_inf));
    vp->elems.emplace_back(create_cbox_item(vp->apanel.sview, nullptr, &mp->loc_npview, "Local Nav Path", uctxt, ui_inf));

    // Set the last item to have a bottom border that's double so the spacing matches the in between spacing
    rect = vp->elems.back().widget->GetLayoutBorder();
    rect.bottom_ *= 2;
    vp->elems.back().widget->SetLayoutBorder(rect);
    map_toggle_views_handle_resize(mp, ui_inf);
}

void map_toggle_views_handle_resize(map_panel *mp, const ui_info &ui_inf)
{
    auto vp = &mp->views_panel;
    vec2 needed_panel_size = get_required_panel_dims(vp);
    auto screen_dims = mp->view->GetSize();
    vec2 normalized_size = needed_panel_size / (vec2{screen_dims.x_, screen_dims.y_});
    vp->apanel.anchor_set_point = 1.0 - normalized_size.x_;
    vp->apanel.widget->SetMaxAnchor(1.0f, vp->apanel.widget->GetMinAnchor().y_ + normalized_size.y_);
}

void map_toggle_views_init(map_panel *mp, const ui_info &ui_inf)
{
    ilog("Initializing map toggle views");

    setup_view_checkboxes(mp, ui_inf);

    mp->views_panel.apanel.widget->SubscribeToEvent(urho::E_UPDATE, [mp, ui_inf](urho::StringHash type, urho::VariantMap &ev_data) {
        auto dt = ev_data[urho::Update::P_TIMESTEP].GetFloat();
        run_frame(mp, dt, ui_inf);
    });
}

void map_toggle_views_term(map_panel *mp)
{
    ilog("Terminating map toggle views");
}

void map_toggle_views_handle_toggle(map_panel *mp, urho::UIElement *elem)
{
    for (int i = 0; i < mp->views_panel.elems.size(); ++i)
    {
        auto cur_elem = &mp->views_panel.elems[i];
        if (elem == cur_elem->cb)
        {
            if (cur_elem->node)
                cur_elem->node->SetEnabled(cur_elem->cb->IsChecked());
            else if (cur_elem->npview)
                cur_elem->npview->enabled = cur_elem->cb->IsChecked();
        }
    }
}

void map_toggle_views_handle_mouse_released(map_panel *mp, urho::UIElement *elem)
{
    if (elem == mp->views_panel.apanel.hide_show_panel)
    {
        animated_panel_hide_show_pressed(&mp->views_panel.apanel);
    }
}