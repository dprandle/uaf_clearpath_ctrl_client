#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/GraphicsAPI/Texture2D.h>
#include "Urho3D/Graphics/Model.h"
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/BillboardSet.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/View3D.h>

#include <Urho3D/Resource/ResourceCache.h>
#include "Urho3D/Resource/XMLFile.h"

#include <Urho3D/Scene/Scene.h>
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

intern void setup_camera_controls(map_panel *mp, urho::Node *cam_node, input_data *inp)
{
    auto on_mouse_tilt = [cam_node, mp](const itrigger_event &tevent) {
        if (mp->js_enabled || mp->view != tevent.vp.element_under_mouse && tevent.vp.element_under_mouse && tevent.vp.element_under_mouse->GetPriority() > 2)
            return;
        auto rot_node = cam_node;
        auto parent = cam_node->GetParent();
        if (parent)
            rot_node = parent;
        if (tevent.vp.vp_norm_mdelta.x_ > 0.1 || tevent.vp.vp_norm_mdelta.y_ > 0.1)
            return;
        rot_node->Rotate(quat(tevent.vp.vp_norm_mdelta.y_ * 100.0f, {1, 0, 0}));
        rot_node->Rotate(quat(tevent.vp.vp_norm_mdelta.x_ * 100.0f, {0, 0, -1}), urho::TransformSpace::World);
    };

    auto on_mouse_zoom = [cam_node, mp](const itrigger_event &tevent) {
        if (mp->js_enabled || mp->view != tevent.vp.element_under_mouse && tevent.vp.element_under_mouse && tevent.vp.element_under_mouse->GetPriority() > 2)
            return;
        float amount = tevent.wheel;
#if defined(__EMSCRIPTEN__)
        amount *= 0.01f;
#endif
        cam_node->Translate(vec3{0, 0, amount});
    };

    auto on_debug_key = [mp](const itrigger_event &tevent) { ilog("Debug Key"); };

    input_action_trigger it{};
    it.cond.mbutton = MOUSEB_MOVE;
    it.name = urho::StringHash("CamTiltAction").ToHash();
    it.tstate = T_BEGIN;
    it.mb_req = urho::MOUSEB_LEFT;
    it.mb_allowed = 0;
    it.qual_req = 0;
    it.qual_allowed = urho::QUAL_ANY;
    input_add_trigger(&inp->map, it);
    ss_connect(&mp->router, inp->dispatch.trigger, it.name, on_mouse_tilt);

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
    mp->view->SetMaxAnchor({1.0f, 1.0f});

    scene->CreateComponent<urho::DebugRenderer>();
    scene->CreateComponent<urho::Octree>();

    auto cam = cam_node->CreateComponent<urho::Camera>();
    mp->view->SetView(scene, cam, true);
    mp->view->GetViewport()->SetRenderPath(rpath);

    cam_node->SetPosition({0, 0, -10});
    cam_node->SetRotation(quat(-1.0, {1, 0, 0}));
    cam_node->SetRotation(quat(0.0, {1, 0, 0}));

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

intern void create_jackal(map_panel *mp, urho::ResourceCache *cache)
{
    auto jackal_base_model = cache->GetResource<urho::Model>("Models/jackal-base.mdl");
    auto jackal_base_mat = cache->GetResource<urho::Material>("Materials/jackal_base.xml");

    auto jackal_fenders_model = cache->GetResource<urho::Model>("Models/jackal-fenders.mdl");
    auto jackal_fender_mat = cache->GetResource<urho::Material>("Materials/jackal_fender.xml");

    auto jackal_wheel_model = cache->GetResource<urho::Model>("Models/jackal-wheel.mdl");

    // Create node for jackel model stuff
    auto jackal_base_model_node = mp->base_link->CreateChild("jackal_base_model");
    auto smodel = jackal_base_model_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_base_model);
    smodel->SetMaterial(jackal_base_mat);
    jackal_base_model_node->Rotate({90, {0, 0, -1}});
    jackal_base_model_node->Rotate({90, {-1, 0, 0}});

    auto jackal_fender_node = jackal_base_model_node->CreateChild("jackal_fender");
    smodel = jackal_fender_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_fenders_model);
    smodel->SetMaterial(jackal_fender_mat);

    auto fl_wheel_node = mp->node_lut[FRONT_LEFT_WHEEL_LINK]->CreateChild("fl_wheel_model");
    smodel = fl_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_wheel_model);
    smodel->SetMaterial(jackal_base_mat);
    fl_wheel_node->Rotate({90, {-1, 0, 0}});

    auto fr_wheel_node = mp->node_lut[FRONT_RIGHT_WHEEL_LINK]->CreateChild("fr_wheel_model");
    smodel = fr_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_wheel_model);
    smodel->SetMaterial(jackal_base_mat);
    fr_wheel_node->Rotate({90, {-1, 0, 0}});

    auto rl_wheel_node = mp->node_lut[REAR_LEFT_WHEEL_LINK]->CreateChild("rl_wheel_model");
    smodel = rl_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_wheel_model);
    smodel->SetMaterial(jackal_base_mat);
    rl_wheel_node->Rotate({90, {-1, 0, 0}});

    auto rr_wheel_node = mp->node_lut[REAR_RIGHT_WHEEL_LINK]->CreateChild("rr_wheel_model");
    smodel = rr_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_wheel_model);
    smodel->SetMaterial(jackal_base_mat);
    rr_wheel_node->Rotate({90, {-1, 0, 0}});
}

