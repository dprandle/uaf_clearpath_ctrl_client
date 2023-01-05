#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/GraphicsAPI/Texture2D.h>
#include "Urho3D/Graphics/Model.h"
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/View.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/BillboardSet.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/View3D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include "Urho3D/Resource/XMLFile.h"
#include <Urho3D/Scene/Scene.h>
#include "Urho3D/Scene/Node.h"

#include "input.h"
#include "mapping.h"
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

intern const ogmap_colors map_colors{.undiscovered{0, 0.7, 0.7, 1}};

intern const ogmap_colors loc_cmc_colors{.lethal{1, 0, 0.7, 0.7},
                                         .inscribed{0, 0.7, 1, 0.7},
                                         .possibly_circumscribed{0.7, 0, 0, 0.7},
                                         .no_collision{0, 0.7, 0, 0.7}};

intern const ogmap_colors glob_cmc_colors{.lethal{1, 0, 1, 0.7}, .inscribed{0, 1, 1, 0.7}, .possibly_circumscribed{1, 0, 0, 0.7}, .no_collision{0, 1, 0, 0.7}};

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

    auto dbg = scene->CreateComponent<urho::DebugRenderer>();
    dbg->SetLineAntiAlias(true);
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

intern void setup_occ_grid_map(occ_grid_map *map, const char *node_name, urho::ResourceCache *cache, urho::Scene *scene, urho::Context *uctxt)
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
    map->rend_texture->SetFilterMode(Urho3D::TextureFilterMode::FILTER_NEAREST);
    map->bb_set->SetNumBillboards(1);

    map_clear_occ_grid(map);

    auto bb = map->bb_set->GetBillboard(0);
    bb->enabled_ = true;
    bb->size_ = {25.6f, 25.6f};
    map->bb_set->Commit();
}

