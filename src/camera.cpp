#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/View3D.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include "Urho3D/Scene/Node.h"

#include "camera.h"
#include "input.h"

#include "mapping.h"

intern const float UI_ADDITIONAL_CAM_SCALING = 0.75f;

intern void setup_camera_controls(map_panel *mp, urho::Node *cam_node, input_data *inp)
{
    auto on_mouse_tilt = [cam_node, mp](const itrigger_event &tevent) {
        if (!mp->toolbar.add_goal->IsEnabled() || mp->js_enabled || (mp->view != tevent.vp.element_under_mouse && tevent.vp.element_under_mouse->GetPriority() > 2))
            return;

        auto rot_node = cam_node;
        auto parent = cam_node->GetParent();
        if (parent)
            rot_node = parent;
        if (tevent.vp.vp_norm_mdelta.x_ > 0.1 || tevent.vp.vp_norm_mdelta.y_ > 0.1)
            return;
        rot_node->Rotate(quat(tevent.vp.vp_norm_mdelta.y_ * 100.0f, {1, 0, 0}));
        rot_node->Rotate(quat(tevent.vp.vp_norm_mdelta.x_ * 100.0f, {0, 0, -1}), urho::TransformSpace::World);
    };

    input_action_trigger it{};
    it.cond.mbutton = MOUSEB_MOVE;
    it.name = urho::StringHash("CamTiltAction").ToHash();
    it.tstate = T_BEGIN;
    it.mb_req = urho::MOUSEB_LEFT;
    it.mb_allowed = 0;
    it.qual_req = 0;
    it.qual_allowed = urho::QUAL_ANY;
    input_add_trigger(&inp->map, it);
    ss_connect(&mp->router, inp->dispatch.trigger, it.name, on_mouse_tilt);
}

intern void setup_cam_zoom_widget(map_panel *mp, const ui_info &ui_inf)
{
    auto uctxt = ui_inf.ui_sys->GetContext();

    mp->cam_cwidget.cam_zoom_widget.widget = new urho::BorderImage(uctxt);
    mp->cam_cwidget.root_element->AddChild(mp->cam_cwidget.cam_zoom_widget.widget);
    mp->cam_cwidget.cam_zoom_widget.widget->SetStyle("ZoomButtonGroup", ui_inf.style);

    mp->cam_cwidget.cam_zoom_widget.zoom_in = new urho::Button(uctxt);
    mp->cam_cwidget.cam_zoom_widget.widget->AddChild(mp->cam_cwidget.cam_zoom_widget.zoom_in);
    mp->cam_cwidget.cam_zoom_widget.zoom_in->SetStyle("ZoomInButton", ui_inf.style);

    auto offset = mp->cam_cwidget.cam_zoom_widget.zoom_in->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_CAM_SCALING;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_CAM_SCALING;
    auto parent_offset = offset;
    parent_offset.y_ = offset.y_ * 2;
    mp->cam_cwidget.cam_zoom_widget.widget->SetMaxOffset(parent_offset);
    mp->cam_cwidget.cam_zoom_widget.zoom_in->SetMaxOffset(offset);

    mp->cam_cwidget.cam_zoom_widget.zoom_out = new urho::Button(uctxt);
    mp->cam_cwidget.cam_zoom_widget.widget->AddChild(mp->cam_cwidget.cam_zoom_widget.zoom_out);
    mp->cam_cwidget.cam_zoom_widget.zoom_out->SetStyle("ZoomOutButton", ui_inf.style);
    mp->cam_cwidget.cam_zoom_widget.zoom_out->SetMaxOffset(offset);
}

intern void setup_cam_move_widget(map_panel *mp, const ui_info &ui_inf)
{
    auto uctxt = ui_inf.ui_sys->GetContext();

    mp->cam_cwidget.cam_move_widget.widget = new urho::BorderImage(uctxt);
    mp->cam_cwidget.root_element->AddChild(mp->cam_cwidget.cam_move_widget.widget);
    mp->cam_cwidget.cam_move_widget.widget->SetStyle("ArrowButtonGroup", ui_inf.style);

    mp->cam_cwidget.cam_move_widget.forward = new urho::Button(uctxt);
    mp->cam_cwidget.cam_move_widget.widget->AddChild(mp->cam_cwidget.cam_move_widget.forward);
    mp->cam_cwidget.cam_move_widget.forward->SetStyle("ArrowButtonForward", ui_inf.style);
    mp->cam_cwidget.cam_move_widget.forward->SetVar("md", 1);

    auto offset = mp->cam_cwidget.cam_move_widget.forward->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_CAM_SCALING;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_CAM_SCALING;
    auto parent_offset = offset;
    parent_offset.x_ = offset.x_ * 3;
    parent_offset.y_ = offset.y_ * 2;
    mp->cam_cwidget.cam_move_widget.widget->SetMaxOffset(parent_offset);
    mp->cam_cwidget.cam_move_widget.forward->SetMaxOffset(offset);

    mp->cam_cwidget.cam_move_widget.back = new urho::Button(uctxt);
    mp->cam_cwidget.cam_move_widget.widget->AddChild(mp->cam_cwidget.cam_move_widget.back);
    mp->cam_cwidget.cam_move_widget.back->SetStyle("ArrowButtonBack", ui_inf.style);
    mp->cam_cwidget.cam_move_widget.back->SetMaxOffset(offset);
    mp->cam_cwidget.cam_move_widget.back->SetVar("md", -1);

    mp->cam_cwidget.cam_move_widget.left = new urho::Button(uctxt);
    mp->cam_cwidget.cam_move_widget.widget->AddChild(mp->cam_cwidget.cam_move_widget.left);
    mp->cam_cwidget.cam_move_widget.left->SetStyle("ArrowButtonLeft", ui_inf.style);
    mp->cam_cwidget.cam_move_widget.left->SetMaxOffset(offset);
    mp->cam_cwidget.cam_move_widget.left->SetVar("md", -2);

    mp->cam_cwidget.cam_move_widget.right = new urho::Button(uctxt);
    mp->cam_cwidget.cam_move_widget.widget->AddChild(mp->cam_cwidget.cam_move_widget.right);
    mp->cam_cwidget.cam_move_widget.right->SetStyle("ArrowButtonRight", ui_inf.style);
    mp->cam_cwidget.cam_move_widget.right->SetMaxOffset(offset);
    mp->cam_cwidget.cam_move_widget.right->SetVar("md", 2);
}