intern void setup_occ_grid_map(occ_grid_map * map, const char *node_name, urho::ResourceCache *cache, urho::Scene *scene, urho::Context *uctxt)
{
    auto occ_mat = cache->GetResource<urho::Material>("Materials/" + urho::String(node_name) + "_billboard.xml");

    map->rend_texture = new urho::Texture2D(uctxt);
    map->image = new urho::Image(uctxt);

    // Main localization node - all nodes child of this node..
    map->node = scene->CreateChild(node_name);

    map->rend_texture->SetNumLevels(1);
    occ_mat->SetTexture(urho::TU_DIFFUSE, map->rend_texture);

    map->bb_set = map->node->CreateComponent<urho::BillboardSet>();
    map->bb_set->SetFixedScreenSize(false);
    map->bb_set->SetMaterial(occ_mat);
    map->bb_set->SetFaceCameraMode(urho::FC_NONE);
    
    map->image->SetSize(512, 512, 4);
    for (int y = 0; y < map->image->GetHeight(); ++y)
    {
        for (int x = 0; x < map->image->GetWidth(); ++x)
            map->image->SetPixel(x, y, map->undiscovered);
    }

    map->rend_texture->SetData(map->image);
    map->rend_texture->SetFilterMode(Urho3D::TextureFilterMode::FILTER_NEAREST);
    map->bb_set->SetNumBillboards(1);

    auto bb = map->bb_set->GetBillboard(0);
    bb->enabled_ = true;
    bb->size_ = {25.6f, 25.6f};
    map->bb_set->Commit();
}

intern void setup_scene(map_panel *mp, urho::ResourceCache *cache, urho::Scene *scene, urho::Context *uctxt)
{
    // Grab all resources needed
    auto scan_mat = cache->GetResource<urho::Material>("Materials/scan_billboard.xml");

    setup_occ_grid_map(&mp->map, MAP.c_str(), cache, scene, uctxt);
    mp->node_lut[MAP] = mp->map.node;

    mp->glob_cmap.undiscovered = mp->glob_cmap.free_space;
    mp->glob_cmap.map_type = OCC_GRID_TYPE_COSTMAP;
    setup_occ_grid_map(&mp->glob_cmap, "global_costmap", cache, scene, uctxt);
    mp->glob_cmap.node->Translate({0, 0,-0.01});

    // Odom frame is smoothly moving while map may experience discreet jumps
    mp->odom = mp->map.node->CreateChild(ODOM.c_str());
    mp->node_lut[ODOM] = mp->odom;

    // Base link is main node tied to the jackal base
    mp->base_link = mp->odom->CreateChild(BASE_LINK.c_str());
    mp->node_lut[BASE_LINK] = mp->base_link;

    // Follow camera for the jackal
    auto jackal_follow_cam = mp->base_link->CreateChild("jackal_follow_cam");
    mp->view->GetCameraNode()->SetParent(jackal_follow_cam);
    jackal_follow_cam->Rotate({90, {0, 0, -1}});

    // Child of base link
    auto chassis_link = mp->base_link->CreateChild(CHASSIS_LINK.c_str());
    mp->node_lut[CHASSIS_LINK] = chassis_link;

    auto offset_link = chassis_link->CreateChild("offset_link");
    offset_link->Translate({0.0f, 0.0f, -0.065f});

    // Children of chassis linkg
    mp->node_lut[FRONT_FENDER_LINK] = chassis_link->CreateChild(FRONT_FENDER_LINK.c_str());
    mp->node_lut[FRONT_LEFT_WHEEL_LINK] = offset_link->CreateChild(FRONT_LEFT_WHEEL_LINK.c_str());
    mp->node_lut[FRONT_RIGHT_WHEEL_LINK] = offset_link->CreateChild(FRONT_RIGHT_WHEEL_LINK.c_str());
    mp->node_lut[IMU_LINK] = chassis_link->CreateChild(IMU_LINK.c_str());
    mp->node_lut[NAVSAT_LINK] = chassis_link->CreateChild(NAVSAT_LINK.c_str());
    mp->node_lut[REAR_FENDER_LINK] = chassis_link->CreateChild(REAR_FENDER_LINK.c_str());
    mp->node_lut[REAR_LEFT_WHEEL_LINK] = offset_link->CreateChild(REAR_LEFT_WHEEL_LINK.c_str());
    mp->node_lut[REAR_RIGHT_WHEEL_LINK] = offset_link->CreateChild(REAR_RIGHT_WHEEL_LINK.c_str());
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

    create_jackal(mp, cache);
}

