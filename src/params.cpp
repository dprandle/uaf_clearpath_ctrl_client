#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/ScrollBar.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/View3D.h>

#include "network.h"
#include "params.h"
#include "mapping.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/websocket.h>
EM_JS(void, toggle_input_visibility, (int rows, int cols), {
    let ta = document.getElementById('text-area');
    if (ta.style.display === "block") {
        ta.style.display = "none";
    }
    else {
        ta.style.display = "block";
        if (rows > ta.rows)
            ta.rows = rows;
        if (cols > ta.cols)
            ta.cols = cols;
    }
});

EM_JS(void, set_input_text, (const char *txt), {
    let ta = document.getElementById('text-area');
    ta.value = UTF8ToString(txt);
    ta.style.display = "block";
});

EM_JS(char *, get_input_text, (), {
    let ta_text = document.getElementById('text-area').value;
    let length = lengthBytesUTF8(ta_text) + 1;
    let str = _malloc(length);
    stringToUTF8(ta_text, str, length);
    console.log(`Should be returning  ${length} bytes for ${
        ta_text}`);
    return str;
});
#endif

intern const float UI_ADDITIONAL_BTN_SCALING = 1.0f;

intern void setup_text_notice_widget(map_panel *mp, const ui_info &ui_inf)
{
    auto uctxt = ui_inf.ui_sys->GetContext();

    mp->text_disp.apanel.widget = ui_inf.ui_sys->GetRoot()->CreateChild<urho::UIElement>();
    mp->text_disp.apanel.widget->SetStyle("TextDispAnimatedPanel", ui_inf.style);

    mp->text_disp.apanel.sview = mp->text_disp.apanel.widget->CreateChild<urho::ListView>();
    mp->text_disp.apanel.sview->SetStyle("AnimatedPanelListView", ui_inf.style);

    mp->text_disp.apanel.hide_show_btn_bg = mp->text_disp.apanel.widget->CreateChild<urho::BorderImage>();
    mp->text_disp.apanel.hide_show_btn_bg->SetStyle("TextDispHideShowBG", ui_inf.style);

    mp->text_disp.apanel.hide_show_panel = mp->text_disp.apanel.hide_show_btn_bg->CreateChild<urho::Button>();
    mp->text_disp.apanel.hide_show_panel->SetStyle("TextDispShow", ui_inf.style);
    auto offset = mp->text_disp.apanel.hide_show_panel->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv;
    mp->text_disp.apanel.hide_show_panel->SetMaxOffset(offset);
    mp->text_disp.apanel.hide_show_btn_bg->SetMaxOffset(offset);
    mp->text_disp.apanel.anim_dir = PANEL_ANIM_VERTICAL;
    mp->text_disp.apanel.anchor_rest_point = mp->text_disp.apanel.widget->GetMaxAnchor().y_;
}

intern void setup_accept_params_button(map_panel *mp, const ui_info &ui_inf)
{
    auto uctxt = ui_inf.ui_sys->GetContext();

    mp->accept_inp.widget = ui_inf.ui_sys->GetRoot()->CreateChild<urho::UIElement>();
    mp->accept_inp.widget->SetStyle("SendParamsGp", ui_inf.style);

    mp->accept_inp.send_btn = mp->accept_inp.widget->CreateChild<urho::Button>();
    mp->accept_inp.send_btn->SetStyle("SendParamsBtn", ui_inf.style);

    mp->accept_inp.send_btn_text = mp->accept_inp.send_btn->CreateChild<urho::Text>();
    mp->accept_inp.send_btn_text->SetStyle("SendParamsBtnText", ui_inf.style);

    mp->accept_inp.get_btn = mp->accept_inp.widget->CreateChild<urho::Button>();
    mp->accept_inp.get_btn->SetStyle("GetParamsBtn", ui_inf.style);

    mp->accept_inp.get_btn_text = mp->accept_inp.get_btn->CreateChild<urho::Text>();
    mp->accept_inp.get_btn_text->SetStyle("GetParamsBtnText", ui_inf.style);

    vec2 offset = vec2{270 * 2, 80} * ui_inf.dev_pixel_ratio_inv * UI_ADDITIONAL_BTN_SCALING;
    ivec2 ioffset = ivec2(offset.x_, offset.y_);
    mp->accept_inp.widget->SetMaxOffset(ioffset);

    double button_ratio = 80.0f / (float)mp->view->GetHeight();
    mp->text_disp.apanel.anchor_set_point = 0.20 - button_ratio;

    mp->accept_inp.send_btn_text->SetFontSize(24 * ui_inf.dev_pixel_ratio_inv);
    mp->accept_inp.get_btn_text->SetFontSize(24 * ui_inf.dev_pixel_ratio_inv);
}