intern void setup_scene(map_panel *mp, urho::ResourceCache *cache, urho::Scene *scene, urho::Context *uctxt)
{
    // Grab all resources needed
    auto scan_mat = cache->GetResource<urho::Material>("Materials/scan_billboard.xml");

    mp->map.cols = map_colors;
    setup_occ_grid_map(&mp->map, MAP.c_str(), cache, scene, uctxt);
    mp->node_lut[MAP] = mp->map.node;

    mp->glob_cmap.cols = glob_cmc_colors;
    mp->glob_cmap.map_type = OCC_GRID_TYPE_COSTMAP;
    setup_occ_grid_map(&mp->glob_cmap, "global_costmap", cache, scene, uctxt);
    mp->glob_cmap.node->Translate({0, 0, -0.01});

    mp->loc_cmap.cols = loc_cmc_colors;
    mp->loc_cmap.map_type = OCC_GRID_TYPE_COSTMAP;
    setup_occ_grid_map(&mp->loc_cmap, "local_costmap", cache, scene, uctxt);
    mp->loc_cmap.node->Translate({0, 0, -0.02});

    mp->loc_npview.color = {1.0f, 0.6f, 0.0f, 1.0f};

    // Odom frame is smoothly moving while map may experience discreet jumps
    mp->odom = mp->map.node->CreateChild(ODOM.c_str());
    mp->node_lut[ODOM] = mp->odom;

    // Base link is main node tied to the jackal base
    mp->base_link = mp->odom->CreateChild(BASE_LINK.c_str());
    mp->node_lut[BASE_LINK] = mp->base_link;

    // Follow camera for the jackal
    auto jackal_follow_cam = mp->base_link->CreateChild("jackal_follow_cam");
    mp->view->GetCameraNode()->SetParent(jackal_follow_cam);
    jackal_follow_cam->SetRotation({90, {0, 0, -1}});

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

intern void update_scene_map_from_occ_grid(occ_grid_map *map, const occ_grid_update &grid)
{
    ivec2 resized{map->image->GetWidth(), map->image->GetHeight()};
    while (resized.x_ < grid.meta.width)
        resized.x_ *= 2;
    while (resized.x_ / 2 > grid.meta.width)
        resized.x_ /= 2;
    while (resized.y_ < grid.meta.height)
        resized.y_ *= 2;
    while (resized.y_ / 2 > grid.meta.height)
        resized.y_ /= 2;

    if (resized != ivec2{map->image->GetWidth(), map->image->GetHeight()} || grid.meta.reset_map == 1)
    {
        ilog("Map resized texture to %d by %d (actual map size %d %d)", resized.x_, resized.y_, grid.meta.width, grid.meta.height);
        map->image->Resize(resized.x_, resized.y_);
        for (int y = 0; y < resized.y_; ++y)
        {
            for (int x = 0; x < resized.x_; ++x)
                map->image->SetPixel(x, y, map->cols.undiscovered);
        }
    }

    quat q = quat_from(grid.meta.origin_p.orientation);
    vec3 pos = vec3_from(grid.meta.origin_p.pos);
    map->node->SetRotation(q);

    auto billboard = map->bb_set->GetBillboard(0);
    billboard->size_ = vec2{(float)map->image->GetWidth(), (float)map->image->GetHeight()} * grid.meta.resolution * 0.5;
    float h_diff = billboard->size_.y_ - grid.meta.height * grid.meta.resolution * 0.5;
    billboard->position_ = pos + vec3{billboard->size_};
    billboard->enabled_ = true;

    // Go through and use the arriving delta packet to set the current map values
    for (int i = 0; i < grid.meta.change_elem_count; ++i)
    {
        u32 map_ind = (grid.change_elems[i] >> 8);
        u8 prob = (u8)grid.change_elems[i];
        ivec2 tex_coods = index_to_texture_coords(map_ind, grid.meta.width, map->image->GetHeight());

        if (map->map_type == OCC_GRID_TYPE_MAP)
        {
            if (prob <= 100)
            {
                float fprob = 1.0 - (float)prob * 0.01;
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, {fprob, fprob, fprob, 1.0});
            }
            else
            {
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, map->cols.undiscovered);
            }
        }
        else
        {
            if (prob == 100)
            {
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, map->cols.lethal);
            }
            else if (prob == 99)
            {
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, map->cols.inscribed);
            }
            else if (prob <= 98 && prob >= 50)
            {
                auto col = map->cols.possibly_circumscribed;
                col.a_ -= -(1.0 - (prob - 50.0) / (98.0 - 50.0)) * 0.4;
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, col);
            }
            else if (prob <= 50 && prob >= 1)
            {
                auto col = map->cols.no_collision;
                col.a_ -= (1.0 - (prob - 1.0) / (50.0 - 1.0)) * 0.4;
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, col);
            }
            else
            {
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, map->cols.free_space);
            }
        }
    }

    map->bb_set->Commit();
    map->rend_texture->SetData(map->image);
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

intern void update_glob_nav_path(map_panel *mp, const nav_path &np)
{
    mp->glob_npview.entry_count = np.path_cnt;
    for (int i = 0; i < np.path_cnt; ++i)
        mp->glob_npview.path_entries[i] = vec3_from(np.path[i].pos);
}

intern void update_loc_nav_path(map_panel *mp, const nav_path &np)
{
    mp->loc_npview.entry_count = np.path_cnt;
    for (int i = 0; i < np.path_cnt; ++i)
        mp->loc_npview.path_entries[i] = vec3_from(np.path[i].pos);
}

intern void update_cur_goal_status(map_panel *mp, const current_goal_status &gs)
{
    if (mp->goals.cur_goal_status != -2 || gs.status == 0 || gs.status == 1)
    {
        mp->goals.cur_goal = vec3_from(gs.goal_p.pos);
        mp->goals.cur_goal_status = gs.status;
    }
}

intern float animate_marker_circles(goal_marker_info *gm, float dt)
{
    static bool increasing = true;
    if (increasing && gm->cur_anim_time < gm->loop_anim_time)
    {
        gm->cur_anim_time += dt;
        if (gm->cur_anim_time > gm->loop_anim_time)
            increasing = false;
    }
    else
    {
        gm->cur_anim_time -= dt;
        if (gm->cur_anim_time < 0)
            increasing = true;
    }

    float rad = (gm->max_rad - gm->min_rad) * (gm->cur_anim_time / gm->loop_anim_time) + gm->min_rad;
    return rad;
}

