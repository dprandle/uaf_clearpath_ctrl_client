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

#include <Urho3D/GraphicsAPI/Texture2D.h>
#include <Urho3D/Graphics/Geometry.h>
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

#include "Urho3D/Scene/Node.h"
#include "input.h"
#include "jackal_control.h"
#include "mapping.h"
#include "math_utils.h"
#include "network.h"

const std::string MAP{"map"};
const std::string ODOM{"odom"};
const std::string BASE_LINK{"base_link"};
const std::string CHASSIS_LINK{"chassis_link"};
const std::string FRONT_FENDER_LINK{"front_fender_link"};
const std::string FRONT_LEFT_WHEEL_LINK{"front_left_wheel_link"};
const std::string FRONT_RIGHT_WHEEL_LINK{"front_right_wheel_link"};
const std::string IMU_LINK{"imu_link"};
const std::string MID_MOUNT{"mid_mount"};
const std::string NAVSAT_LINK{"navsat_link"};
const std::string REAR_FENDER_LINK{"rear_fender_link"};
const std::string REAR_LEFT_WHEEL_LINK{"rear_left_wheel_link"};
const std::string REAR_RIGHT_WHEEL_LINK{"rear_right_wheel_link"};
const std::string REAR_MOUNT{"rear_mount"};
const std::string FRONT_MOUNT{"front_mount"};
const std::string FRONT_LASER_MOUNT{"front_laser_mount"};
const std::string FRONT_LASER{"front_laser"};

intern const Color NO_GRID_COLOR = {0,0.7,0.7,1.0};


intern void setup_camera_controls(map_panel *mp, urho::Node *cam_node, input_data *inp)
{
    auto on_mouse_move = [cam_node](const itrigger_event &tevent) {
        auto world_dir = cam_node->GetWorldDirection();
        auto world_right = cam_node->GetWorldRight();
        if (std::abs(world_dir.Angle({0, 0, 1}) - 90.0f) < 20)
        {
            auto loc_trans = vec3(tevent.vp.vp_norm_mdelta.x_, {}, {}) * -20.0f;
            auto world_trans = vec3({}, {}, tevent.vp.vp_norm_mdelta.y_) * -20.0f;
            cam_node->Translate(loc_trans, Urho3D::TransformSpace::Local);
            cam_node->Translate(world_trans, Urho3D::TransformSpace::World);
        }
        else
        {
            auto world_dir_no_z_norm = vec3(world_dir.x_, world_dir.y_, 0.0);
            world_dir_no_z_norm.Normalize();
            auto trans = world_right * -tevent.vp.vp_norm_mdelta.x_ + world_dir_no_z_norm * tevent.vp.vp_norm_mdelta.y_;
            cam_node->Translate(trans * 20.0f, Urho3D::TransformSpace::World);
        }
    };

    auto on_mouse_tilt = [cam_node](const itrigger_event &tevent) {
        auto rot_node = cam_node;
        auto parent = cam_node->GetParent();
        if (parent)
            rot_node = parent;
        rot_node->Rotate(quat(tevent.vp.vp_norm_mdelta.y_ * 100.0f, {1, 0, 0}));
        rot_node->Rotate(quat(tevent.vp.vp_norm_mdelta.x_ * 100.0f, {0, 0, -1}), urho::TransformSpace::World);
    };

    auto on_mouse_zoom = [cam_node](const itrigger_event &tevent) { 
        float amount = tevent.wheel;
        #if defined(__EMSCRIPTEN__)
        amount *= 0.1f;
        #endif        
        cam_node->Translate(vec3{0, 0, amount}); 
        };

    auto on_debug_key = [mp](const itrigger_event &tevent) { ilog("Debug Key"); };

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

    it.cond.mbutton = MOUSEB_MOVE;
    it.name = urho::StringHash("CamTiltAction").ToHash();
    it.tstate = T_BEGIN;
    it.mb_req = urho::MOUSEB_MIDDLE;
    it.mb_allowed = 0;
    it.qual_req = 0;
    it.qual_allowed = urho::QUAL_ANY;
    input_add_trigger(&inp->map, it);
    ss_connect(&mp->router, inp->dispatch.trigger, it.name, on_mouse_tilt);

    it.cond.mbutton = MOUSEB_WHEEL;
    it.name = urho::StringHash("Zoom").ToHash();
    it.tstate = T_BEGIN;
    it.mb_req = 0;
    it.mb_allowed = 0;
    it.qual_req = 0;
    it.qual_allowed = 0;
    input_add_trigger(&inp->map, it);
    ss_connect(&mp->router, inp->dispatch.trigger, it.name, on_mouse_zoom);

    it.cond = {};
    it.cond.key = urho::KEY_D;
    it.name = urho::StringHash("DebugButton").ToHash();
    it.tstate = T_BEGIN;
    input_add_trigger(&inp->map, it);
    ss_connect(&mp->router, inp->dispatch.trigger, it.name, on_debug_key);

    viewport_element vp{mp->view->GetViewport(), mp->view};
    inp->dispatch.vp_stack.emplace_back(vp);
}