intern ivec2 index_to_texture_coords(u32 index, u32 row_width, u32 height)
{
    return {(int)(index % row_width), (int)height - (int)(index / row_width)};
}

intern void update_scene_map_from_occ_grid(const occ_grid_map &map, const occ_grid_update &grid)
{
    ilog("Got %d changes this frame for map size %dx%d", grid.meta.change_elem_count, grid.meta.width, grid.meta.height);

    ivec2 resized{map.image->GetWidth(), map.image->GetHeight()};
    while (resized.x_ < grid.meta.width)
        resized.x_ *= 2;
    while (resized.x_/2 > grid.meta.width)
        resized.x_ /= 2;
    while (resized.y_ < grid.meta.height)
        resized.y_ *= 2;
    while (resized.y_/2 > grid.meta.height)
        resized.y_ /= 2;

    if (resized != ivec2{map.image->GetWidth(), map.image->GetHeight()})
    {
        ilog("Map resized texture to %d by %d (actual map size %d %d)", resized.x_, resized.y_, grid.meta.width, grid.meta.height);
        map.image->Resize(resized.x_, resized.y_);
        for (int y = 0; y < resized.y_; ++y)
        {
            for (int x = 0; x < resized.x_; ++x)
                map.image->SetPixel(x, y, map.undiscovered);
        }
    }

    quat q = quat_from(grid.meta.origin_orientation);
    vec3 pos = vec3_from(grid.meta.origin_pos);
    map.node->SetRotation(q);

    auto billboard = map.bb_set->GetBillboard(0);
    billboard->size_ = vec2{(float)map.image->GetWidth(), (float)map.image->GetHeight()} * grid.meta.resolution * 0.5;
    float h_diff = billboard->size_.y_ - grid.meta.height * grid.meta.resolution * 0.5;
    billboard->position_ = pos + vec3{billboard->size_};
    billboard->enabled_ = true;

    // Go through and use the arriving delta packet to set the current map values
    for (int i = 0; i < grid.meta.change_elem_count; ++i)
    {
        u32 map_ind = (grid.change_elems[i] >> 8);
        u8 prob = (u8)grid.change_elems[i];
        ivec2 tex_coods = index_to_texture_coords(map_ind, grid.meta.width, map.image->GetHeight());

        if (map.map_type == OCC_GRID_TYPE_MAP)
        {
            if (prob <= 100)
            {
                float fprob = 1.0 - (float)prob * 0.01;
                map.image->SetPixel(tex_coods.x_, tex_coods.y_, {fprob, fprob, fprob, 1.0});
            }
            else
            {
                map.image->SetPixel(tex_coods.x_, tex_coods.y_, map.undiscovered);
            }
        }
        else
        {
            if (prob == 100)
            {
                map.image->SetPixel(tex_coods.x_, tex_coods.y_, map.lethal);
            }
            else if (prob == 99)
            {
                map.image->SetPixel(tex_coods.x_, tex_coods.y_, map.inscribed);
            }
            else if (prob <= 98 && prob >= 50)
            {
                auto col = map.possibly_circumscribed;
                col.a_ -= - (1.0 - (prob - 50.0) / (98.0 - 50.0)) * 0.4;
                map.image->SetPixel(tex_coods.x_, tex_coods.y_, col);
            }
            else if (prob <= 50 && prob >= 1)
            {
                auto col = map.no_collision;
                col.a_ -= (1.0 - (prob - 1.0) / (50.0 - 1.0)) * 0.4;
                map.image->SetPixel(tex_coods.x_, tex_coods.y_, col);
            }
            else
            {
                map.image->SetPixel(tex_coods.x_, tex_coods.y_, map.free_space);
            }
        }
    }

    map.bb_set->Commit();
    map.rend_texture->SetData(map.image);
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

    auto node_parent = node->GetParent();
    urho::Node *node_parent_parent = nullptr;
    if (node_parent)
        node_parent_parent = node_parent->GetParent();

    if (node_parent != parent && node_parent_parent != parent)
    {
        wlog("Received update for node %s with different parent than received in packet (%s)", tform.name, tform.parent_name);
    }

    node->SetPositionSilent(vec3_from(tform.pos));
    node->SetRotationSilent(quat_from(tform.orientation));
}

