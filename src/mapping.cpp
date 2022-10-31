#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Core/CoreEvents.h>

#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Math/Color.h"
#include "Urho3D/Resource/XMLFile.h"
#include "jackal_control.h"
#include "mapping.h"

#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Technique.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/View3D.h>

void map_panel_init(map_panel *mp, const ui_info &ui_inf, net_connection *conn)
{
    auto uctxt = ui_inf.ui_sys->GetContext();
    auto cache = uctxt->GetSubsystem<urho::ResourceCache>();
    auto rpath = cache->GetResource<urho::XMLFile>("RenderPaths/Forward.xml");
    mp->view = new urho::View3D(uctxt);
    auto scene = new urho::Scene(uctxt);
    auto cam_node = new urho::Node(uctxt);

    ui_inf.ui_sys->GetRoot()->AddChild(mp->view);
    mp->view->SetEnableAnchor(true);
    mp->view->SetMinAnchor({0.0f, 0.0f});
    mp->view->SetMaxAnchor({1.0f, 0.7f});

    scene->CreateComponent<urho::DebugRenderer>();
    scene->CreateComponent<urho::Octree>();

    auto cam = cam_node->CreateComponent<urho::Camera>();
    mp->view->SetView(scene, cam, true);
    mp->view->GetViewport()->SetRenderPath(rpath);

    cam_node->SetPosition({8, -8, 5});
    cam_node->SetDirection({0, 0, -1});
    cam_node->Pitch(-70.0f);

    // Create a directional light
    auto light_node = scene->CreateChild("Dir_Light");
    light_node->SetDirection({-0.0f, -0.5f, -1.0f});
    auto *light = light_node->CreateComponent<urho::Light>();
    light->SetLightType(urho::LIGHT_DIRECTIONAL);
    light->SetColor({1.0f, 1.0f, 1.0f});
    light->SetSpecularIntensity(5.0f);
    light->SetBrightness(1.0f);

    // These are temporary to visualize stuff working
    auto sky_node = scene->CreateChild("Sky");
    sky_node->Rotate(quat(90.0f, {1, 0, 0}));
    auto skybox = sky_node->CreateComponent<urho::Skybox>();
    skybox->SetModel(cache->GetResource<urho::Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<urho::Material>("Materials/Skybox.xml"));

    auto grass_tile = cache->GetResource<urho::Material>("Materials/Tiles/Grass.xml");
    auto *grass_mod = cache->GetResource<urho::Model>("Models/Tiles/Grass.mdl");

    int cnt = 0;
    for (int y = 0; y < 20; ++y)
    {
        for (int x = 0; x < 20; ++x)
        {
            auto tile_node = scene->CreateChild("Grass_Tile");
            auto static_model = tile_node->CreateComponent<urho::StaticModel>();
            static_model->SetModel(grass_mod);
            static_model->SetMaterial(grass_tile);
            tile_node->SetPosition({x*1.45f, y*1.45f, 0.0f});
            tile_node->SetRotation(quat(90.0f, {1.0f, 0.0f, 0.0f}));
            ++cnt;
        }
    }
        
}

void map_panel_term(map_panel *mp)
{}

void map_panel_run_frame(map_panel *mpanel, net_connection *conn)
{}