intern void create_3dview(map_panel *mp, urho::ResourceCache *cache, urho::UIElement *root, urho::Context *uctxt)
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

    cam_node->SetPosition({0, 2.5, -10});

    auto zone_node = scene->CreateChild("Zone");
    auto zone = zone_node->CreateComponent<urho::Zone>();
    zone->SetBoundingBox({-1000.0f, 1000.0f});
    zone->SetAmbientColor({0.3f, 0.3f, 0.3f});
    zone->SetFogColor({0.4f, 0.4f, 0.4f});
    zone->SetFogStart(10.0f);
    zone->SetFogEnd(200.0f);

    // Create a directional light
    auto light_node = scene->CreateChild("Dir_Light");
    light_node->SetDirection({0.0f, 0.5f, 1.0f});
    auto *light = light_node->CreateComponent<urho::Light>();
    light->SetLightType(urho::LIGHT_DIRECTIONAL);
    light->SetSpecularIntensity(5.0f);
    light->SetBrightness(0.7f);
}

intern void setup_scene(map_panel *mp, urho::ResourceCache *cache, urho::Scene *scene)
{
    // Grab all resources needed
    auto jackal_base = cache->GetResource<urho::Model>("Models/jackal-base.mdl");
    auto scan_mat = cache->GetResource<urho::Material>("Materials/scan_billboard.xml");
    auto occ_mat = cache->GetResource<urho::Material>("Materials/occ_billboard.xml");
    mp->map_text = new urho::Texture2D(mp->view->GetContext());
    mp->map_image = new urho::Image(mp->view->GetContext());

    quat rot{};
    rot.FromAngleAxis(180.0, {1, 0, 0});
    auto root = scene->CreateChild("root");
    //root->SetRotation(rot);

    // Main localization node - all nodes child of this node..
    // TODO: This should likely be created only upon first receiving a scan?
    mp->map = root->CreateChild(MAP.c_str());

    mp->map_text->SetNumLevels(1);
    occ_mat->SetTexture(urho::TU_DIFFUSE, mp->map_text);
    mp->occ_grid_bb = mp->map->CreateComponent<urho::BillboardSet>();
    mp->occ_grid_bb->SetFixedScreenSize(false);
    mp->occ_grid_bb->SetMaterial(occ_mat);
    mp->occ_grid_bb->SetFaceCameraMode(urho::FC_NONE);
    mp->map_image->SetSize(1024,1024,4);
    for (int y = 0; y < mp->map_image->GetHeight(); ++y)
    {
        for (int x = 0; x < mp->map_image->GetWidth(); ++x)
            mp->map_image->SetPixel(x,y, NO_GRID_COLOR);
    }

    mp->map_text->SetData(mp->map_image);
    mp->occ_grid_bb->SetNumBillboards(1);
    auto bb = mp->occ_grid_bb->GetBillboard(0);
    bb->enabled_ = true;
    bb->size_ = {10, 10};
    mp->occ_grid_bb->Commit();
    mp->node_lut[MAP] = mp->map;

    // Odom frame is smoothly moving while map may experience discreet jumps
    auto odom = mp->map->CreateChild(ODOM.c_str());
    mp->node_lut[ODOM] = odom;

    // Base link is main node tied to the jackal base
    mp->base_link = odom->CreateChild(BASE_LINK.c_str());
    mp->node_lut[BASE_LINK] = mp->base_link;

    // Create node for jackel model stuff
    auto jackal_model = mp->base_link->CreateChild("jackal_model");
    auto smodel = jackal_model->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_base);
    jackal_model->Rotate({90, {0, 0, -1}});
    jackal_model->Rotate({90, {-1, 0, 0}});

    auto jackal_follow_cam = mp->base_link->CreateChild("jackal_follow_cam");
    mp->view->GetCameraNode()->SetParent(jackal_follow_cam);
    jackal_follow_cam->Rotate({90, {0, 0, -1}});

    // Child of base link
    auto chassis_link = mp->base_link->CreateChild(CHASSIS_LINK.c_str());
    mp->node_lut[CHASSIS_LINK] = chassis_link;

    // Children of chassis linkg
    mp->node_lut[FRONT_FENDER_LINK] = chassis_link->CreateChild(FRONT_FENDER_LINK.c_str());
    mp->node_lut[FRONT_LEFT_WHEEL_LINK] = chassis_link->CreateChild(FRONT_LEFT_WHEEL_LINK.c_str());
    mp->node_lut[FRONT_RIGHT_WHEEL_LINK] = chassis_link->CreateChild(FRONT_RIGHT_WHEEL_LINK.c_str());
    mp->node_lut[IMU_LINK] = chassis_link->CreateChild(IMU_LINK.c_str());
    mp->node_lut[NAVSAT_LINK] = chassis_link->CreateChild(NAVSAT_LINK.c_str());
    mp->node_lut[REAR_FENDER_LINK] = chassis_link->CreateChild(REAR_FENDER_LINK.c_str());
    mp->node_lut[REAR_LEFT_WHEEL_LINK] = chassis_link->CreateChild(REAR_LEFT_WHEEL_LINK.c_str());
    mp->node_lut[REAR_RIGHT_WHEEL_LINK] = chassis_link->CreateChild(REAR_RIGHT_WHEEL_LINK.c_str());
    auto mid_mount = chassis_link->CreateChild(MID_MOUNT.c_str());
    mp->node_lut[MID_MOUNT] = mid_mount;

    // Children of mid mount
    mp->node_lut[REAR_MOUNT] = mid_mount->CreateChild(REAR_MOUNT.c_str());
    auto front_mount = mid_mount->CreateChild(FRONT_MOUNT.c_str());
    mp->node_lut[FRONT_MOUNT] = front_mount;

    // Child of front mount
    auto front_laser_mount = front_mount->CreateChild(FRONT_LASER_MOUNT.c_str());
    mp->node_lut[FRONT_LASER_MOUNT] = front_laser_mount;

    // Child of front_laser_mount - This also has our billboard set for the scan
    mp->front_laser = front_laser_mount->CreateChild(FRONT_LASER.c_str());
    mp->scan_bb = mp->front_laser->CreateComponent<urho::BillboardSet>();
    mp->scan_bb->SetMaterial(scan_mat);
    mp->scan_bb->SetFixedScreenSize(true);
    mp->node_lut[FRONT_LASER] = mp->front_laser;

    auto niter = mp->node_lut.begin();
    while (niter != mp->node_lut.end())
    {
        mp->node_updates[niter->second] = {};
        ++niter;
    }
}

