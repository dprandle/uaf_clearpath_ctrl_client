#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Core/CoreEvents.h>

#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Graphics/BillboardSet.h"
#include "Urho3D/Input/InputConstants.h"
#include "Urho3D/Math/Color.h"
#include "Urho3D/Math/StringHash.h"
#include "Urho3D/Resource/XMLFile.h"

#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/BillboardSet.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/DebugHud.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/View3D.h>

#include "input.h"
#include "jackal_control.h"
#include "mapping.h"
#include "math_utils.h"
#include "network.h"

intern void setup_camera_controls(map_panel *mp, urho::Node * cam_node, input_data * inp)
{
    auto on_mouse_move = [cam_node](const itrigger_event &tevent) {
        auto trans = vec2(tevent.vp.vp_norm_mdelta.x_, tevent.vp.vp_norm_mdelta.y_)*cam_node->GetWorldPosition().z_;
        cam_node->Translate2D(trans, Urho3D::TransformSpace::World);
    };

    auto on_mouse_zoom = [cam_node](const itrigger_event &tevent) {
        cam_node->Translate(vec3{0,0,tevent.wheel*1.0f});
    };

    input_action_trigger it{};
    it.cond.mbutton = MOUSEB_MOVE;
    it.name = urho::StringHash("CamMoveAction").ToHash();
    it.tstate = T_BEGIN;
    it.mb_req = urho::MOUSEB_LEFT;
    it.mb_allowed = 0;
    it.qual_req = 0;
    it.qual_allowed = urho::QUAL_ANY;
    input_add_trigger(&inp->map, it);
    ss_connect(&mp->router, inp->dispatch.trigger, it.name, on_mouse_move);

    it.cond.mbutton = MOUSEB_WHEEL;
    it.name = urho::StringHash("Zoom").ToHash();
    it.tstate = T_BEGIN;
    it.mb_req = 0;
    it.mb_allowed = 0;
    it.qual_req = 0;
    it.qual_allowed = 0;
    input_add_trigger(&inp->map, it);
    ss_connect(&mp->router, inp->dispatch.trigger, it.name, on_mouse_zoom);

    viewport_element vp {mp->view->GetViewport(),mp->view};
    inp->dispatch.vp_stack.emplace_back(vp);
}

intern void create_3dview(map_panel *mp, urho::ResourceCache * cache, urho::UIElement * root, urho::Context * uctxt)
{
    auto rpath = cache->GetResource<urho::XMLFile>("RenderPaths/Forward.xml");
    mp->view = new urho::View3D(uctxt);
    auto scene = new urho::Scene(uctxt);
    auto cam_node = new urho::Node(uctxt);

    root->AddChild(mp->view);
    mp->view->SetEnableAnchor(true);
    mp->view->SetMinAnchor({0.0f, 0.0f});
    mp->view->SetMaxAnchor({1.0f, 0.7f});

    scene->CreateComponent<urho::DebugRenderer>();
    scene->CreateComponent<urho::Octree>();

    auto cam = cam_node->CreateComponent<urho::Camera>();
    mp->view->SetView(scene, cam, true);
    mp->view->GetViewport()->SetRenderPath(rpath);

    cam_node->SetPosition({0, 2.5, 10});
    cam_node->SetDirection({0, 0, -1});
    cam_node->Pitch(00.0f);

    auto zone_node = scene->CreateChild("Zone");
    auto zone = zone_node->CreateComponent<urho::Zone>();
    zone->SetBoundingBox({-1000.0f, 1000.0f});
    zone->SetAmbientColor({0.3f, 0.3f, 0.3f});
    zone->SetFogColor({0.4f, 0.4f, 0.4f});
    zone->SetFogStart(10.0f);
    zone->SetFogEnd(200.0f);

    // Create a directional light
    auto light_node = scene->CreateChild("Dir_Light");
    light_node->SetDirection({0.0f, 0.5f, -1.0f});
    auto *light = light_node->CreateComponent<urho::Light>();
    light->SetLightType(urho::LIGHT_DIRECTIONAL);
    light->SetSpecularIntensity(5.0f);
    light->SetBrightness(0.7f);
}

