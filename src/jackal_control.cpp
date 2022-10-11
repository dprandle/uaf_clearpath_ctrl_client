#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/CollisionShape.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Technique.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/Urho3DAll.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/DebugHud.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/IO/IOEvents.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/AngelScript/Script.h>
#include <Urho3D/LuaScript/LuaScript.h>

#include "Urho3D/Core/Object.h"
#include "Urho3D/Math/StringHash.h"
#include "ss_router.h"
#include "typedefs.h"
#include "math_utils.h"
#include "jackal_control.h"
#include "logging.h"

int main(int argc, char **argv)
{
    urho::ParseArguments(argc, argv);
    urho::Context urho_ctxt;
    jackal_control_ctxt jctrl;

    jctrl_alloc(&jctrl, &urho_ctxt);
    if (!jctrl_init(&jctrl))
        return 0;

    jctrl_exec(&jctrl);
    jctrl_terminate(&jctrl);
}

intern void setup_global_keys(jackal_control_ctxt *ictxt)
{
    ASSERT(ictxt);
    ictxt->input_map.name = "global";

    input_action_trigger it;
    it.cond.mbutton = 0;
    it.mb_req = 0;
    it.mb_allowed = urho::MOUSEB_ANY;
    it.qual_req = 0;
    it.qual_allowed = urho::QUAL_ANY;
    it.cond.key = urho::KEY_F1;
    it.name = StringHash("ToggleConsole").ToHash();
    it.tstate = T_BEGIN;
    input_add_trigger(&ictxt->input_map, it);
    ss_connect(
        &ictxt->router, ictxt->input_dispatch.trigger, it.name, [ictxt](const itrigger_event &) { ictxt->urho_ctxt->GetSubsystem<urho::Console>()->Toggle(); });

    it.cond.key = urho::KEY_F2;
    it.name = StringHash("ToggleDebugHUD").ToHash();
    input_add_trigger(&ictxt->input_map, it);
    ss_connect(
        &ictxt->router, ictxt->input_dispatch.trigger, it.name, [ictxt](const itrigger_event &) { ictxt->urho_ctxt->GetSubsystem<urho::DebugHud>()->ToggleAll(); });

    it.cond.key = urho::KEY_F9;
    it.name = StringHash("TakeScreenshot").ToHash();
    input_add_trigger(&ictxt->input_map, it);
    ss_connect(&ictxt->router, ictxt->input_dispatch.trigger, it.name, [ictxt](const itrigger_event &) {
        auto graphics = ictxt->urho_ctxt->GetSubsystem<urho::Graphics>();
        auto fs = ictxt->urho_ctxt->GetSubsystem<urho::FileSystem>();
        urho::Image screenshot(ictxt->urho_ctxt);
        graphics->TakeScreenShot(screenshot);
        // Here we save in the Data folder with date and time appended
        screenshot.SavePNG(fs->GetProgramDir() + "Data/Screenshot_" + urho::Time::GetTimeStamp().Replaced(':', '_').Replaced('.', '_').Replaced(' ', '_') + ".png");
    });

    it.cond.key = urho::KEY_F3;
    it.name = StringHash("PrintTEData").ToHash();
    input_add_trigger(&ictxt->input_map, it);
    ss_connect(&ictxt->router, ictxt->input_dispatch.trigger, it.name, [ictxt](const itrigger_event &te) {
        ilog("Trig {} {} ({} {}) ({} {}) {} ({} {}) ({} {})",
             te.name,
             te.state,
             te.norm_mpos.x_,
             te.norm_mpos.y_,
             te.norm_mdelta.x_,
             te.norm_mdelta.y_,
             te.wheel,
             te.vp.vp_norm_mpos.x_,
             te.vp.vp_norm_mpos.y_,
             te.vp.vp_norm_mdelta.x_,
             te.vp.vp_norm_mdelta.y_);
    });
}