intern ivec2 index_to_texture_coords(u32 index, u32 row_width, u32 height)
{
    return {(int)(index % row_width), (int)height - (int)(index / row_width)};
}

intern void update_scene_from_occ_grid(map_panel *mp, const occupancy_grid_update &grid)
{
    ilog("Got %d changes this frame for map size %dx%d", grid.meta.change_elem_count, grid.meta.width, grid.meta.height);

    bool resized = false;
    while (mp->map_image->GetWidth() < grid.meta.width || mp->map_image->GetHeight() < grid.meta.height)
    {
        ilog("Image size is %d %d",mp->map_image->GetWidth(), mp->map_image->GetHeight());
        mp->map_image->SetSize(mp->map_image->GetWidth()*2, mp->map_image->GetHeight()*2, 3);
        ilog("Image size is now %d %d",mp->map_image->GetWidth(), mp->map_image->GetHeight());
        resized = true;
    }
    if (resized)
    {
        ilog("Map resized texture to %d by %d (actual map size %d %d)", mp->map_image->GetWidth(), mp->map_image->GetHeight(), grid.meta.width, grid.meta.height);
        for (int y = 0; y < mp->map_image->GetHeight(); ++y)
        {
            for (int x = 0; x < mp->map_image->GetWidth(); ++x)
                mp->map_image->SetPixel(x,y, NO_GRID_COLOR);
        }
    }

    quat q = quat_from(grid.meta.origin_orientation);
    vec3 pos = vec3_from(grid.meta.origin_pos);
    mp->map->SetRotation(q);

    auto map_bb = mp->occ_grid_bb->GetBillboard(0);
    map_bb->size_ = vec2{(float)mp->map_image->GetWidth(), (float)mp->map_image->GetHeight()} * grid.meta.resolution * 0.5;
    float h_diff = map_bb->size_.y_ - grid.meta.height * grid.meta.resolution * 0.5;
    map_bb->position_ = pos + vec3{map_bb->size_};
    map_bb->enabled_ = true;

    // Go through and use the arriving delta packet to set the current map values
    for (int i = 0; i < grid.meta.change_elem_count; ++i)
    {
        u32 map_ind = (grid.change_elems[i] >> 8);
        u8 prob = (u8)grid.change_elems[i];
        ivec2 tex_coods = index_to_texture_coords(map_ind, grid.meta.width, mp->map_image->GetHeight());

        if (prob <= 100)
        {
            float fprob = 1.0 - (float)prob * 0.01;
            mp->map_image->SetPixel(tex_coods.x_, tex_coods.y_, {fprob, fprob, fprob, 1.0f});
        }
        else
        {
            mp->map_image->SetPixel(tex_coods.x_, tex_coods.y_, NO_GRID_COLOR);
        }
    }

    mp->occ_grid_bb->Commit();
    mp->map_text->SetData(mp->map_image);
}

