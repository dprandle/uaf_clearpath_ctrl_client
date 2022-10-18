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

#include "Urho3D/Graphics/Viewport.h"
#include "Urho3D/IO/File.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/IO/Serializer.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/IO/VectorBuffer.h"
#include "Urho3D/Math/BoundingBox.h"
#include "Urho3D/Math/Color.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Resource/JSONValue.h"
#include "Urho3D/UI/Button.h"
#include "Urho3D/UI/UIElement.h"

#include "ss_router.h"
#include "typedefs.h"
#include "math_utils.h"
#include "jackal_control.h"

#if defined(IOS) || defined(TVOS) || defined(__EMSCRIPTEN__)
// Code for supporting SDL_iPhoneSetAnimationCallback() and emscripten_set_main_loop_arg()
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif
void RunFrame(void* data)
{
    static_cast<urho::Engine*>(data)->RunFrame();
}
#endif

int main(int argc, char **argv)
{
    urho::ParseArguments(argc, argv);
    urho::Context urho_ctxt {};
    jackal_control_ctxt jctrl {};

    jctrl_alloc(&jctrl, &urho_ctxt);
    if (!jctrl_init(&jctrl))
        return 0;

    jctrl_exec(&jctrl);
    jctrl_terminate(&jctrl);
}

intern void setup_global_keys(jackal_control_ctxt *ictxt)
{
    ictxt->input_map.name = "global";
}

intern void create_visuals(jackal_control_ctxt *ctxt)
{
    auto uctxt = ctxt->urho_ctxt;
    auto ueng = ctxt->urho_engine;
    auto graphics = uctxt->GetSubsystem<urho::Graphics>();
    graphics->SetWindowTitle("Jackal Control");
    
    // Get default style
    urho::ResourceCache *cache = uctxt->GetSubsystem<urho::ResourceCache>();
    auto *xmlFile = cache->GetResource<urho::XMLFile>("UI/jacakal_style.xml");
    urho::UI *usi = uctxt->GetSubsystem<urho::UI>();
    urho::UIElement *root = usi->GetRoot();

    auto btn = new urho::Button(uctxt);
    root->AddChild(btn);
    btn->SetEnableAnchor(true);
    btn->SetMinAnchor(0.25, 0.25);
    btn->SetMaxAnchor(0.75, 0.75);
    
    ctxt->scene = new urho::Scene(uctxt);
    
    ctxt->scene->CreateComponent<urho::DebugRenderer>();
    ctxt->scene->CreateComponent<urho::Octree>();

    urho::Node *editor_cam_node = new urho::Node(uctxt);
    urho::Camera *editor_cam = editor_cam_node->CreateComponent<urho::Camera>();
    editor_cam_node->SetPosition(vec3(8, -8, 5));
    editor_cam_node->SetDirection(vec3(0, 0, -1));
    editor_cam_node->Pitch(-90.0f);

    urho::Renderer *rnd = uctxt->GetSubsystem<urho::Renderer>();

    urho::Viewport *vp = new urho::Viewport(uctxt, ctxt->scene, editor_cam);
    vp->SetDrawDebug(true);
    rnd->SetViewport(0, vp);
    auto zn = rnd->GetDefaultZone();
    zn->SetFogColor({0.7, 0.7, 0.7, 1.0});
    
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
    ctxt->event_handler = new jctrl_uevent_handlers(ctxt->urho_ctxt);
    input_alloc(&ctxt->input_dispatch, urho_ctxt);
}

void jctrl_free(jackal_control_ctxt *ctxt)
{
    input_free(&ctxt->input_dispatch);
    delete ctxt->event_handler;
    delete ctxt->urho_ctxt;
}

bool jctrl_init(jackal_control_ctxt *ctxt)
{
    urho::VariantMap engine_params;
    
    String log_loc = "/home/dprandle/projects/jackal_control/build/build_and_battle.log";

    engine_params[urho::EP_FRAME_LIMITER] = false;
    engine_params[urho::EP_LOG_NAME] = log_loc;
    engine_params[urho::EP_FULL_SCREEN] = false;
    engine_params[urho::EP_WINDOW_WIDTH] = 1440;
    engine_params[urho::EP_WINDOW_HEIGHT] = 2960;
    engine_params[urho::EP_HIGH_DPI] = true;
    engine_params[urho::EP_WINDOW_RESIZABLE] = true;
    if (!ctxt->urho_engine->Initialize(engine_params))
        return false;

    create_visuals(ctxt);

    input_init(&ctxt->input_dispatch);
    setup_global_keys(ctxt);
    ctxt->input_dispatch.context_stack.push_back(&ctxt->input_map);

    ctxt->event_handler->subscribe();
    return true;
}

void jctrl_exec(jackal_control_ctxt* ctxt)
{
#if !defined(IOS) && !defined(TVOS) && !defined(__EMSCRIPTEN__)
    while (!ctxt->urho_engine->IsExiting())
        ctxt->urho_engine->RunFrame();
#elif defined(__EMSCRIPTEN__)
        emscripten_set_main_loop_arg(RunFrame, ctxt->urho_engine, 0, 1);
#endif
}

void jctrl_terminate(jackal_control_ctxt *ctxt)
{}

jctrl_uevent_handlers::jctrl_uevent_handlers(urho::Context *ctxt) : urho::Object(ctxt)
{}

void jctrl_uevent_handlers::subscribe()
{}

void jctrl_uevent_handlers::unsubscribe()
{
    UnsubscribeFromAllEvents();
}