intern void map_panel_run_frame(map_panel *mp, float dt)
{
    static float counter = 0.0f;
    counter += dt;
    if (counter >= 0.030f)
    {
        mp->odom->MarkDirty();
        counter = 0.0f;
    }

    if (mp->cam_move_widget.world_trans != vec3::ZERO)
    {
        mp->view->GetCameraNode()->Translate(mp->cam_move_widget.world_trans * dt * 10, Urho3D::TransformSpace::World);
    }

    if (mp->cam_zoom_widget.loc_trans != vec3::ZERO)
    {
        mp->view->GetCameraNode()->Translate(mp->cam_zoom_widget.loc_trans * dt * 20);
    }
}

intern void setup_cam_zoom_widget(map_panel *mp, const ui_info &ui_inf)
{
    auto uctxt = ui_inf.ui_sys->GetContext();
    auto cache = uctxt->GetSubsystem<urho::ResourceCache>();

    mp->cam_zoom_widget.widget = new urho::BorderImage(uctxt);
    ui_inf.ui_sys->GetRoot()->AddChild(mp->cam_zoom_widget.widget);
    mp->cam_zoom_widget.widget->SetStyle("ZoomButtonGroup", ui_inf.style);

    mp->cam_zoom_widget.zoom_in = new urho::Button(uctxt);
    mp->cam_zoom_widget.widget->AddChild(mp->cam_zoom_widget.zoom_in);
    mp->cam_zoom_widget.zoom_in->SetStyle("ZoomInButton", ui_inf.style);

    auto offset = mp->cam_zoom_widget.zoom_in->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv * 0.75;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv * 0.75;
    auto parent_offset = offset;
    parent_offset.y_ = offset.y_ * 2 + 10;
    mp->cam_zoom_widget.widget->SetMaxOffset(parent_offset);
    mp->cam_zoom_widget.zoom_in->SetMaxOffset(offset);

    mp->cam_zoom_widget.zoom_out = new urho::Button(uctxt);
    mp->cam_zoom_widget.widget->AddChild(mp->cam_zoom_widget.zoom_out);
    mp->cam_zoom_widget.zoom_out->SetStyle("ZoomOutButton", ui_inf.style);
    mp->cam_zoom_widget.zoom_out->SetMaxOffset(offset);

    mp->cam_zoom_widget.widget->SubscribeToEvent(urho::E_PRESSED, [mp](urho::StringHash type, urho::VariantMap &ev_data) {
        auto elem = (urho::UIElement *)ev_data[urho::ClickEnd::P_ELEMENT].GetPtr();
        if (elem == mp->cam_zoom_widget.zoom_in)
            mp->cam_zoom_widget.loc_trans = mp->view->GetCameraNode()->GetDirection();
        else if (elem == mp->cam_zoom_widget.zoom_out)
            mp->cam_zoom_widget.loc_trans = mp->view->GetCameraNode()->GetDirection() * -1;
    });

    mp->cam_zoom_widget.widget->SubscribeToEvent(urho::E_CLICKEND, [mp](urho::StringHash type, urho::VariantMap &ev_data) { mp->cam_zoom_widget.loc_trans = {}; });
}

