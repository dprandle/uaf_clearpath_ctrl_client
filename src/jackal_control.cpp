#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/Serializer.h>

#include "Urho3D/Core/Context.h"
#include "Urho3D/Engine/Engine.h"
#include "Urho3D/Graphics/Viewport.h"
#include "Urho3D/IO/File.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/IO/Serializer.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/IO/VectorBuffer.h"
#include "Urho3D/Math/BoundingBox.h"
#include "Urho3D/Math/Color.h"
#include "Urho3D/Math/Rect.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Resource/JSONValue.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/UI/BorderImage.h"
#include "Urho3D/UI/UIEvents.h"
#include "Urho3D/UI/Button.h"
#include "Urho3D/UI/UIElement.h"
#include "Urho3D/UI/Sprite.h"

#include <string>

#include "input.h"
#include "joystick.h"
#include "network.h"
#include "ss_router.h"
#include "typedefs.h"
#include "math_utils.h"
#include "jackal_control.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

int main(int argc, char **argv)
{
    //urho::ParseArguments(argc, argv);
    urho::Context urho_ctxt{};
    jackal_control_ctxt jctrl{};

    jctrl_alloc(&jctrl, &urho_ctxt);
    if (!jctrl_init(&jctrl))
        return 0;

    jctrl_exec(&jctrl);
    jctrl_terminate(&jctrl);
    jctrl_free(&jctrl);
}

intern bool init_urho_engine(urho::Engine * urho_engine)
{
    urho::VariantMap engine_params;
    String log_loc = "build_and_battle.log";
    engine_params[urho::EP_FRAME_LIMITER] = false;
    engine_params[urho::EP_LOG_NAME] = log_loc;
    engine_params[urho::EP_FULL_SCREEN] = false;
    engine_params[urho::EP_WINDOW_WIDTH] = 1080;
    engine_params[urho::EP_WINDOW_HEIGHT] = 2400;
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

intern void setup_visuals(jackal_control_ctxt *ctxt)
{
    auto graphics = ctxt->urho_ctxt->GetSubsystem<urho::Graphics>();
    graphics->SetWindowTitle("Jackal Control");

    ctxt->scene->CreateComponent<urho::DebugRenderer>();
    ctxt->scene->CreateComponent<urho::Octree>();

    urho::Camera *editor_cam = ctxt->cam_node->CreateComponent<urho::Camera>();
    ctxt->cam_node->SetPosition(vec3(8, -8, 5));
    ctxt->cam_node->SetDirection(vec3(0, 0, -1));
    ctxt->cam_node->Pitch(-90.0f);

    urho::Renderer *rnd = ctxt->urho_ctxt->GetSubsystem<urho::Renderer>();
    urho::Viewport *vp = new urho::Viewport(ctxt->urho_ctxt, ctxt->scene, editor_cam);
    vp->SetDrawDebug(true);
    rnd->SetViewport(0, vp);
    auto zn = rnd->GetDefaultZone();
    zn->SetFogColor({0.0, 0.0, 0.0, 1.0});

    // Create a directional light
    urho::Node *light_node = ctxt->scene->CreateChild("dirlight");
    light_node->SetDirection(vec3(-0.0f, -0.5f, -1.0f));
    urho::Light *light = light_node->CreateComponent<urho::Light>();
    light->SetLightType(urho::LIGHT_DIRECTIONAL);
    light->SetColor(Color(1.0f, 1.0f, 1.0f));
    light->SetSpecularIntensity(5.0f);
    light->SetBrightness(1.0f);
}

void jctrl_alloc(jackal_control_ctxt *ctxt, urho::Context *urho_ctxt)
{
    ctxt->urho_ctxt = urho_ctxt;
    ctxt->urho_engine = new urho::Engine(ctxt->urho_ctxt);
    ctxt->scene = new urho::Scene(ctxt->urho_ctxt);
    ctxt->cam_node = new urho::Node(ctxt->urho_ctxt);

    input_alloc(&ctxt->input_dispatch, urho_ctxt);
    joystick_panel_alloc(&ctxt->js_panel, urho_ctxt);
}

void jctrl_free(jackal_control_ctxt *ctxt)
{
    joystick_panel_free(&ctxt->js_panel);
    input_free(&ctxt->input_dispatch);
    delete ctxt->scene;
    delete ctxt->cam_node;
    delete ctxt->urho_ctxt;
}

bool jctrl_init(jackal_control_ctxt *ctxt)
{
    if (!init_urho_engine(ctxt->urho_engine))
        return false;
    
    setup_ui_info(&ctxt->ui_inf, ctxt->urho_ctxt);

    setup_visuals(ctxt);

    input_init(&ctxt->input_dispatch);
    ctxt->input_map.name = "global";
    ctxt->input_dispatch.context_stack.push_back(&ctxt->input_map);

    joystick_panel_init(&ctxt->js_panel, ctxt->ui_inf, &ctxt->conn);

    net_connect(&ctxt->conn);
    ilog("Device pixel ratio inverse: %f", ctxt->ui_inf.dev_pixel_ratio_inv);

    // temp
    auto scan_func = [](const jackal_laser_scan_packet &){
        ilog("Scan received!");
    };
    ss_connect(&ctxt->router, ctxt->conn.scan_received,scan_func);

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

void jctrl_terminate(jackal_control_ctxt *ctxt)
{
    net_disconnect(&ctxt->conn);
    input_term(&ctxt->input_dispatch);
}

void jctrl_run_frame(jackal_control_ctxt *ctxt)
{
    net_rx(&ctxt->conn);
    ctxt->urho_engine->RunFrame();
}