intern void show_received_text(map_panel *mp, const text_block &tb, const ui_info &ui_inf)
{
    static char txt[5000] = {};
    strncpy(txt, tb.text, tb.txt_size);
    txt[tb.txt_size] = '\0';

    auto txt_elem = new urho::Text(mp->view->GetContext());
    txt_elem->SetStyle("AnimatedPanelText", ui_inf.style);
    txt_elem->SetFontSize(26 * ui_inf.dev_pixel_ratio_inv);
    urho::String str("> ");
    str += urho::String(txt);
    txt_elem->SetText(str);
    mp->text_disp.apanel.sview->AddItem(txt_elem);

    if (mp->text_disp.apanel.anim_state != PANEL_ANIM_INACTIVE)
        return;
    float cur_y_anch = mp->text_disp.apanel.widget->GetMaxAnchor().y_;
    if (fequals(cur_y_anch, 0.0f)) {
        mp->text_disp.apanel.anim_state = PANEL_ANIM_SHOW;
        mp->text_disp.cur_open_time = mp->text_disp.apanel.max_anim_time;
    }
    else if (mp->text_disp.cur_open_time > 0.0) {
        mp->text_disp.cur_open_time = mp->text_disp.apanel.max_anim_time;
    }
}

intern void handle_received_get_params_response(map_panel *mp, const text_block &tb, const ui_info &ui_inf)
{
    static char txt[text_block::MAX_TXT_SIZE] = {};
    strncpy(txt, tb.text, tb.txt_size);
    txt[tb.txt_size] = '\0';
    ilog("Recieved text: %s", txt);
    mp->accept_inp.get_btn_text->SetText("Get Params");
#if defined(__EMSCRIPTEN__)
    mp->toolbar.set_params->SetChecked(true);
    set_input_text(txt);
#endif
}

intern void param_run_frame(map_panel *mp, float dt, const ui_info &ui_inf)
{
    if (!animated_panel_run_frame(&mp->text_disp.apanel, dt, ui_inf, "TextDisp") &&
        (mp->text_disp.cur_open_time > (mp->text_disp.apanel.max_anim_time - FLOAT_EPS))) {
        mp->text_disp.cur_open_time += dt;
        if (mp->text_disp.cur_open_time >= mp->text_disp.max_open_timer_time) {
            mp->text_disp.cur_open_time = 0.0f;
            mp->text_disp.apanel.anim_state = PANEL_ANIM_HIDE;
        }
    }
}

void param_init(map_panel *mp, net_connection *conn, const ui_info &ui_inf)
{
    ilog("Initializing params");

    setup_accept_params_button(mp, ui_inf);
    setup_text_notice_widget(mp, ui_inf);

    ss_connect(&mp->router, conn->param_set_response_received, [mp, ui_inf](const text_block &pckt) {
        show_received_text(mp, pckt, ui_inf);
    });
    ss_connect(&mp->router, conn->param_get_response_received, [mp, ui_inf](const text_block &pckt) {
        handle_received_get_params_response(mp, pckt, ui_inf);
    });

    mp->text_disp.apanel.widget->SubscribeToEvent(urho::E_UPDATE,
                                                  [mp, ui_inf](urho::StringHash type, urho::VariantMap &ev_data) {
                                                      auto dt = ev_data[urho::Update::P_TIMESTEP].GetFloat();
                                                      param_run_frame(mp, dt, ui_inf);
                                                  });
}

void param_term(map_panel *mp)
{
    ilog("Terminating params");
}

void param_toggle_input_visible(map_panel *mp)
{
#if defined(__EMSCRIPTEN__)
    toggle_input_visibility(mp->view->GetHeight() * 0.03, mp->view->GetWidth() * 0.08);
#else
#endif
    mp->accept_inp.widget->SetVisible(!mp->accept_inp.widget->IsVisible());
}

void param_handle_mouse_released(map_panel *mp, urho::UIElement *elem, net_connection *conn)
{
    if (elem == mp->text_disp.apanel.hide_show_panel) {
        animated_panel_hide_show_pressed(&mp->text_disp.apanel);
    }
    else if (elem == mp->accept_inp.get_btn) {
        command_get_params cgp{};
        mp->accept_inp.get_btn_text->SetText("Getting...");
        net_tx(*conn, cgp);
    }
    else if (elem == mp->accept_inp.send_btn || elem == mp->accept_inp.send_btn_text) {
        static command_set_params param_pckt{};

        // Get input from box and send it
#if defined(__EMSCRIPTEN__)
        char *txt = get_input_text();
        param_pckt.blob_size = strlen(txt);
        if (command_set_params::MAX_STR_SIZE < param_pckt.blob_size)
            param_pckt.blob_size = command_set_params::MAX_STR_SIZE;
        strncpy((char *)param_pckt.blob_data, txt, param_pckt.blob_size);
        ilog("Sending Params: %s", txt);
        free(txt);
        net_tx(*conn, param_pckt);
#endif
        mp->toolbar.set_params->SetChecked(false);
    }
}