intern void draw_nav_path(const nav_path_view &np, urho::DebugRenderer *dbg)
{
    if (np.enabled)
    {
        for (int i = 1; i < np.entry_count; ++i)
            dbg->AddLine(np.path_entries[i - 1], np.path_entries[i], np.color);
    }
}

intern void draw_measure_path(const measure_points &mp, urho::DebugRenderer *dbg)
{
    float path_len = 0.0f;
    for (int i = 0; i < mp.entry_count; ++i)
    {
        if (i > 0)
        {
            dbg->AddLine(mp.entries[i - 1], mp.entries[i], mp.color);
            path_len += (mp.entries[i - 1]- mp.entries[i]).Length();
        }
        dbg->AddCircle(mp.entries[i], {0,0,-1}, mp.marker_rad, mp.color);
    }
    mp.path_len_text->SetVisible(path_len > FLOAT_EPS);
    if (path_len > FLOAT_EPS)
    {
        path_len *= METERS_TO_FEET;
        int feet = int(path_len);
        path_len -= feet;
        int inches = int(path_len*12.0);

        urho::String text;
        text.AppendWithFormat("%d'%d\"", feet, inches);
        mp.path_len_text->SetText(text);
    }
}

intern void update_and_draw_nav_goals(map_panel *mp, float dt, urho::DebugRenderer *dbg, net_connection *conn)
{
    float marker_rad = animate_marker_circles(&mp->glob_npview.goal_marker, dt);
    if (mp->goals.cur_goal_status == 0 || mp->goals.cur_goal_status == 1 || mp->goals.cur_goal_status == -2)
    {
        dbg->AddCircle(mp->goals.cur_goal, {0, 0, -1}, marker_rad, mp->glob_npview.goal_marker.color);
        auto dist_to_goal = (mp->base_link->GetWorldPosition() - mp->goals.cur_goal).Length();
        if (dist_to_goal < 0.25)
        {
            command_stop stop{};
            net_tx(*conn, stop);
        }
    }
    else
    {
        mp->glob_npview.goal_marker.cur_anim_time = 0.0f;
        mp->glob_npview.entry_count = 0;
        if (!mp->goals.queued_goals.empty())
        {
            command_goal cg;
            mp->goals.cur_goal = mp->goals.queued_goals.back();
            mp->goals.queued_goals.pop_back();
            cg.goal_p.pos = dvec3_from(mp->goals.cur_goal);
            mp->goals.cur_goal_status = -2;
            net_tx(*conn, cg);
        }
    }

    for (int i = mp->goals.queued_goals.size() - 1; i >= 0; --i)
    {
        vec3 from = mp->goals.cur_goal;
        if (i != mp->goals.queued_goals.size() - 1)
            from = mp->goals.queued_goals[i + 1];
        dbg->AddLine(from, mp->goals.queued_goals[i], mp->glob_npview.color);
        dbg->AddCircle(mp->goals.queued_goals[i], {0, 0, -1}, marker_rad * 0.75, mp->glob_npview.goal_marker.queued_color);
    }
}

intern void mark_transforms_for_update_if_needed(map_panel *mp, float dt)
{
    static float counter = 0.0f;
    counter += dt;
    if (counter >= 0.016f)
    {
        mp->odom->MarkDirty();
        counter = 0.0f;
    }
}

intern void map_panel_run_frame(map_panel *mp, float dt, net_connection *conn)
{
    auto dbg = mp->view->GetScene()->GetComponent<urho::DebugRenderer>();
    mark_transforms_for_update_if_needed(mp, dt);
    update_and_draw_nav_goals(mp, dt, dbg, conn);
    draw_nav_path(mp->glob_npview, dbg);
    draw_nav_path(mp->loc_npview, dbg);
    draw_measure_path(mp->mpoints, dbg);
}


