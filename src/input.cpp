#include "Urho3D/Base/PrimitiveTypes.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Variant.h"
#include "Urho3D/Math/StringHash.h"
#include "math_utils.h"

#include <Urho3D/GraphicsAPI/RenderSurface.h>
#include <Urho3D/Graphics/View.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Input/Input.h>
#include "input.h"

itrigger_range input_find_triggers(input_keymap *kmap, const input_trigger_cond &cond)
{
    return kmap->tmap.equal_range(cond.lookup_key);
}

void input_add_trigger(input_keymap *kmap, const input_action_trigger &trigger)
{
    kmap->tmap.emplace(trigger.cond.lookup_key, trigger);
}

bool input_remove_trigger(input_keymap *kmap, const input_action_trigger &trigger)
{
    auto range = input_find_triggers(kmap, trigger.cond);
    while (range.first != range.second)
    {
        if (range.first->second == trigger)
        {
            kmap->tmap.erase(range.first);
            return true;
        }
        ++range.first;
    }
    return false;
}

int input_remove_triggers(input_keymap *kmap, const input_trigger_cond &cond)
{
    return kmap->tmap.erase(cond.lookup_key);
}

intern viewport_info viewport_with_mouse(const std::vector<viewport_element> &vp_stack, const ivec2 &screen_size, const vec2 &norm_mpos, const vec2 &norm_mdelta, double inv_pixel_ratio)
{
    viewport_info ret{};

    vec2 sz_f(screen_size.x_, screen_size.y_);

    // Go through viewport stack and try to find one that has our mouse pointer within
    for (int i = 0; i < vp_stack.size(); ++i)
    {
        viewport_element vp_desc{vp_stack[i].vp, vp_stack[i].view_element};

        irect scrn = vp_desc.vp->GetView()->GetViewRect();
        ivec2 min = scrn.Min();
        ivec2 max = scrn.Max();

        if (vp_desc.view_element)
        {
            min = vp_desc.view_element->GetScreenPosition();
            max = max + min;
        }

        vec2 ll(min.x_, min.y_);
        vec2 ur(max.x_, max.y_);
        vec2 mpos = norm_mpos * sz_f * inv_pixel_ratio;
        vec2 mdelta = norm_mdelta * sz_f * inv_pixel_ratio;

        if (scrn == irect())
            ur = sz_f;

        // Return the first viewport that contains the mouse click!
        rect fr(ll, ur);
        if (fr.IsInside(mpos) == urho::Intersection::INSIDE)
        {
            ret.vp_desc = vp_desc;
            ret.vp_norm_mpos = (mpos - ll) / fr.Size();
            ret.vp_norm_mdelta = mdelta / fr.Size();
            return ret;
        }
    }
    return ret;
}

intern bool trigger_already_active(const std::vector<input_action_trigger> &active_triggers, const input_action_trigger &trig)
{
    auto iter = active_triggers.begin();
    while (iter != active_triggers.end())
    {
        if (trig == *iter)
            return true;
        ++iter;
    }
    return false;
}

intern void on_key_down(input_uevent_handlers *uh, Urho3D::StringHash event_type, Urho3D::VariantMap &event_data)
{
    input_trigger_cond tc;
    tc.key = i32(event_data["Key"].GetInt());
    tc.mbutton = 0;
    i32 qualifiers(event_data["Qualifiers"].GetInt());
    i32 mouse_buttons(event_data["Buttons"].GetInt());
    ivec2 screen_size = uh->GetSubsystem<Urho3D::Graphics>()->GetSize();

    for (int i = uh->ictxt->context_stack.size(); i > 0; --i)
    {
        input_keymap *cur_km = uh->ictxt->context_stack[i - 1];
        input_action_trigger *trig = nullptr;

        // Go through every trigger - if the current mb and quals are contained in the required qual and mb fields
        // and the allowed
        // First find all triggers with the exact match for key and mouse qualifiers
        auto tr = input_find_triggers(cur_km, tc);
        while (tr.first != tr.second)
        {
            const input_action_trigger &trig = tr.first->second;

            // Check the qualifier and mouse button required conditions
            bool pass_qual_required((trig.qual_req & qualifiers) == trig.qual_req);
            bool pass_mb_required((trig.mb_req & mouse_buttons) == trig.mb_req);

            // Check the qualifier and mouse button allowed conditions
            i32 allowed_quals = trig.qual_req | trig.qual_allowed;
            i32 allowed_mb = trig.mb_req | trig.mb_allowed;
            bool pass_qual_allowed(((allowed_quals & urho::QUAL_ANY) == urho::QUAL_ANY) || (qualifiers | allowed_quals) == allowed_quals);
            bool pass_mb_allowed((mouse_buttons | allowed_mb) == allowed_mb);

            // If passes all the conditions, send the event for the trigger and mark trigger as active
            if (!trigger_already_active(uh->ictxt->active_triggers, trig) && pass_qual_required && pass_mb_required && pass_qual_allowed && pass_mb_allowed)
            {
                if ((trig.tstate & T_BEGIN) == T_BEGIN)
                {
                    itrigger_event ev{trig.name,
                                      T_BEGIN,
                                      uh->ictxt->current_norm_mpos,
                                      {},
                                      {},
                                      viewport_with_mouse(uh->ictxt->vp_stack, screen_size, uh->ictxt->current_norm_mpos, {}, uh->ictxt->inv_pixel_ratio)};
                    uh->ictxt->trigger(trig.name, ev);
                }
                uh->ictxt->active_triggers.push_back(trig);
            }

            ++tr.first;
        }
    }
}