intern void setup_cam_move_widget(map_panel *mp, const ui_info &ui_inf)
{
    auto uctxt = ui_inf.ui_sys->GetContext();
    auto cache = uctxt->GetSubsystem<urho::ResourceCache>();

    mp->cam_move_widget.widget = new urho::BorderImage(uctxt);
    ui_inf.ui_sys->GetRoot()->AddChild(mp->cam_move_widget.widget);
    mp->cam_move_widget.widget->SetStyle("ArrowButtonGroup", ui_inf.style);

    mp->cam_move_widget.forward = new urho::Button(uctxt);
    mp->cam_move_widget.widget->AddChild(mp->cam_move_widget.forward);
    mp->cam_move_widget.forward->SetStyle("ArrowButtonForward", ui_inf.style);
    mp->cam_move_widget.forward->SetVar("md", 1);

    auto offset = mp->cam_move_widget.forward->GetImageRect().Size();
    offset.x_ *= ui_inf.dev_pixel_ratio_inv * 0.75;
    offset.y_ *= ui_inf.dev_pixel_ratio_inv * 0.75;
    auto parent_offset = offset;
    parent_offset.x_ = offset.x_ * 3 + 20;
    parent_offset.y_ = offset.y_ * 2 + 10;
    mp->cam_move_widget.widget->SetMaxOffset(parent_offset);
    mp->cam_move_widget.forward->SetMaxOffset(offset);

    mp->cam_move_widget.back = new urho::Button(uctxt);
    mp->cam_move_widget.widget->AddChild(mp->cam_move_widget.back);
    mp->cam_move_widget.back->SetStyle("ArrowButtonBack", ui_inf.style);
    mp->cam_move_widget.back->SetMaxOffset(offset);
    mp->cam_move_widget.back->SetVar("md", -1);

    mp->cam_move_widget.left = new urho::Button(uctxt);
    mp->cam_move_widget.widget->AddChild(mp->cam_move_widget.left);
    mp->cam_move_widget.left->SetStyle("ArrowButtonLeft", ui_inf.style);
    mp->cam_move_widget.left->SetMaxOffset(offset);
    mp->cam_move_widget.left->SetVar("md", -2);

    mp->cam_move_widget.right = new urho::Button(uctxt);
    mp->cam_move_widget.widget->AddChild(mp->cam_move_widget.right);
    mp->cam_move_widget.right->SetStyle("ArrowButtonRight", ui_inf.style);
    mp->cam_move_widget.right->SetMaxOffset(offset);
    mp->cam_move_widget.right->SetVar("md", 2);

    mp->cam_move_widget.widget->SubscribeToEvent(urho::E_PRESSED, [mp](urho::StringHash type, urho::VariantMap &ev_data) {
        auto elem = (urho::UIElement *)ev_data[urho::ClickEnd::P_ELEMENT].GetPtr();
        auto trans = elem->GetVar("md").GetInt();
        if (trans == 0)
            return;

        auto cam_node = mp->view->GetCameraNode();
        auto world_right = cam_node->GetWorldRight().Normalized();
        auto world_up = cam_node->GetWorldUp().Normalized();
        float angle = std::abs(world_up.Angle({0, 0, 1}) - 90.0f);
        if (std::abs(angle) > 70)
        {
            mp->cam_move_widget.world_trans = vec3(0, 0, -trans);
            if (std::abs(trans) == 2)
                mp->cam_move_widget.world_trans = world_right * trans / 2;
        }
        else
        {
            mp->cam_move_widget.world_trans = vec3(world_up.x_, world_up.y_, 0.0) * trans;
            if (std::abs(trans) == 2)
                mp->cam_move_widget.world_trans = world_right * trans / 2;
        }
    });

    mp->cam_move_widget.widget->SubscribeToEvent(urho::E_CLICKEND, [mp](urho::StringHash type, urho::VariantMap &ev_data) { mp->cam_move_widget.world_trans = {}; });
}

void map_panel_init(map_panel *mp, const ui_info &ui_inf, net_connection *conn, input_data *inp)
{
    auto uctxt = ui_inf.ui_sys->GetContext();
    auto cache = uctxt->GetSubsystem<urho::ResourceCache>();

    create_3dview(mp, cache, ui_inf.ui_sys->GetRoot(), uctxt);
    setup_scene(mp, cache, mp->view->GetScene(), uctxt);
    setup_camera_controls(mp, mp->view->GetCameraNode(), inp);
    setup_cam_move_widget(mp, ui_inf);
    setup_cam_zoom_widget(mp, ui_inf);

    ss_connect(&mp->router, conn->scan_received, [mp](const sicklms_laser_scan &pckt) { update_scene_from_scan(mp, pckt); });

    ss_connect(&mp->router, conn->map_update_received, [mp](const occ_grid_update &pckt) { update_scene_map_from_occ_grid(mp->map, pckt); });

    ss_connect(&mp->router, conn->glob_cm_update_received, [mp](const occ_grid_update &pckt) { 
        ilog("Got costmap update");
        update_scene_map_from_occ_grid(mp->glob_cmap, pckt); });

    ss_connect(&mp->router, conn->transform_updated, [mp](const node_transform &pckt) { update_node_transform(mp, pckt); });

    mp->view->SubscribeToEvent(urho::E_UPDATE, [mp](urho::StringHash type, urho::VariantMap &ev_data) {
        auto dt = ev_data[urho::Update::P_TIMESTEP].GetFloat();
        map_panel_run_frame(mp, dt);
    });
}

void map_panel_term(map_panel *mp)
{}
