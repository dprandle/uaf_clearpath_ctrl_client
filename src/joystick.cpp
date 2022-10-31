#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Core/CoreEvents.h>

#include "jackal_control.h"
#include "joystick.h"
#include "network.h"

intern void handle_jostick_move(const ivec2 &cur_mpos, joystick_panel *jspanel, float dev_pixel_ratio_inv)
{
    float max_r = jspanel->outer_ring->GetMaxOffset().x_ / 3.0f;
    vec2 dir_vec = vec2{cur_mpos.x_, cur_mpos.y_} - jspanel->cached_mouse_pos;
    auto l = dir_vec.Length();
    if (l > max_r)
        dir_vec = dir_vec * (max_r / l);
    ivec2 new_pos = jspanel->cached_js_pos + ivec2(dir_vec.x_, dir_vec.y_);
    jspanel->js->SetPosition(new_pos);
    jspanel->velocity = dir_vec * (-1.0f / max_r);
}

intern void handle_update(joystick_panel *jsp, net_connection *conn)
{
    command_velocity packet;
    packet.vinfo.angular = jsp->velocity.x_;
    packet.vinfo.linear = jsp->velocity.y_;
    net_tx(*conn, packet);
}

intern void handle_joystick_move_begin(joystick_panel *jsp, const ui_info &ui_inf)
{
    ivec2 pos = jsp->js->GetPosition();
    jsp->cached_js_pos = pos;
    ivec2 cmp;
    SDL_GetMouseState(&cmp.x_, &cmp.y_);
    jsp->cached_mouse_pos = vec2{cmp.x_, cmp.y_} * ui_inf.dev_pixel_ratio_inv;
    jsp->js->SetEnableAnchor(false);
}

intern void handle_joystick_move_end(joystick_panel *jsp)
{
    jsp->js->SetEnableAnchor(true);
    jsp->cached_js_pos = {};
    jsp->cached_mouse_pos = {};
}

void joystick_panel_init(joystick_panel *jsp, const ui_info &ui_inf, net_connection *conn)
{
    auto uctxt = ui_inf.ui_sys->GetContext();

    jsp->frame = new urho::BorderImage(uctxt);
    jsp->outer_ring = new urho::BorderImage(uctxt);
    jsp->js = new urho::Button(uctxt);

    ui_inf.ui_sys->GetRoot()->AddChild(jsp->frame);
    jsp->frame->SetColor({0.2f, 0.2f, 0.2f, 0.7f});
    jsp->frame->SetEnableAnchor(true);
    jsp->frame->SetMinAnchor(0.0f, 0.7f);
    jsp->frame->SetMaxAnchor(1.0f, 1.0f);

    jsp->frame->AddChild(jsp->outer_ring);
    jsp->outer_ring->SetStyle("JoystickBorder", ui_inf.style);
    jsp->outer_ring->SetEnableAnchor(true);
    jsp->outer_ring->SetMinAnchor(0.5f, 0.5f);
    jsp->outer_ring->SetMaxAnchor(0.5f, 0.5f);

    ivec2 offset = jsp->outer_ring->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv;

    jsp->outer_ring->SetMaxOffset(offset);
    jsp->outer_ring->SetPivot(0.5f, 0.5f);

    jsp->outer_ring->AddChild(jsp->js);
    jsp->js->SetStyle("Joystick", ui_inf.style);
    jsp->js->SetEnableAnchor(true);
    jsp->js->SetMinOffset({0, 0});

    offset = jsp->js->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv;

    jsp->js->SetMaxOffset(offset);
    jsp->js->SetMinAnchor(0.5f, 0.5f);
    jsp->js->SetMaxAnchor(0.5f, 0.5f);
    jsp->js->SetPivot(0.5f, 0.5f);

    jsp->js->SubscribeToEvent(urho::E_DRAGMOVE, [jsp, ui_inf, conn](urho::StringHash type, urho::VariantMap &ev_data) {
        using namespace urho::DragMove;
        auto elem = (urho::UIElement *)ev_data[P_ELEMENT].GetPtr();
        if (elem == jsp->js)
            handle_jostick_move({ev_data[P_X].GetInt(), ev_data[P_Y].GetInt()}, jsp, ui_inf.dev_pixel_ratio_inv);
    });

    jsp->js->SubscribeToEvent(urho::E_DRAGEND, [jsp](urho::StringHash type, urho::VariantMap &ev_data) {
        using namespace urho::DragEnd;
        auto elem = (urho::UIElement *)ev_data[P_ELEMENT].GetPtr();
        if (elem == jsp->js)
            handle_joystick_move_end(jsp);
    });

    jsp->js->SubscribeToEvent(urho::E_DRAGBEGIN, [jsp, ui_inf](urho::StringHash type, urho::VariantMap &ev_data) {
        using namespace urho::DragBegin;
        auto elem = (urho::UIElement *)ev_data[P_ELEMENT].GetPtr();
        if (elem == jsp->js)
            handle_joystick_move_begin(jsp, ui_inf);
    });

    jsp->js->SubscribeToEvent(urho::E_UPDATE, [jsp, conn](urho::StringHash type, urho::VariantMap &ev_data) {
        joystick_panel_run_frame(jsp, conn);
    });
}

void joystick_panel_term(joystick_panel *jsp)
{
    jsp->js->UnsubscribeFromAllEvents();
    jsp->cached_js_pos = {};
    jsp->cached_mouse_pos = {};
}

void joystick_panel_run_frame(joystick_panel*jspanel, net_connection * conn)
{
    if (!jspanel->js->GetEnableAnchor())
    {
        command_velocity pckt;
        pckt.vinfo.linear = jspanel->velocity.y_;
        pckt.vinfo.angular = jspanel->velocity.x_;
        net_tx(*conn, pckt);
    }
}
