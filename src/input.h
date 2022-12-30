#pragma once

#include "Urho3D/Core/Context.h"
#include <Urho3D/Core/Object.h>
#include <Urho3D/Input/InputEvents.h>
#include <map>
#include <vector>

#include "math_utils.h"
#include "ss_router.h"

const i32 MOUSEB_MOVE = SDL_BUTTON(7);
const i32 MOUSEB_WHEEL = SDL_BUTTON(8);

namespace Urho3D
{
class Viewport;
class UIElement;
} // namespace Urho3D

enum input_trigger_state
{
    T_BEGIN = 1,
    T_END = 2
};

union input_trigger_cond
{
    struct
    {
        i32 key; // 0 is no key - if no key then mb_mask must be non-zero
        i32 mbutton;
    };
    i64 lookup_key;
};

inline bool operator==(const input_trigger_cond &lhs, const input_trigger_cond &rhs)
{
    return (lhs.lookup_key == rhs.lookup_key);
}

struct input_action_trigger
{
    u32 name;
    i32 tstate;
    input_trigger_cond cond; // Here for convenience
    i32 mb_req;     // 0 is no button
    i32 qual_req;   // 0 is no keyboard qualifier
    i32 mb_allowed;      // If MOUSEB_ANY is included int the mouse_buttons, these mouse buttons will cause the state to end or not begin
    i32 qual_allowed;    // Same thing as above with QUAL_ANY for key qualifiers
};

inline bool operator==(const input_action_trigger &lhs, const input_action_trigger &rhs)
{
    return (lhs.cond == rhs.cond && lhs.mb_req == rhs.mb_req && lhs.qual_req == rhs.qual_req && lhs.tstate == rhs.tstate);
}

using itrigger_map = std::multimap<i64, input_action_trigger>;
using itrigger_range = std::pair<itrigger_map::iterator, itrigger_map::iterator>;

struct input_keymap
{
    std::string name;
    itrigger_map tmap;
};

struct viewport_element
{
    Urho3D::Viewport *vp;
    Urho3D::UIElement *view_element;
};

struct viewport_info
{
    viewport_element vp_desc;
    Urho3D::UIElement *element_under_mouse{};
    vec2 vp_norm_mpos;
    vec2 vp_norm_mdelta;
};

struct input_context_stack;

struct itrigger_event
{
    u32 name;
    i32 state;
    vec2 norm_mpos;
    vec2 norm_mdelta;
    i32 wheel;
    viewport_info vp;
};

struct input_uevent_handlers : public Urho3D::Object
{
    URHO3D_OBJECT(input_uevent_handlers, urho::Object)
    input_uevent_handlers(urho::Context * ctxt);

    input_context_stack * ictxt {};

    void subscribe();
    void unsubscribe();
    
    void handle_key_down(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data);
    void handle_key_up(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data);
    void handle_mouse_down(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data);
    void handle_mouse_up(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data);
    void handle_mouse_wheel(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data);
    void handle_mouse_move(Urho3D::StringHash event_type, Urho3D::VariantMap &event_data);
};

// Keep track of current modifier state etc?
struct input_context_stack
{
    std::vector<input_keymap*> context_stack;
    std::vector<input_action_trigger> active_triggers;
    std::vector<viewport_element> vp_stack;
    double inv_pixel_ratio {0.0};
    vec2 current_norm_mpos;
    input_uevent_handlers * event_handlers {};
    ss_signal<const itrigger_event&> trigger {};
};

struct input_data
{
    input_context_stack dispatch;
    input_keymap map;
};

void input_init(input_context_stack * input, urho::Context * uctxt);
void input_term(input_context_stack * input);

itrigger_range input_find_triggers(input_keymap * kmap, const input_trigger_cond &cond);
void input_add_trigger(input_keymap * kmap, const input_action_trigger & trigger);
bool input_remove_trigger(input_keymap * kmap, const input_action_trigger & triggger);
int input_remove_triggers(input_keymap * kmap, const input_trigger_cond &cond);