intern void update_scene_from_scan(map_panel *mp, const sicklms_laser_scan &packet)
{
    sizet range_count = sicklms_get_range_count(packet.meta);
    mp->scan_bb->SetNumBillboards(range_count);

    float cur_ang = packet.meta.angle_min;
    for (int i = 0; i < range_count; ++i)

    {
        auto bb = mp->scan_bb->GetBillboard(i);
        if (packet.ranges[i] < packet.meta.range_max && packet.ranges[i] > packet.meta.range_min)
        {
            bb->position_ = {packet.ranges[i] * cos(cur_ang), packet.ranges[i] * sin(cur_ang), 0.0f};
            bb->enabled_ = true;
            bb->size_ = {6, 6};
        }
        else
        {
            bb->enabled_ = false;
        }
        cur_ang += packet.meta.angle_increment;
    }
    mp->scan_bb->Commit();
}

intern void update_node_transform(map_panel *mp, const node_transform &tform)
{
    urho::Node *node{}, *parent{};
    auto node_fiter = mp->node_lut.find(tform.name);
    if (node_fiter == mp->node_lut.end())
    {
        wlog("Could not find node %s in scene tree despitre getting node update packet", tform.name);
        return;
    }

    node = node_fiter->second;

    auto node_parent_fiter = mp->node_lut.find(tform.parent_name);
    if (node_parent_fiter != mp->node_lut.end())
        parent = node_parent_fiter->second;

    if (node->GetParent() != parent)
    {
        wlog("Received update for node %s with different parent than received in packet (%s)", tform.name, tform.parent_name);
    }

    node->SetPosition(vec3_from(tform.pos));
    node->SetRotation(quat_from(tform.orientation));
}

intern void map_panel_run_frame(map_panel *mp, float dt)
{
    // auto deb_r = mp->view->GetScene()->GetComponent<urho::DebugRenderer>();
    // deb_r->AddLine(vec3{orig.x_,orig.y_,-0.2}, {0,0,-0.2}, {1,0,0});
    // deb_r->AddLine(orig, orig + vec3{(float)mp->map_image->GetWidth(), 0.0f, 0} * res, {0,0,1});
    // //deb_r->AddLine(orig, orig + vec3{(float)mp->map_image->GetWidth(), 0.0f, 0} * res, {0,0,1});

    // auto render = mp->view->GetContext()->GetSubsystem<urho::Renderer>();
    // render->DrawDebugGeometry(true);
}

void map_panel_init(map_panel *mp, const ui_info &ui_inf, net_connection *conn, input_data *inp)
{
    auto uctxt = ui_inf.ui_sys->GetContext();
    auto cache = uctxt->GetSubsystem<urho::ResourceCache>();

    create_3dview(mp, cache, ui_inf.ui_sys->GetRoot(), uctxt);
    setup_scene(mp, cache, mp->view->GetScene());
    setup_camera_controls(mp, mp->view->GetCameraNode(), inp);

    ss_connect(&mp->router, conn->scan_received, [mp](const sicklms_laser_scan &pckt) { update_scene_from_scan(mp, pckt); });

    ss_connect(&mp->router, conn->occ_grid_update_received, [mp](const occupancy_grid_update &pckt) { update_scene_from_occ_grid(mp, pckt); });

    ss_connect(&mp->router, conn->transform_updated, [mp](const node_transform &pckt) { update_node_transform(mp, pckt); });

    mp->view->SubscribeToEvent(urho::E_UPDATE, [mp](urho::StringHash type, urho::VariantMap &ev_data) {
        auto dt = ev_data[urho::Update::P_TIMESTEP].GetFloat();
        map_panel_run_frame(mp, dt);
    });
}

void map_panel_term(map_panel *mp)
{}