intern void setup_scene(map_panel *mp, urho::ResourceCache * cache, urho::Scene * scene)
{
    auto jackal_model = cache->GetResource<urho::Model>("Models/jackal_base.mdl");
    mp->jackal_node = scene->CreateChild("jackal");
    auto smodel = mp->jackal_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_model);
    mp->jackal_node->SetRotation(quat(-90.0f, {1.0f, 0.0f, 0.0f}));

    auto scan_mat = cache->GetResource<urho::Material>("Materials/scan_billboard.xml");
    mp->scan_node = scene->CreateChild("scan");
    mp->scan_bb = mp->scan_node->CreateComponent<urho::BillboardSet>();
    mp->scan_bb->SetMaterial(scan_mat);
    mp->scan_bb->SetNumBillboards(jackal_laser_scan::SCAN_POINTS);
    mp->scan_bb->SetFixedScreenSize(true);

    auto occ_mat = cache->GetResource<urho::Material>("Materials/occ_billboard.xml");
    mp->occ_grid_node = scene->CreateChild("occ_grid");
    mp->occ_grid_bb = mp->occ_grid_node->CreateComponent<urho::BillboardSet>();
    mp->occ_grid_bb->SetFixedScreenSize(false);
    mp->occ_grid_bb->SetMaterial(occ_mat);
}

intern ivec2 index_to_cell_coords(u32 index, u32 row_width)
{
    return {(int)(index % row_width), (int)(index / row_width)};
}

intern vec3 get_pos_from_map_ind(u32 index, const occ_grid_meta &grid_meta)
{
    ivec2 cell_coords = index_to_cell_coords(index, grid_meta.width);
    vec3 map_origin =  vec3_from(grid_meta.origin_pos);
    vec3 cell_to_world = {(float)cell_coords.x_, (float)cell_coords.y_, 0.1f};
    return map_origin + cell_to_world*grid_meta.resolution;
}

intern void update_scene_from_occ_grid(map_panel *mp, const occupancy_grid_update &grid)
{
    // Set the number of billboards to match the entire map - if this causes problems we can rethink it
    mp->occ_grid_bb->SetNumBillboards(grid.meta.width*grid.meta.height);
    vec2 cell_size = {grid.meta.resolution, grid.meta.resolution};

    // Go through and use the arriving delta packet to set the current map values
    for (int i = 0; i < grid.meta.change_elem_count; ++i)
    {
        u32 map_ind = (grid.change_elems[i] >> 8);
        u8 prob = (u8)grid.change_elems[i];
        auto bb = mp->occ_grid_bb->GetBillboard(map_ind);
        if (prob <= 100)
        {
            bb->enabled_ = true;
            bb->position_ = get_pos_from_map_ind(map_ind, grid.meta);
            bb->size_ = cell_size;
            float fprob =  0.8 - (float)prob * 0.008;
            bb->color_ = {fprob, fprob, fprob, 1.0f};
        }
        else
        {
            bb->enabled_ = false;
        }
    }
    mp->occ_grid_bb->Commit();
}

intern void update_scene_from_scan(map_panel *mp, const jackal_laser_scan &packet)
{
    float cur_ang = packet.angle_min;
    for (int i = 0; i < jackal_laser_scan::SCAN_POINTS; ++i)
    {
        auto bb = mp->scan_bb->GetBillboard(i);
        if (packet.scan[i] < packet.range_max && packet.scan[i] > packet.range_min)
        {
            bb->position_ = {packet.scan[i] * sin(cur_ang), packet.scan[i] * cos(cur_ang), 0.2f};
            bb->enabled_ = true;
            bb->size_ = {6, 6};
        }
        else
        {
            bb->enabled_ = false;
        }
        cur_ang += packet.angle_increment;
    }
    mp->scan_bb->Commit();
}

void map_panel_init(map_panel *mp, const ui_info &ui_inf, net_connection *conn, input_data * inp)
{
    auto uctxt = ui_inf.ui_sys->GetContext();
    auto cache = uctxt->GetSubsystem<urho::ResourceCache>();

    create_3dview(mp, cache, ui_inf.ui_sys->GetRoot(), uctxt);
    setup_scene(mp, cache, mp->view->GetScene());
    setup_camera_controls(mp, mp->view->GetCameraNode(), inp);

    ss_connect(&mp->router, conn->scan_received, [mp](const jackal_laser_scan &pckt) {
        update_scene_from_scan(mp, pckt);
    });

    ss_connect(&mp->router, conn->occ_grid_update_received, [mp](const occupancy_grid_update &pckt) {
        update_scene_from_occ_grid(mp, pckt);
    });
}

void map_panel_term(map_panel *mp)
{}

void map_panel_run_frame(map_panel *mpanel, net_connection *conn)
{

}