intern void setup_input_actions(map_panel *mp, const ui_info &ui_inf, net_connection *conn, input_data *inp)
{
    auto cam_node = mp->view->GetCameraNode();
    auto on_click = [cam_node, mp](const itrigger_event &tevent) {
        if (mp->js_enabled || (mp->view != tevent.vp.element_under_mouse && tevent.vp.element_under_mouse->GetPriority() > 2))
            return;

        if ((mp->toolbar.add_goal && !mp->toolbar.add_goal->IsChecked()) && !mp->toolbar.enable_measure->IsChecked())
            return;

        // Here is a hack for the phone to ignore the buggy press that happens on toggling a checkbox for the first time
        auto ctxt = cam_node->GetContext();
        auto time = ctxt->GetSubsystem<urho::Time>();
        if (time->GetFrameNumber() == mp->toolbar.last_frame_checked)
            return;

        auto camc = cam_node->GetComponent<urho::Camera>();

        urho::Plane p{{0, 0, -1}, {0, 0, 0}};
        auto scrn_ray = camc->GetScreenRay(tevent.norm_mpos.x_, tevent.norm_mpos.y_);

        double dist = scrn_ray.HitDistance(p);
        if (dist < 1000)
        {
            auto pos = scrn_ray.origin_ + scrn_ray.direction_ * dist;
            pos.z_ -= 0.05f; // No z fighting!
            dlog("CALLED with pos %f %f", pos.x_, pos.y_);

            // Place at the front
            if (mp->toolbar.add_goal && mp->toolbar.add_goal->IsChecked())
            {
                mp->goals.queued_goals.insert(mp->goals.queued_goals.begin(), pos);
                mp->toolbar.add_goal->SetChecked(false);
            }
            else if (mp->toolbar.enable_measure->IsChecked())
            {
                mp->mpoints.entries[mp->mpoints.entry_count] = pos;
                ++mp->mpoints.entry_count;
            }
        }
    };

    input_action_trigger it{};
    it.cond.mbutton = urho::MOUSEB_LEFT;
    it.name = urho::StringHash("Click").ToHash();
    it.tstate = T_BEGIN;
    it.mb_req = 0;
    it.mb_allowed = 0;
    it.qual_req = 0;
    it.qual_allowed = urho::QUAL_ANY;
    input_add_trigger(&inp->map, it);
    ss_connect(&mp->router, inp->dispatch.trigger, it.name, on_click);

    viewport_element vp{mp->view->GetViewport(), mp->view};
    inp->dispatch.vp_stack.emplace_back(vp);
}

intern void resize_path_length_text(map_panel *mp, const ui_info &ui_inf)
{    
    float y_anchor = mp->toolbar.widget->GetMinAnchor().y_;
    float tb_height = mp->toolbar.widget->GetHeight();
    float view_height = mp->view->GetHeight();
    float y_norm_offset = tb_height / view_height;
    mp->mpoints.path_len_text->SetMinAnchor(0.01, y_anchor + y_norm_offset);
}

intern void setup_conn_text(map_panel *mp, const ui_info &ui_inf)
{
    mp->conn_text = mp->view->CreateChild<urho::Text>();
    mp->conn_text->SetStyle("ConnText", ui_inf.style);
    mp->conn_text->SetFontSize(40 * ui_inf.dev_pixel_ratio_inv);
}

intern void setup_path_length_text(map_panel *mp, const ui_info &ui_inf)
{
    mp->mpoints.path_len_text = mp->view->CreateChild<urho::Text>();
    mp->mpoints.path_len_text->SetStyle("MeasurePathText", ui_inf.style);
    mp->mpoints.path_len_text->SetFontSize(40 * ui_inf.dev_pixel_ratio_inv);
    resize_path_length_text(mp, ui_inf);
}

intern void setup_event_handlers(map_panel *mp, const ui_info &ui_inf, net_connection *conn)
{
    mp->view->SubscribeToEvent(urho::E_UPDATE, [mp, conn](urho::StringHash type, urho::VariantMap &ev_data) {
        auto dt = ev_data[urho::Update::P_TIMESTEP].GetFloat();
        map_panel_run_frame(mp, dt, conn);
    });

    mp->view->SubscribeToEvent(urho::E_CLICKEND, [mp, conn](urho::StringHash type, urho::VariantMap &ev_data) {
        auto elem = (urho::UIElement *)ev_data[urho::Pressed::P_ELEMENT].GetPtr();
        cam_handle_mouse_released(mp, elem);
        param_handle_mouse_released(mp, elem, conn);
        toolbar_handle_mouse_released(mp, elem, conn);
        map_toggle_views_handle_mouse_released(mp, elem);
    });

    mp->view->SubscribeToEvent(urho::E_RESIZED, [mp, ui_inf](urho::StringHash type, urho::VariantMap &ev_data) {
        auto elem = (urho::UIElement *)ev_data[urho::Pressed::P_ELEMENT].GetPtr();
        if (elem == mp->view)
        {
            resize_path_length_text(mp, ui_inf);
            map_toggle_views_handle_resize(mp, ui_inf);
            mp->map.rend_texture->SetData(mp->map.image);
        }
    });

    mp->view->SubscribeToEvent(urho::E_TOGGLED, [mp, conn](urho::StringHash type, urho::VariantMap &ev_data) {
        auto elem = (urho::UIElement *)ev_data[urho::Toggled::P_ELEMENT].GetPtr();
        toolbar_handle_toggle(mp, elem);
        map_toggle_views_handle_toggle(mp, elem);
    });
}