intern void create_visuals(jackal_control_ctxt *ctxt)
{
    auto uctxt = ctxt->urho_ctxt;
    auto ueng = ctxt->urho_engine;
    auto graphics = uctxt->GetSubsystem<urho::Graphics>();
    graphics->SetWindowTitle("Jackal Control");

    // Get default style
    urho::ResourceCache *cache = uctxt->GetSubsystem<urho::ResourceCache>();
    urho::XMLFile *xmlFile = cache->GetResource<urho::XMLFile>("UI/DefaultStyle.xml");
    urho::UI *usi = uctxt->GetSubsystem<urho::UI>();
    urho::UIElement *root = usi->GetRoot();
    root->SetDefaultStyle(xmlFile);

    // Create console
    urho::Console *console = ueng->CreateConsole();
    console->SetDefaultStyle(xmlFile);
    
    // Create debug HUD.
    urho::DebugHud *debugHud = ueng->CreateDebugHud();
    debugHud->SetDefaultStyle(xmlFile);

    ctxt->scene = new urho::Scene(uctxt);
    ctxt->scene->CreateComponent<urho::DebugRenderer>();
    ctxt->scene->CreateComponent<urho::Octree>();
    urho::PhysicsWorld *phys = ctxt->scene->CreateComponent<urho::PhysicsWorld>();
    phys->SetGravity(vec3(0.0f, 0.0f, -9.81f));

    urho::Node *editor_cam_node = new urho::Node(uctxt);
    urho::Camera *editor_cam = editor_cam_node->CreateComponent<urho::Camera>();
    editor_cam_node->SetPosition(vec3(8, -8, 5));
    editor_cam_node->SetDirection(vec3(0, 0, -1));
    editor_cam_node->Pitch(-70.0f);

    urho::Renderer *rnd = uctxt->GetSubsystem<urho::Renderer>();

    urho::Viewport *vp = new urho::Viewport(uctxt, ctxt->scene, editor_cam);
    vp->SetDrawDebug(true);
    rnd->SetViewport(0, vp);

    urho::RenderPath *rp = new urho::RenderPath;
    rp->Load(cache->GetResource<urho::XMLFile>("RenderPaths/DeferredWithOutlines.xml"));
    vp->SetRenderPath(rp);

    urho::Node *skyNode = ctxt->scene->CreateChild("Sky");
    skyNode->Rotate(quat(90.0f, vec3(1, 0, 0)));
    urho::Skybox *skybox = skyNode->CreateComponent<urho::Skybox>();
    skybox->SetModel(cache->GetResource<urho::Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<urho::Material>("Materials/Skybox.xml"));

    // Create a directional light
    urho::Node *light_node = ctxt->scene->CreateChild("Dir_Light");
    light_node->SetDirection(vec3(-0.0f, -0.5f, -1.0f));
    urho::Light *light = light_node->CreateComponent<urho::Light>();
    light->SetLightType(urho::LIGHT_DIRECTIONAL);
    light->SetColor(Color(1.0f, 1.0f, 1.0f));
    light->SetSpecularIntensity(5.0f);
    light->SetBrightness(1.0f);

    urho::Technique *tech = cache->GetResource<urho::Technique>("Techniques/DiffNormal.xml");
    urho::Material *grass_tile = cache->GetResource<urho::Material>("Materials/Tiles/Grass.xml");
    urho::Model *mod = cache->GetResource<urho::Model>("Models/Tiles/Grass.mdl");

    int cnt = 0;
    for (int y = 0; y < 20; ++y)
    {
        for (int x = 0; x < 20; ++x)
        {
            for (int z = 0; z < 2; ++z)
            {
                urho::Node *tile_node = ctxt->scene->CreateChild("Grass_Tile");

                urho::StaticModel *modc = tile_node->CreateComponent<urho::StaticModel>();
                modc->SetModel(mod);
                modc->SetMaterial(grass_tile);

                urho::CollisionShape *cs = tile_node->CreateComponent<urho::CollisionShape>();
                const urho::BoundingBox &bb = modc->GetBoundingBox();
                cs->SetBox(bb.Size());

                // RigidBody * rb = tile_node->CreateComponent<RigidBody>();
                // rb->SetMass(0.0f);
                // if (z == 1)
                //     rb->SetMass(10.0f);
                // rb->SetFriction(0.1f);
                // rb->SetRestitution(0.9f);

                tile_node->SetPosition(vec3(x, y, z));
                tile_node->SetRotation(quat(90.0f, vec3(1.0f, 0.0f, 0.0f)));
                ++cnt;
            }
        }
    }
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
    ctxt->ls = log_init();
    log_add_logger(ctxt->ls);

    auto fs = ctxt->urho_ctxt->GetSubsystem<urho::FileSystem>();
    auto prog_dir = fs->GetProgramDir();
    String str = prog_dir + "build_and_battle.log";
    str = fs->GetCurrentDir() + ";" + prog_dir;

    engine_params[urho::EP_FRAME_LIMITER] = false;
    engine_params[urho::EP_LOG_NAME] = str;
    engine_params[urho::EP_RESOURCE_PREFIX_PATHS] = str;
    engine_params[urho::EP_FULL_SCREEN] = false;
    engine_params[urho::EP_WINDOW_WIDTH] = 1920;
    engine_params[urho::EP_WINDOW_HEIGHT] = 1080;
    engine_params[urho::EP_HIGH_DPI] = true;
    engine_params[urho::EP_WINDOW_RESIZABLE] = true;
    if (!ctxt->urho_engine->Initialize(engine_params))
        return false;

    ctxt->urho_ctxt->RegisterSubsystem(new urho::Script(ctxt->urho_ctxt));
    create_visuals(ctxt);

    input_init(&ctxt->input_dispatch);
    setup_global_keys(ctxt);
    ctxt->input_dispatch.context_stack.push_back(&ctxt->input_map);

    ctxt->event_handler->subscribe();
    return true;
}

void jctrl_exec(jackal_control_ctxt *ctxt)
{
    while (!ctxt->urho_engine->IsExiting())
        ctxt->urho_engine->RunFrame();
}

void jctrl_terminate(jackal_control_ctxt *ctxt)
{
    log_terminate(ctxt->ls);
}

jctrl_uevent_handlers::jctrl_uevent_handlers(urho::Context *ctxt) : urho::Object(ctxt)
{}

void jctrl_uevent_handlers::subscribe()
{}

void jctrl_uevent_handlers::unsubscribe()
{
    UnsubscribeFromAllEvents();
}