intern void handle_press_event(map_panel *mp, urho::UIElement *elem)
{
    auto trans = elem->GetVar("md").GetInt();

    if (elem == mp->cam_cwidget.cam_zoom_widget.zoom_in)
        mp->cam_cwidget.cam_zoom_widget.loc_trans = mp->view->GetCameraNode()->GetDirection();
    else if (elem == mp->cam_cwidget.cam_zoom_widget.zoom_out)
        mp->cam_cwidget.cam_zoom_widget.loc_trans = mp->view->GetCameraNode()->GetDirection() * -1;
    else if (trans == 0)
        return;

    auto cam_node = mp->view->GetCameraNode();
    auto world_right = cam_node->GetWorldRight().Normalized();
    auto world_up = cam_node->GetWorldUp().Normalized();
    float angle = std::abs(world_up.Angle({0, 0, 1}) - 90.0f);
    if (std::abs(angle) > 70)
    {
        mp->cam_cwidget.cam_move_widget.world_trans = vec3(0, 0, -trans);
        if (std::abs(trans) == 2)
            mp->cam_cwidget.cam_move_widget.world_trans = world_right * trans / 2;
    }
    else
    {
        mp->cam_cwidget.cam_move_widget.world_trans = vec3(world_up.x_, world_up.y_, 0.0) * trans;
        if (std::abs(trans) == 2)
            mp->cam_cwidget.cam_move_widget.world_trans = world_right * trans / 2;
    }
}

intern void cam_run_frame(map_panel *mp, float dt)
{
    if (mp->cam_cwidget.cam_move_widget.world_trans != vec3::ZERO)
    {
        mp->view->GetCameraNode()->Translate(mp->cam_cwidget.cam_move_widget.world_trans * dt * 10, Urho3D::TransformSpace::World);
    }

    if (mp->cam_cwidget.cam_zoom_widget.loc_trans != vec3::ZERO)
    {
        mp->view->GetCameraNode()->Translate(mp->cam_cwidget.cam_zoom_widget.loc_trans * dt * 20);
    }
}

void cam_handle_click_end(map_panel *mp, urho::UIElement * elem)
{
    mp->cam_cwidget.cam_zoom_widget.loc_trans = {};
    mp->cam_cwidget.cam_move_widget.world_trans = {};
}

void cam_init(map_panel *mp, input_data *inp, const ui_info &ui_inf)
{
    auto uctxt = ui_inf.ui_sys->GetContext();
    auto cam_node = mp->view->GetCameraNode();

    ilog("Initializing camera");

    setup_camera_controls(mp, cam_node, inp);

    mp->cam_cwidget.root_element = new urho::BorderImage(uctxt);
    ui_inf.ui_sys->GetRoot()->AddChild(mp->cam_cwidget.root_element);
    mp->cam_cwidget.root_element->SetStyle("CamControlButtonGroup", ui_inf.style);

    setup_cam_move_widget(mp, ui_inf);
    setup_cam_zoom_widget(mp, ui_inf);

    ivec2 child_offsets = {};
    for (int i = 0; i < mp->cam_cwidget.root_element->GetNumChildren(); ++i)
    {
        auto child = mp->cam_cwidget.root_element->GetChild(i);
        auto coffset = child->GetMaxOffset();
        child_offsets.x_ += coffset.x_ + 20;
        child_offsets.y_ = coffset.y_;
    }

    if (mp->cam_cwidget.root_element->GetMinAnchor() == mp->cam_cwidget.root_element->GetMaxAnchor())
        mp->cam_cwidget.root_element->SetMaxOffset(child_offsets);

    cam_node->SubscribeToEvent(urho::E_UPDATE, [mp](urho::StringHash type, urho::VariantMap &ev_data) {
        auto dt = ev_data[urho::Update::P_TIMESTEP].GetFloat();
        cam_run_frame(mp, dt);
    });

    mp->cam_cwidget.cam_move_widget.widget->SubscribeToEvent(urho::E_PRESSED, [mp](urho::StringHash type, urho::VariantMap &ev_data) {
        auto elem = (urho::UIElement *)ev_data[urho::ClickEnd::P_ELEMENT].GetPtr();
        handle_press_event(mp, elem);
    });
}

void cam_term(map_panel *mp)
{
    ilog("Terminating camera");
}