intern void update_conn_count_text(map_panel *mp, i8 new_count)
{
    urho::String str;
    str.AppendWithFormat("Connections: %d", new_count);
    mp->conn_text->SetText(str);
}

void map_panel_init(map_panel *mp, const ui_info &ui_inf, net_connection *conn, input_data *inp)
{
    auto uctxt = ui_inf.ui_sys->GetContext();
    auto cache = uctxt->GetSubsystem<urho::ResourceCache>();
    ilog("Initializing map panel");

    create_3dview(mp, cache, ui_inf.ui_sys->GetRoot(), uctxt);
    setup_scene(mp, cache, mp->view->GetScene(), uctxt);

    toolbar_init(mp, ui_inf, conn->can_control);
    param_init(mp, conn, ui_inf);
    cam_init(mp, inp, ui_inf);
    map_toggle_views_init(mp, ui_inf);
    setup_conn_text(mp, ui_inf);
    setup_path_length_text(mp, ui_inf);

    ss_connect(&mp->router, conn->scan_received, [mp](const sicklms_laser_scan &pckt) { update_scene_from_scan(mp, pckt); });
    ss_connect(&mp->router, conn->map_update_received, [mp](const occ_grid_update &pckt) { update_scene_map_from_occ_grid(&mp->map, pckt); });
    ss_connect(&mp->router, conn->glob_cm_update_received, [mp](const occ_grid_update &pckt) { update_scene_map_from_occ_grid(&mp->glob_cmap, pckt); });
    ss_connect(&mp->router, conn->loc_cm_update_received, [mp](const occ_grid_update &pckt) { update_scene_map_from_occ_grid(&mp->loc_cmap, pckt); });
    ss_connect(&mp->router, conn->transform_updated, [mp](const node_transform &pckt) { update_node_transform(mp, pckt); });
    ss_connect(&mp->router, conn->glob_nav_path_updated, [mp](const nav_path &pckt) { update_glob_nav_path(mp, pckt); });
    ss_connect(&mp->router, conn->loc_nav_path_updated, [mp](const nav_path &pckt) { update_loc_nav_path(mp, pckt); });
    ss_connect(&mp->router, conn->goal_status_updated, [mp](const current_goal_status &pckt) { update_cur_goal_status(mp, pckt); });
    ss_connect(&mp->router, conn->connection_count_change, [mp](i8 nc) { update_conn_count_text(mp, nc); });

    setup_input_actions(mp, ui_inf, conn, inp);
    setup_event_handlers(mp, ui_inf, conn);

    mp->mpoints.entry_count = 0;
}

void map_clear_occ_grid(occ_grid_map *ocg)
{
    ocg->image->SetSize(512, 512, 4);
    for (int h = 0; h < ocg->image->GetHeight(); ++h)
    {
        for (int w = 0; w < ocg->image->GetWidth(); ++w)
            ocg->image->SetPixel(w, h, ocg->cols.undiscovered);
    }
    ocg->rend_texture->SetData(ocg->image);

    auto bb = ocg->bb_set->GetBillboard(0);
    bb->enabled_ = true;
    bb->size_ = {25.6f, 25.6f};
    ocg->bb_set->Commit();
}

void map_panel_term(map_panel *mp)
{
    ilog("Terminating map panel");
    cam_term(mp);
    param_term(mp);
    toolbar_term(mp);
    map_toggle_views_term(mp);
}
