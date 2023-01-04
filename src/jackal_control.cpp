#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/UI/UI.h>

#include "jackal_control.h"
#include "logging.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

int main(int argc, char **argv)
{
    auto args = urho::ParseArguments(argc, argv);
    auto urho_ctxt = new urho::Context;
    jackal_control_ctxt jctrl{};
    jctrl.urho_ctxt = urho_ctxt;

    if (!jctrl_init(&jctrl, args))
        return 0;

    jctrl_exec(&jctrl);
    jctrl_term(&jctrl);
}

intern bool init_urho_engine(urho::Engine * urho_engine, float ui_scale)
{
    urho::VariantMap engine_params;
    String log_loc = "build_and_battle.log";
    engine_params[urho::EP_FRAME_LIMITER] = false;
    engine_params[urho::EP_LOG_NAME] = log_loc;
    engine_params[urho::EP_FULL_SCREEN] = false;
    engine_params[urho::EP_WINDOW_WIDTH] = 1080*ui_scale;
    engine_params[urho::EP_WINDOW_HEIGHT] = 2400*ui_scale;
#if !defined(__EMSCRIPTEN__)
    engine_params[urho::EP_HIGH_DPI] = true;
    engine_params[urho::EP_WINDOW_RESIZABLE] = true;
#endif
    if (!urho_engine->Initialize(engine_params))
        return false;
    return true;
}

intern void setup_ui_info(ui_info * ui_inf, urho::Context * uctxt)
{
    auto rcache = uctxt->GetSubsystem<urho::ResourceCache>();
#if defined(__EMSCRIPTEN__)
    ui_inf->dev_pixel_ratio_inv = 1.0 / emscripten_get_device_pixel_ratio();
#endif
    ui_inf->style = rcache->GetResource<urho::XMLFile>("UI/jackal_style.xml");
    ui_inf->ui_sys = uctxt->GetSubsystem<urho::UI>();
}

intern void setup_main_renderer(jackal_control_ctxt *ctxt)
{
    auto graphics = ctxt->urho_ctxt->GetSubsystem<urho::Graphics>();
    graphics->SetWindowTitle("Jackal Control");

    auto cache = ctxt->urho_ctxt->GetSubsystem<urho::ResourceCache>();
    auto rpath = cache->GetResource<urho::XMLFile>("RenderPaths/clear_only.xml");
    urho::Renderer *rnd = ctxt->urho_ctxt->GetSubsystem<urho::Renderer>();
    urho::Viewport *vp = new urho::Viewport(ctxt->urho_ctxt, nullptr, nullptr);
    vp->SetRenderPath(rpath);
    rnd->SetViewport(0, vp);

    auto zn = rnd->GetDefaultZone();
    zn->SetFogColor({0.0, 0.0, 0.0, 1.0});
}

intern void parse_command_line_args(int *port, urho::String * ip, float *ui_scale, const urho::StringVector & args)
{
    for (const auto &arg : args)
    {
        auto split = arg.Split('=');
        if (split.Size() == 2)
        {
            if (split[0] == "-ip")
            {
                *ip = split[1];
            }
            else if (split[0] == "-port")
            {
                *port = strtol(split[1].CString(), nullptr, 10);
            }
            else if (split[0] == "-ui_scale")
            {
                *ui_scale = strtof(split[1].CString(), nullptr);
                ilog("Setting ui scale val to %f", *ui_scale);
            }
        }
    }
}

intern void jctrl_run_frame(jackal_control_ctxt *ctxt)
{
    net_rx(&ctxt->conn);
    ctxt->urho_engine->RunFrame();
}

bool jctrl_init(jackal_control_ctxt *ctxt, const urho::StringVector & args)
{
    ctxt->urho_engine = new urho::Engine(ctxt->urho_ctxt);
    int port;
    urho::String ip;
    parse_command_line_args(&port, &ip, &ctxt->ui_inf.dev_pixel_ratio_inv, args);

    if (!init_urho_engine(ctxt->urho_engine, ctxt->ui_inf.dev_pixel_ratio_inv))
        return false;
    
    log_init(ctxt->urho_ctxt);

    ilog("Initializing jackal control");

#if defined(DEBUG_VERSION)
    log_set_level(urho::LOG_DEBUG);
#else
    log_set_level(urho::LOG_DEBUG);
#endif   

    setup_ui_info(&ctxt->ui_inf, ctxt->urho_ctxt);

    setup_main_renderer(ctxt);

    input_init(&ctxt->inp.dispatch, ctxt->urho_ctxt);
    ctxt->inp.map.name = "global";
    ctxt->inp.dispatch.inv_pixel_ratio = ctxt->ui_inf.dev_pixel_ratio_inv;
    ctxt->inp.dispatch.context_stack.push_back(&ctxt->inp.map);

    net_connect(&ctxt->conn, ip.CString(), port);

    joystick_panel_init(&ctxt->js_panel, ctxt->ui_inf, &ctxt->conn);
    
    map_panel_init(&ctxt->mpanel, ctxt->ui_inf, &ctxt->conn, &ctxt->inp);

    ss_connect(&ctxt->router, ctxt->js_panel.in_use, [ctxt](bool in_use) {
        ctxt->mpanel.js_enabled = in_use;
    });    

    ilog("Device pixel ratio inverse: %f", ctxt->ui_inf.dev_pixel_ratio_inv);
    return true;
}

#if defined(IOS) || defined(TVOS) || defined(__EMSCRIPTEN__)
intern void run_frame_proxy(void *data)
{
    auto ctxt = (jackal_control_ctxt *)data;
    jctrl_run_frame(ctxt);
}
#endif

void jctrl_exec(jackal_control_ctxt *ctxt)
{
#if !defined(IOS) && !defined(TVOS) && !defined(__EMSCRIPTEN__)
    while (!ctxt->urho_engine->IsExiting())
    {
        jctrl_run_frame(ctxt);
    }
#elif defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(run_frame_proxy, ctxt, -1, true);
#endif
}

void jctrl_term(jackal_control_ctxt *ctxt)
{
    ilog("Terminating jackal control");
    net_disconnect(&ctxt->conn);
    input_term(&ctxt->inp.dispatch);
    joystick_panel_term(&ctxt->js_panel);
    map_panel_term(&ctxt->mpanel);
    log_term();
}