intern void on_key_up(input_uevent_handlers *uh, urho::StringHash event_type, urho::VariantMap &event_data)
{
    input_trigger_cond tc;
    tc.key = i32(event_data["Key"].GetInt());
    tc.mbutton = 0;
    i32 mouse_buttons = i32(event_data["Buttons"].GetInt());
    i32 qualifiers = i32(event_data["Qualifiers"].GetInt());
    ivec2 screen_size = uh->GetSubsystem<Urho3D::Graphics>()->GetSize();

    auto iter = uh->ictxt->active_triggers.begin();
    while (iter != uh->ictxt->active_triggers.end())
    {
        const input_action_trigger &curt = *iter;

        // Always end the action and send the trigger when the main key is depressed - dont care
        // about any of the qualifier situation
        if (tc.key == curt.cond.key)
        {
            if ((curt.tstate & T_END) == T_END)
            {
                itrigger_event ev{
                    curt.name, T_END, uh->ictxt->current_norm_mpos, {}, {}, viewport_with_mouse(uh->ictxt->vp_stack, screen_size, uh->ictxt->current_norm_mpos, {}, uh->ictxt->inv_pixel_ratio)};
                uh->ictxt->trigger(curt.name, ev);
            }
            iter = uh->ictxt->active_triggers.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

intern void on_mouse_down(input_uevent_handlers *uh, urho::StringHash event_type, urho::VariantMap &event_data)
{
    input_trigger_cond tc;
    tc.key = 0;
    tc.mbutton = event_data["Button"].GetInt();
    i32 mouse_buttons = i32(event_data["Buttons"].GetInt());
    i32 qualifiers = i32(event_data["Qualifiers"].GetInt());
    i32 wheel = 0;
    vec2 norm_mdelta;
    ivec2 screen_size = uh->GetSubsystem<Urho3D::Graphics>()->GetSize();

    if (tc.mbutton == MOUSEB_WHEEL)
        wheel = i32(event_data["Wheel"].GetInt());

    if (tc.mbutton == MOUSEB_MOVE)
    {
        vec2 current_mpos(float(event_data["X"].GetInt()), float(event_data["Y"].GetInt()));
        current_mpos = normalize_coords(current_mpos, screen_size);
        norm_mdelta = current_mpos - uh->ictxt->current_norm_mpos;
        uh->ictxt->current_norm_mpos = current_mpos;
    }

    for (int i = uh->ictxt->context_stack.size(); i > 0; --i)
    {
        input_keymap *cur_km = uh->ictxt->context_stack[i - 1];
        input_action_trigger *trig = nullptr;

        // Go through every trigger - if the current mb and quals are contained in the required qual and mb fields
        // and the allowed
        // First find all triggers with the exact match for key and mouse qualifiers
        auto tr = input_find_triggers(cur_km, tc);
        while (tr.first != tr.second)
        {
            const input_action_trigger &trig = tr.first->second;

            // Check the qualifier and mouse button required conditions
            bool pass_qual_required((trig.qual_req & qualifiers) == trig.qual_req);

            bool pass_mb_required((trig.mb_req & mouse_buttons) == trig.mb_req);

            // Check the qualifier and mouse button allowed conditions
            i32 allowed_quals = trig.qual_req | trig.qual_allowed;
            i32 allowed_mb = trig.mb_req | trig.mb_allowed | trig.cond.mbutton;
            bool pass_qual_allowed(((allowed_quals & urho::QUAL_ANY) == urho::QUAL_ANY) || (qualifiers | allowed_quals) == allowed_quals);
            bool pass_mb_allowed((mouse_buttons | allowed_mb) == allowed_mb);

            // If passes all the conditions, send the event for the trigger and mark trigger as active

            if (!trigger_already_active(uh->ictxt->active_triggers, trig) && pass_qual_required && pass_mb_required && pass_qual_allowed && pass_mb_allowed)
            {
                if ((trig.tstate & T_BEGIN) == T_BEGIN)
                {
                    itrigger_event ev{trig.name,
                                      T_BEGIN,
                                      uh->ictxt->current_norm_mpos,
                                      norm_mdelta,
                                      wheel,
                                      viewport_with_mouse(uh->ictxt->vp_stack, screen_size, uh->ictxt->current_norm_mpos, norm_mdelta, uh->ictxt->inv_pixel_ratio)};
                    uh->ictxt->trigger(trig.name, ev);
                }

                if (tc.mbutton != MOUSEB_WHEEL && tc.mbutton != MOUSEB_MOVE)
                    uh->ictxt->active_triggers.push_back(trig);
            }

            ++tr.first;
        }
    }
}

intern void on_mouse_up(input_uevent_handlers *uh, Urho3D::StringHash event_type, Urho3D::VariantMap &event_data)
{
    input_trigger_cond tc;
    tc.key = 0;
    tc.mbutton = event_data["Button"].GetInt();
    i32 mouse_buttons = i32(event_data["Buttons"].GetInt());
    i32 qualifiers = i32(event_data["Qualifiers"].GetInt());
    ivec2 screen_size = uh->GetSubsystem<urho::Graphics>()->GetSize();

    auto iter = uh->ictxt->active_triggers.begin();
    while (iter != uh->ictxt->active_triggers.end())
    {
        const input_action_trigger &curt = *iter;

        if (tc.mbutton == curt.cond.mbutton)
        {
            if (((curt.tstate & T_END) == T_END))
            {
                itrigger_event ev{
                    curt.name, T_END, uh->ictxt->current_norm_mpos, {}, {}, viewport_with_mouse(uh->ictxt->vp_stack, screen_size, uh->ictxt->current_norm_mpos, {}, uh->ictxt->inv_pixel_ratio)};
                uh->ictxt->trigger(curt.name, ev);
            }
            iter = uh->ictxt->active_triggers.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void input_init(input_context_stack *input, urho::Context *uctxt)
{
    input->event_handlers = new input_uevent_handlers(uctxt);
    input->event_handlers->ictxt = input;
    input->event_handlers->subscribe();
    input->event_handlers->GetSubsystem<urho::Input>()->SetMouseVisible(true);
}

void input_term(input_context_stack *input)
{
    input->event_handlers->unsubscribe();
    input->active_triggers.clear();
    input->vp_stack.clear();
    input->context_stack.clear();

    delete input->event_handlers;
    input->event_handlers = {};
}

input_uevent_handlers::input_uevent_handlers(urho::Context *uctxt) : urho::Object(uctxt)
{}

void input_uevent_handlers::subscribe()
{
    SubscribeToEvent(urho::E_KEYDOWN, URHO3D_HANDLER(input_uevent_handlers, handle_key_down));
    SubscribeToEvent(urho::E_KEYUP, URHO3D_HANDLER(input_uevent_handlers, handle_key_up));
    SubscribeToEvent(urho::E_MOUSEBUTTONDOWN, URHO3D_HANDLER(input_uevent_handlers, handle_mouse_down));
    SubscribeToEvent(urho::E_MOUSEBUTTONUP, URHO3D_HANDLER(input_uevent_handlers, handle_mouse_up));
    SubscribeToEvent(urho::E_MOUSEWHEEL, URHO3D_HANDLER(input_uevent_handlers, handle_mouse_wheel));
    SubscribeToEvent(urho::E_MOUSEMOVE, URHO3D_HANDLER(input_uevent_handlers, handle_mouse_move));
}

void input_uevent_handlers::unsubscribe()
{
    UnsubscribeFromAllEvents();
}

void input_uevent_handlers::handle_key_down(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data)
{
    on_key_down(this, event_type, event_data);
}

void input_uevent_handlers::handle_key_up(urho::StringHash event_type, urho::VariantMap &event_data)
{
    on_key_up(this, event_type, event_data);
}

void input_uevent_handlers::handle_mouse_down(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data)
{
    on_mouse_down(this, event_type, event_data);
}

void input_uevent_handlers::handle_mouse_up(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data)
{
    on_mouse_up(this, event_type, event_data);
}

void input_uevent_handlers::handle_mouse_wheel(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data)
{
    event_data["Button"] = MOUSEB_WHEEL;
    on_mouse_down(this, event_type, event_data);
}

void input_uevent_handlers::handle_mouse_move(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data)
{
    event_data["Button"] = MOUSEB_MOVE;
    on_mouse_down(this, event_type, event_data);
}