
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
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>

#include <GL/gl.h>
#include <STB/stb_image.h>

#include "input.h"
#include "mapping.h"
#include "network.h"
#include "robot_control.h"
#include "joystick.h"

// Common to jackal and husky
const std::string MAP{"map"};
const std::string ODOM{"odom"};
const std::string BASE_LINK{"base_link"};
const std::string FRONT_LEFT_WHEEL_LINK{"front_left_wheel_link"};
const std::string FRONT_RIGHT_WHEEL_LINK{"front_right_wheel_link"};
const std::string REAR_LEFT_WHEEL_LINK{"rear_left_wheel_link"};
const std::string REAR_RIGHT_WHEEL_LINK{"rear_right_wheel_link"};
const std::string IMU_LINK{"imu_link"};

// Jackal only
const std::string CHASSIS_LINK{"chassis_link"};
const std::string FRONT_FENDER_LINK{"front_fender_link"};
const std::string NAVSAT_LINK{"navsat_link"};
const std::string REAR_FENDER_LINK{"rear_fender_link"};
const std::string MID_MOUNT{"mid_mount"};
const std::string FRONT_MOUNT{"front_mount"};
const std::string FRONT_CAMERA_MOUNT{"front_camera_mount"};
const std::string FRONT_CAMERA_BEAM{"front_camera_beam"};
const std::string FRONT_CAMERA{"front_camera"};
const std::string FRONT_CAMERA_OPTICAL{"front_camera_optical"};
const std::string FRONT_LASER_MOUNT{"front_laser_mount"};
const std::string FRONT_LASER{"front_laser"};
const std::string REAR_MOUNT{"rear_mount"};
const std::string REAR_BRIDGE_BASE{"rear_bridge_base"};
const std::string REAR_BRIDGE{"rear_bridge"};
const std::string REAR_NAVSAT{"rear_navsat"};
const std::string REAR_STANDOFF0{"rear_standoff0"};
const std::string REAR_STANDOFF1{"rear_standoff1"};
const std::string REAR_STANDOFF2{"rear_standoff2"};
const std::string REAR_STANDOFF3{"rear_standoff3"};

// Husky only
const std::string BASE_FOOTPRINT{"base_footprint"};
const std::string FRONT_BUMPER_LINK{"front_bumper_link"};
const std::string INERTIAL_LINK{"inertial_link"};
const std::string REAR_BUMPER_LINK{"rear_bumper_link"};
const std::string TOP_PLATE_LINK{"top_plate_link"};
const std::string SENSOR_ARCH_BASE_LINK{"sensor_arch_base_link"};
const std::string SENSOR_ARCH_MOUNT_LINK{"sensor_arch_mount_link"};
const std::string VLP16_MOUNT_BASE_LINK{"vlp16_mount_base_link"};
const std::string VLP16_MOUNT_PLATE{"vlp16_mount_plate"};
const std::string VELODYNE_BASE_LINK{"velodyne_base_link"};
const std::string VELODYNE{"velodyne"};
const std::string VLP16_MOUNT_LEFT_SUPPORT{"vlp16_mount_left_support"};
const std::string VLP16_MOUNT_RIGHT_SUPPORT{"vlp16_mount_right_support"};
const std::string TOP_PLATE_FRONT_LINK{"top_plate_front_link"};
const std::string TOP_PLATE_REAR_LINK{"top_plate_rear_link"};
const std::string TOP_CHASSIS_LINK{"top_chassis_link"};
const std::string USER_RAIL_LINK{"user_rail_link"};

intern const ogmap_colors map_colors{.undiscovered{0, 0.7, 0.7, 1}};

intern const ogmap_colors loc_cmc_colors{.lethal{1, 0, 0.7, 0.7},
                                         .inscribed{0, 0.7, 1, 0.7},
                                         .possibly_circumscribed{0.7, 0, 0, 0.7},
                                         .no_collision{0, 0.7, 0, 0.7}};

intern const ogmap_colors glob_cmc_colors{.lethal{1, 0, 1, 0.7},
                                          .inscribed{0, 1, 1, 0.7},
                                          .possibly_circumscribed{1, 0, 0, 0.7},
                                          .no_collision{0, 1, 0, 0.7}};

intern void create_3dview(map_panel *mp, urho::ResourceCache *cache, urho::UIElement *root, urho::Context *uctxt)
{
    auto rpath = cache->GetResource<urho::XMLFile>("RenderPaths/simple.xml");
    auto scene = new urho::Scene(uctxt);
    auto cam_node = new urho::Node(uctxt);

    mp->view = root->CreateChild<urho::View3D>();
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

intern void setup_scan_bb_from_node(map_panel *mp, urho::ResourceCache *cache)
{
    auto scan_mat = cache->GetResource<urho::Material>("Materials/scan_billboard.xml");
    mp->scan_bb = mp->lidar_node->CreateComponent<urho::BillboardSet>();
    mp->scan_bb->SetMaterial(scan_mat);
    mp->scan_bb->SetFixedScreenSize(true);
}

intern void create_husky(map_panel *mp, urho::ResourceCache *cache)
{
    auto yellow_mat = cache->GetResource<urho::Material>("Materials/Yellow.xml");
    auto dark = cache->GetResource<urho::Material>("Materials/Dark.xml");
    auto gray = cache->GetResource<urho::Material>("Materials/Grey.xml");

    auto husky_bumper = cache->GetResource<urho::Model>("Models/husky_bumper.mdl");
    auto husky_top_chassis = cache->GetResource<urho::Model>("Models/husky_top_chassis.mdl");
    auto husky_top_plate = cache->GetResource<urho::Model>("Models/husky_top_plate.mdl");
    auto husky_top_plate_cover = cache->GetResource<urho::Model>("Models/husky_top_plate_cover.mdl");
    auto husky_user_rail = cache->GetResource<urho::Model>("Models/husky_user_rail.mdl");
    auto husky_base_link = cache->GetResource<urho::Model>("Models/husky_base_link.mdl");
    auto husky_wheel_model = cache->GetResource<urho::Model>("Models/husky_wheel.mdl");
    auto novatel_smart6 = cache->GetResource<urho::Model>("Models/novatel-smart6.mdl");
    auto husky_sensor_arch = cache->GetResource<urho::Model>("Models/husky_sensor_arch.mdl");
    auto husky_lidar_riser = cache->GetResource<urho::Model>("Models/husky_lidar_riser.mdl");
    auto husky_lidar_cross = cache->GetResource<urho::Model>("Models/husky_lidar_cross.mdl");

    auto velo_top = cache->GetResource<urho::Model>("Models/husky_vlp_top.mdl");
    auto velo_mid = cache->GetResource<urho::Model>("Models/husky_vlp_middle.mdl");
    auto velo_bottom = cache->GetResource<urho::Model>("Models/husky_vlp_bottom.mdl");

    auto husky_base_model_node = mp->base_link->CreateChild("husky_base_link_model");
    auto smodel = husky_base_model_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_base_link);
    smodel->SetMaterial(dark);
    husky_base_model_node->Rotate({90, {-1, 0, 0}});

    // Children of chassis link
    mp->node_lut[BASE_FOOTPRINT] = mp->base_link->CreateChild(BASE_FOOTPRINT.c_str());

    auto front_bumper_link = mp->base_link->CreateChild(FRONT_BUMPER_LINK.c_str());
    mp->node_lut[FRONT_BUMPER_LINK] = front_bumper_link;
    auto front_bumper_offset = front_bumper_link->CreateChild("front_bumper_offset");
    smodel = front_bumper_offset->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_bumper);
    smodel->SetMaterial(dark);
    front_bumper_offset->Rotate({90, {-1, 0, 0}});

    mp->node_lut[IMU_LINK] = mp->base_link->CreateChild(IMU_LINK.c_str());
    mp->node_lut[INERTIAL_LINK] = mp->base_link->CreateChild(INERTIAL_LINK.c_str());

    auto rear_bumper_link = mp->base_link->CreateChild(REAR_BUMPER_LINK.c_str());
    mp->node_lut[REAR_BUMPER_LINK] = rear_bumper_link;
    auto rear_bumper_offset = rear_bumper_link->CreateChild("rear_bumper_offset");
    smodel = rear_bumper_offset->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_bumper);
    smodel->SetMaterial(dark);
    rear_bumper_offset->Rotate({90, {-1, 0, 0}});

    auto top_plate_link = mp->base_link->CreateChild(TOP_PLATE_LINK.c_str());
    mp->node_lut[TOP_PLATE_LINK] = top_plate_link;
    auto top_plate_offset = top_plate_link->CreateChild("top_plate_offset");
    smodel = top_plate_offset->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_top_plate);
    smodel->SetMaterial(dark);
    top_plate_offset->Rotate({90, {-1, 0, 0}});

    auto top_plate_cover_offset = top_plate_link->CreateChild("top_plate_cover_offset");
    smodel = top_plate_cover_offset->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_top_plate_cover);
    smodel->SetMaterial(dark);
    top_plate_cover_offset->Rotate({90, {-1, 0, 0}});
    top_plate_cover_offset->Translate({0, -0.005, 0});

    auto sensor_arch_base_link = top_plate_link->CreateChild(SENSOR_ARCH_BASE_LINK.c_str());
    mp->node_lut[SENSOR_ARCH_BASE_LINK] = sensor_arch_base_link;

    auto sensor_arch_mount_link = sensor_arch_base_link->CreateChild(SENSOR_ARCH_MOUNT_LINK.c_str());
    mp->node_lut[SENSOR_ARCH_MOUNT_LINK] = sensor_arch_mount_link;
    auto sensor_arch_offset = sensor_arch_mount_link->CreateChild("sensor_arch_offset");
    smodel = sensor_arch_offset->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_sensor_arch);
    smodel->SetMaterial(dark);
    sensor_arch_offset->Rotate({90, {-1, 0, 0}});

    auto vlp_mount_base_link = sensor_arch_mount_link->CreateChild(VLP16_MOUNT_BASE_LINK.c_str());
    mp->node_lut[VLP16_MOUNT_BASE_LINK] = vlp_mount_base_link;

    auto vlp_mount_plate = vlp_mount_base_link->CreateChild(VLP16_MOUNT_PLATE.c_str());
    mp->node_lut[VLP16_MOUNT_PLATE] = vlp_mount_plate;
    smodel = vlp_mount_plate->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_lidar_cross);
    smodel->SetMaterial(dark);
    
    auto velodyne_base_link = vlp_mount_plate->CreateChild(VELODYNE_BASE_LINK.c_str());
    mp->node_lut[VELODYNE_BASE_LINK] = velodyne_base_link;

    // Child of front_laser_mount - This also has our billboard set for the scan
    mp->lidar_node = velodyne_base_link->CreateChild(VELODYNE.c_str());
    setup_scan_bb_from_node(mp, cache);
    mp->node_lut[VELODYNE] = mp->lidar_node;
    mp->node_lut[FRONT_LASER] = mp->lidar_node;
    auto offset_lidar_node = mp->lidar_node->CreateChild("offset_lidar_node");
    smodel = offset_lidar_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(velo_top);
    smodel->SetMaterial(gray);
    smodel = offset_lidar_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(velo_mid);
    smodel->SetMaterial(dark);
    smodel = offset_lidar_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(velo_bottom);
    smodel->SetMaterial(gray);

    auto vlp_left = vlp_mount_base_link->CreateChild(VLP16_MOUNT_LEFT_SUPPORT.c_str());
    mp->node_lut[VLP16_MOUNT_LEFT_SUPPORT] = vlp_left;
    smodel = vlp_left->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_lidar_riser);
    smodel->SetMaterial(dark);

    auto vlp_right = vlp_mount_base_link->CreateChild(VLP16_MOUNT_RIGHT_SUPPORT.c_str());
    mp->node_lut[VLP16_MOUNT_RIGHT_SUPPORT] = vlp_right;
    smodel = vlp_right->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_lidar_riser);
    smodel->SetMaterial(dark);

    mp->node_lut[TOP_PLATE_FRONT_LINK] = top_plate_link->CreateChild(TOP_PLATE_FRONT_LINK.c_str());
    mp->node_lut[TOP_PLATE_REAR_LINK] = top_plate_link->CreateChild(TOP_PLATE_REAR_LINK.c_str());

    auto top_chassis_link = mp->base_link->CreateChild(TOP_CHASSIS_LINK.c_str());
    mp->node_lut[TOP_CHASSIS_LINK] = top_chassis_link;
    auto top_chassis_offset = top_chassis_link->CreateChild("top_chassis_offset");
    smodel = top_chassis_offset->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_top_chassis);
    smodel->SetMaterial(yellow_mat);
    top_chassis_offset->Rotate({90, {-1, 0, 0}});

    auto user_rail_link = mp->base_link->CreateChild(USER_RAIL_LINK.c_str());
    mp->node_lut[USER_RAIL_LINK] = user_rail_link;
    auto user_rail_offset = user_rail_link->CreateChild("user_rail_offset");
    smodel = user_rail_offset->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_user_rail);
    smodel->SetMaterial(dark);
    user_rail_offset->Rotate({90, {-1, 0, 0}});

    auto fl_wheel_node_parent = mp->base_link->CreateChild(FRONT_LEFT_WHEEL_LINK.c_str());
    mp->node_lut[FRONT_LEFT_WHEEL_LINK] = fl_wheel_node_parent;
    auto fl_wheel_node = fl_wheel_node_parent->CreateChild("fl_wheel_model");
    smodel = fl_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_wheel_model);
    smodel->SetMaterial(dark);
    fl_wheel_node->Rotate({90, {-1, 0, 0}});

    auto fr_wheel_node_parent = mp->base_link->CreateChild(FRONT_RIGHT_WHEEL_LINK.c_str());
    mp->node_lut[FRONT_RIGHT_WHEEL_LINK] = fr_wheel_node_parent;
    auto fr_wheel_node = fr_wheel_node_parent->CreateChild("fr_wheel_model");
    smodel = fr_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_wheel_model);
    smodel->SetMaterial(dark);
    fr_wheel_node->Rotate({90, {-1, 0, 0}});

    auto rl_wheel_node_parent = mp->base_link->CreateChild(REAR_LEFT_WHEEL_LINK.c_str());
    mp->node_lut[REAR_LEFT_WHEEL_LINK] = rl_wheel_node_parent;
    auto rl_wheel_node = rl_wheel_node_parent->CreateChild("rl_wheel_model");
    smodel = rl_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_wheel_model);
    smodel->SetMaterial(dark);
    rl_wheel_node->Rotate({90, {-1, 0, 0}});

    auto rr_wheel_node_parent = mp->base_link->CreateChild(REAR_RIGHT_WHEEL_LINK.c_str());
    mp->node_lut[REAR_RIGHT_WHEEL_LINK] = rr_wheel_node_parent;
    auto rr_wheel_node = rr_wheel_node_parent->CreateChild("rr_wheel_model");
    smodel = rr_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(husky_wheel_model);
    smodel->SetMaterial(dark);
    rr_wheel_node->Rotate({90, {-1, 0, 0}});
}

intern void create_jackal(map_panel *mp, urho::ResourceCache *cache)
{
    auto jackal_base_model = cache->GetResource<urho::Model>("Models/jackal-base.mdl");
    auto jackal_fender_model = cache->GetResource<urho::Model>("Models/jackal-fender.mdl");
    auto jackal_wheel_model = cache->GetResource<urho::Model>("Models/jackal-wheel.mdl");
    auto jackal_bridge_plate = cache->GetResource<urho::Model>("Models/jackal-bridge-plate.mdl");
    auto jackal_sicklms = cache->GetResource<urho::Model>("Models/jackal-sicklms-2.mdl");
    auto jackal_sicklms_bracket = cache->GetResource<urho::Model>("Models/jackal-sicklms-bracket.mdl");
    auto jackal_bumblebee2 = cache->GetResource<urho::Model>("Models/jackal-bumblebee2.mdl");
    auto novatel_smart6 = cache->GetResource<urho::Model>("Models/novatel-smart6.mdl");
    auto standoff = cache->GetResource<urho::Model>("Models/jackal-standoff.mdl");
    
    auto jackal_fender_mat = cache->GetResource<urho::Material>("Materials/Yellow.xml");
    auto jackal_base_mat = cache->GetResource<urho::Material>("Materials/Dark.xml");
    auto jackal_bb2_mat0 = cache->GetResource<urho::Material>("Materials/jackal_bb2_01.xml");
    auto jackal_bb2_mat1 = cache->GetResource<urho::Material>("Materials/jackal_bb2_02.xml");
    auto gray_mat = cache->GetResource<urho::Material>("Materials/Grey.xml");

    // Child of base link
    auto chassis_link = mp->base_link->CreateChild(CHASSIS_LINK.c_str());
    mp->node_lut[CHASSIS_LINK] = chassis_link;

    auto jackal_base_model_node = chassis_link->CreateChild("jackal_base_model");
    auto smodel = jackal_base_model_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_base_model);
    smodel->SetMaterial(jackal_base_mat);
    jackal_base_model_node->Rotate({90, {0, 0, -1}});
    jackal_base_model_node->Rotate({90, {-1, 0, 0}});
    jackal_base_model_node->Translate({0,0,0.065}, urho::TransformSpace::World);

    auto fl_wheel_node_parent = chassis_link->CreateChild(FRONT_LEFT_WHEEL_LINK.c_str());
    mp->node_lut[FRONT_LEFT_WHEEL_LINK] = fl_wheel_node_parent;
    auto fl_wheel_node = fl_wheel_node_parent->CreateChild("fl_wheel_model");
    smodel = fl_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_wheel_model);
    smodel->SetMaterial(jackal_base_mat);
    fl_wheel_node->Rotate({90, {-1, 0, 0}});

    auto fr_wheel_node_parent = chassis_link->CreateChild(FRONT_RIGHT_WHEEL_LINK.c_str());
    mp->node_lut[FRONT_RIGHT_WHEEL_LINK] = fr_wheel_node_parent;
    auto fr_wheel_node = fr_wheel_node_parent->CreateChild("fr_wheel_model");
    smodel = fr_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_wheel_model);
    smodel->SetMaterial(jackal_base_mat);
    fr_wheel_node->Rotate({90, {-1, 0, 0}});

    auto rl_wheel_node_parent = chassis_link->CreateChild(REAR_LEFT_WHEEL_LINK.c_str());
    mp->node_lut[REAR_LEFT_WHEEL_LINK] = rl_wheel_node_parent;
    auto rl_wheel_node = rl_wheel_node_parent->CreateChild("rl_wheel_model");
    smodel = rl_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_wheel_model);
    smodel->SetMaterial(jackal_base_mat);
    rl_wheel_node->Rotate({90, {-1, 0, 0}});

    auto rr_wheel_node_parent = chassis_link->CreateChild(REAR_RIGHT_WHEEL_LINK.c_str());
    mp->node_lut[REAR_RIGHT_WHEEL_LINK] = rr_wheel_node_parent;
    auto rr_wheel_node = rr_wheel_node_parent->CreateChild("rr_wheel_model");
    smodel = rr_wheel_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_wheel_model);
    smodel->SetMaterial(jackal_base_mat);
    rr_wheel_node->Rotate({90, {-1, 0, 0}});

    // Children of chassis link
    auto front_fender_link = chassis_link->CreateChild(FRONT_FENDER_LINK.c_str());
    mp->node_lut[FRONT_FENDER_LINK] = front_fender_link;
    auto jackal_fender_node = front_fender_link->CreateChild("jackal_front_fender");
    smodel = jackal_fender_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_fender_model);
    smodel->SetMaterial(jackal_fender_mat);

    auto rear_fender_link = chassis_link->CreateChild(REAR_FENDER_LINK.c_str());
    mp->node_lut[REAR_FENDER_LINK] = rear_fender_link;
    auto jackal_rear_fender_node = rear_fender_link->CreateChild("jackal_rear_fender");
    smodel = jackal_rear_fender_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_fender_model);
    smodel->SetMaterial(jackal_fender_mat);

    mp->node_lut[IMU_LINK] = chassis_link->CreateChild(IMU_LINK.c_str());
    mp->node_lut[NAVSAT_LINK] = chassis_link->CreateChild(NAVSAT_LINK.c_str());

    auto mid_mount = chassis_link->CreateChild(MID_MOUNT.c_str());
    mp->node_lut[MID_MOUNT] = mid_mount;
    // Create node for jackel model stuff

    // Children of mid mount
    auto front_mount = mid_mount->CreateChild(FRONT_MOUNT.c_str());
    mp->node_lut[FRONT_MOUNT] = front_mount;

    auto front_camera_mount = front_mount->CreateChild(FRONT_CAMERA_MOUNT.c_str());
    mp->node_lut[FRONT_CAMERA_MOUNT] = front_camera_mount;

    auto front_camera_beam = front_camera_mount->CreateChild(FRONT_CAMERA_BEAM.c_str());
    mp->node_lut[FRONT_CAMERA_BEAM] = front_camera_beam;

    auto front_camera = front_camera_beam->CreateChild(FRONT_CAMERA.c_str());
    mp->node_lut[FRONT_CAMERA] = front_camera;

    auto front_camera_optical = front_camera->CreateChild(FRONT_CAMERA_OPTICAL.c_str());
    mp->node_lut[FRONT_CAMERA_OPTICAL] = front_camera_optical;
    auto cam_offset = front_camera_optical->CreateChild("cam_offset");
    smodel = cam_offset->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_bumblebee2);
    smodel->SetMaterial(0, jackal_bb2_mat0);
    smodel->SetMaterial(1, jackal_bb2_mat1);
    smodel->SetMaterial(2, jackal_bb2_mat1);
    cam_offset->Rotate({90, {0, 1, 0}});

    // Child of front mount
    auto front_laser_mount = front_mount->CreateChild(FRONT_LASER_MOUNT.c_str());
    mp->node_lut[FRONT_LASER_MOUNT] = front_laser_mount;
    smodel = front_laser_mount->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_sicklms_bracket);
    smodel->SetMaterial(jackal_base_mat);

    // Child of front_laser_mount - This also has our billboard set for the scan
    mp->lidar_node = front_laser_mount->CreateChild(FRONT_LASER.c_str());
    setup_scan_bb_from_node(mp, cache);
    mp->node_lut[FRONT_LASER] = mp->lidar_node;

    auto lidar_offset = mp->lidar_node->CreateChild("lidar_offset");
    smodel = lidar_offset->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_sicklms);
    smodel->SetMaterial(gray_mat);
    lidar_offset->Rotate({90, {-1, 0, 0}});

    auto rear_mnt = mid_mount->CreateChild(REAR_MOUNT.c_str());
    mp->node_lut[REAR_MOUNT] = rear_mnt;

    auto rear_bridge_base = rear_mnt->CreateChild(REAR_BRIDGE_BASE.c_str());
    mp->node_lut[REAR_BRIDGE_BASE] = rear_bridge_base;

    auto rear_bridge = rear_bridge_base->CreateChild(REAR_BRIDGE.c_str());
    mp->node_lut[REAR_BRIDGE] = rear_bridge;
    auto rear_bridge_offset = rear_bridge->CreateChild("rear_bridge_offset");
    smodel = rear_bridge_offset->CreateComponent<urho::StaticModel>();
    smodel->SetModel(jackal_bridge_plate);
    smodel->SetMaterial(gray_mat);
    rear_bridge_offset->Rotate({90, {0, 0, -1}});
    rear_bridge_offset->Rotate({90, {1, 0, 0}});

    auto rear_navsat = rear_bridge->CreateChild(REAR_NAVSAT.c_str());
    mp->node_lut[REAR_NAVSAT] = rear_navsat;
    smodel = rear_navsat->CreateComponent<urho::StaticModel>();
    smodel->SetModel(novatel_smart6);
    smodel->SetMaterial(gray_mat);

    auto standoff_node = rear_bridge_base->CreateChild(REAR_STANDOFF0.c_str());
    mp->node_lut[REAR_STANDOFF0] = standoff_node;
    smodel = standoff_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(standoff);
    smodel->SetMaterial(gray_mat);
    
    standoff_node = rear_bridge_base->CreateChild(REAR_STANDOFF1.c_str());
    mp->node_lut[REAR_STANDOFF1] = standoff_node;
    smodel = standoff_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(standoff);
    smodel->SetMaterial(gray_mat);
    
    standoff_node = rear_bridge_base->CreateChild(REAR_STANDOFF2.c_str());
    mp->node_lut[REAR_STANDOFF2] = standoff_node;
    smodel = standoff_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(standoff);
    smodel->SetMaterial(gray_mat);
    
    standoff_node = rear_bridge_base->CreateChild(REAR_STANDOFF3.c_str());
    mp->node_lut[REAR_STANDOFF3] = standoff_node;
    smodel = standoff_node->CreateComponent<urho::StaticModel>();
    smodel->SetModel(standoff);
    smodel->SetMaterial(gray_mat);
}

intern void setup_occ_grid_map(occ_grid_map *map,
                               const char *node_name,
                               urho::ResourceCache *cache,
                               urho::Scene *scene,
                               urho::Context *uctxt)
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

intern void setup_scene(map_panel *mp, urho::ResourceCache *cache, urho::Scene *scene, urho::Context *uctxt, bool is_husky)
{
    // Grab all resources needed
    float additional = 0.0;
    if (is_husky)
        additional = 0.06;
    mp->map.cols = map_colors;
    setup_occ_grid_map(&mp->map, MAP.c_str(), cache, scene, uctxt);
    mp->map.offset_z = 0.1 + additional;
    mp->node_lut[MAP] = mp->map.node;

    mp->glob_cmap.cols = glob_cmc_colors;
    mp->glob_cmap.map_type = OCC_GRID_TYPE_GCOSTMAP;
    mp->glob_cmap.offset_z = 0.09 + additional;
    setup_occ_grid_map(&mp->glob_cmap, "global_costmap", cache, scene, uctxt);
    mp->glob_cmap.node->Translate({0, 0, -0.01});

    mp->loc_cmap.cols = loc_cmc_colors;
    mp->loc_cmap.map_type = OCC_GRID_TYPE_LCOSTMAP;
    mp->loc_cmap.offset_z = 0.08 + additional;
    setup_occ_grid_map(&mp->loc_cmap, "local_costmap", cache, scene, uctxt);
    mp->loc_cmap.node->Translate({0, 0, -0.02});

    mp->loc_npview.color = {1.0f, 0.6f, 0.0f, 1.0f};

    // Odom frame is smoothly moving while map may experience discreet jumps
    mp->odom = mp->map.node->CreateChild(ODOM.c_str());
    mp->node_lut[ODOM] = mp->odom;

    // Base link is main node tied to the jackal base
    mp->base_link = mp->odom->CreateChild(BASE_LINK.c_str());
    mp->node_lut[BASE_LINK] = mp->base_link;

    // Follow camera for the robot
    auto robot_follow_cam = mp->base_link->CreateChild("robot_follow_cam");
    mp->view->GetCameraNode()->SetParent(robot_follow_cam);
    robot_follow_cam->SetRotation({90, {0, 0, -1}});

    if (is_husky) {
        create_husky(mp, cache);
    }
    else {
        create_jackal(mp, cache);
    }
}

intern void reposition_cam_view(map_panel *mp, const ui_info &ui_inf)
{
    auto cam_view_size = mp->cam_view.window->GetSize();
    auto fcam_view_size = vec2{cam_view_size.x_, cam_view_size.y_};

    // Put the cam view, by default, centered horizontally and a bit below the center vertically
    auto view_size = mp->view->GetSize();

    vec2 fvsize{view_size.x_, view_size.y_};

    auto js_size = mp->ctxt->js_panel.outer_ring->GetMaxOffset();
    auto js_anch = mp->ctxt->js_panel.outer_ring->GetMaxAnchor();
    float y_pos = fvsize.y_ * js_anch.y_ - js_size.y_ - fcam_view_size.y_ - 20.0f * ui_inf.dev_pixel_ratio_inv;

    vec2 fpos{(fvsize.x_ - fcam_view_size.x_) * 0.5f, y_pos};
    ivec2 pos = {(int)fpos.x_, (int)fpos.y_};
    mp->cam_view.window->SetPosition(pos);
}

intern void create_image_view(map_panel *mp, const ui_info &ui_inf)
{
    vec2 fcamview_size{vec2{640, 480} * ui_inf.dev_pixel_ratio_inv};

    ivec2 default_camview_size = {(int)fcamview_size.x_, (int)fcamview_size.y_};
    int border = 40 * ui_inf.dev_pixel_ratio_inv;
    irect resize_borders = {border, border, border, border};

    mp->cam_view.window = mp->view->CreateChild<urho::Window>("jackal_cam");
    mp->cam_view.texture_view = mp->cam_view.window->CreateChild<urho::BorderImage>("jackal_cam");
    mp->cam_view.rend_text = new urho::Texture2D(ui_inf.ui_sys->GetContext());
    mp->cam_view.rend_text->SetNumLevels(1);
    mp->cam_view.rend_text->SetFilterMode(urho::FILTER_BILINEAR);
    mp->cam_view.rend_text->SetAddressMode(urho::COORD_U, urho::ADDRESS_CLAMP);
    mp->cam_view.rend_text->SetAddressMode(urho::COORD_V, urho::ADDRESS_CLAMP);

    mp->cam_view.window->SetSize(default_camview_size.x_ + resize_borders.Width(),
                                 default_camview_size.y_ + resize_borders.Height());
    mp->cam_view.window->SetColor({0, 0, 0, 0.8f});
    mp->cam_view.window->SetResizeBorder(resize_borders);
    mp->cam_view.window->SetPriority(3);
    mp->cam_view.window->SetResizable(true);
    mp->cam_view.window->SetMovable(true);

    mp->cam_view.texture_view->SetTexture(mp->cam_view.rend_text);
    mp->cam_view.texture_view->SetFullImageRect();
    mp->cam_view.texture_view->SetEnableAnchor(true);
    mp->cam_view.texture_view->SetMinAnchor({0, 0});
    mp->cam_view.texture_view->SetMaxAnchor({1, 1});
    mp->cam_view.texture_view->SetMinOffset({resize_borders.left_, resize_borders.top_});
    mp->cam_view.texture_view->SetMaxOffset({-resize_borders.right_, -resize_borders.bottom_});

    mp->cam_view.window->SetVisible(false);
    reposition_cam_view(mp, ui_inf);
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

    if (resized != ivec2{map->image->GetWidth(), map->image->GetHeight()} || grid.meta.reset_map == 1) {
        ilog("Map resized texture to %d by %d (actual map size %d %d)",
             resized.x_,
             resized.y_,
             grid.meta.width,
             grid.meta.height);
        map->image->SetSize(resized.x_, resized.y_, 4);
        for (int y = 0; y < resized.y_; ++y) {
            for (int x = 0; x < resized.x_; ++x) {
                map->image->SetPixel(x, y, map->cols.undiscovered);
            }
        }
    }

    quat q = quat_from(grid.meta.origin_p.orientation);
    vec3 pos = vec3_from(grid.meta.origin_p.pos);
    map->node->SetRotation(q);

    auto billboard = map->bb_set->GetBillboard(0);
    billboard->size_ = vec2{(float)map->image->GetWidth(), (float)map->image->GetHeight()} * grid.meta.resolution * 0.5;
    float h_diff = billboard->size_.y_ - grid.meta.height * grid.meta.resolution * 0.5;
    billboard->position_ = pos + vec3{billboard->size_, map->offset_z};
    billboard->enabled_ = true;
    
    for (int i = 0; i < grid.meta.change_elem_count; ++i) {
        u32 map_ind = (grid.change_elems[i] >> 8);
        u8 prob = (u8)grid.change_elems[i];
        ivec2 tex_coods = index_to_texture_coords(map_ind, grid.meta.width, map->image->GetHeight());

        if (map->map_type == OCC_GRID_TYPE_MAP) {
            if (prob <= 100) {
                float fprob = 1.0 - (float)prob * 0.01;
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, {fprob, fprob, fprob, 1.0});
            }
            else {
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, map->cols.undiscovered);
            }
        }
        else {
            if (prob == 100) {
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, map->cols.lethal);
            }
            else if (prob == 99) {
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, map->cols.inscribed);
            }
            else if (prob <= 98 && prob >= 50) {
                auto col = map->cols.possibly_circumscribed;
                col.a_ -= -(1.0 - (prob - 50.0) / (98.0 - 50.0)) * 0.4;
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, col);
            }
            else if (prob <= 50 && prob >= 1) {
                auto col = map->cols.no_collision;
                col.a_ -= (1.0 - (prob - 1.0) / (50.0 - 1.0)) * 0.4;
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, col);
            }
            else {
                map->image->SetPixel(tex_coods.x_, tex_coods.y_, map->cols.free_space);
            }
        }
    }

    map->bb_set->Commit();
    map->rend_texture->SetData(map->image);
}

intern void update_scene_from_scan(map_panel *mp, const lidar_scan &packet)
{
    // Resize the billboard count to match the received scan
    sizet range_count = lidar_get_range_count(packet.meta);
    mp->scan_bb->SetNumBillboards(range_count);

    // Loop over each ange and convert the polar angle/distance to cartesian coordinates
    float cur_ang = packet.meta.angle_min;
    for (int i = 0; i < range_count; ++i) {
        auto bb = mp->scan_bb->GetBillboard(i);

        // Verify the range is within tolerance of the LIDAR - otherwise
        if (packet.ranges[i] < packet.meta.range_max && packet.ranges[i] > packet.meta.range_min) {
            bb->position_ = {packet.ranges[i] * cos(cur_ang), packet.ranges[i] * sin(cur_ang), 0.0f};
            bb->enabled_ = true;
            bb->size_ = {6, 6};
        }
        else {
            bb->enabled_ = false;
        }
        cur_ang += packet.meta.angle_increment;
    }
    // Send billboard to the GPU
    mp->scan_bb->Commit();
}

intern void update_node_transform(map_panel *mp, const node_transform &tform)
{
    urho::Node *node{}, *parent{};
    auto node_fiter = mp->node_lut.find(tform.name);
    if (node_fiter == mp->node_lut.end()) {
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

    if (node_parent != parent && node_parent_parent != parent) {
        wlog("Received update for node %s with different parent than received in packet (%s)",
             tform.name,
             tform.parent_name);
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
    if (mp->goals.cur_goal_status != -2 || gs.status == 0 || gs.status == 1) {
        mp->goals.cur_goal = vec3_from(gs.goal_p.pos);
        mp->goals.cur_goal_status = gs.status;
    }
}

intern float animate_marker_circles(goal_marker_info *gm, float dt)
{
    static bool increasing = true;
    if (increasing && gm->cur_anim_time < gm->loop_anim_time) {
        gm->cur_anim_time += dt;
        if (gm->cur_anim_time > gm->loop_anim_time)
            increasing = false;
    }
    else {
        gm->cur_anim_time -= dt;
        if (gm->cur_anim_time < 0)
            increasing = true;
    }

    float rad = (gm->max_rad - gm->min_rad) * (gm->cur_anim_time / gm->loop_anim_time) + gm->min_rad;
    return rad;
}

intern void draw_nav_path(const nav_path_view &np, urho::DebugRenderer *dbg)
{
    if (np.enabled) {
        for (int i = 1; i < np.entry_count; ++i)
            dbg->AddLine(np.path_entries[i - 1], np.path_entries[i], np.color);
    }
}

intern void draw_measure_path(const measure_points &mp, urho::DebugRenderer *dbg)
{
    float path_len = 0.0f;
    for (int i = 0; i < mp.entry_count; ++i) {
        if (i > 0) {
            dbg->AddLine(mp.entries[i - 1], mp.entries[i], mp.color);
            path_len += (mp.entries[i - 1] - mp.entries[i]).Length();
        }
        dbg->AddCircle(mp.entries[i], {0, 0, -1}, mp.marker_rad, mp.color);
    }
    mp.path_len_text->SetVisible(path_len > FLOAT_EPS);
    if (path_len > FLOAT_EPS) {
        path_len *= METERS_TO_FEET;
        int feet = int(path_len);
        path_len -= feet;
        int inches = int(path_len * 12.0);

        urho::String text;
        text.AppendWithFormat("%d'%d\"", feet, inches);
        mp.path_len_text->SetText(text);
    }
}

intern void update_and_draw_nav_goals(map_panel *mp, float dt, urho::DebugRenderer *dbg, net_connection *conn)
{
    float marker_rad = animate_marker_circles(&mp->glob_npview.goal_marker, dt);

    // If the current goal is in an active state (ie 0, 1, or -2 sort of) draw a circle for it. If the robot position
    // is within 0.25 meters, issue a stop command to the server. Once the stop command completes, the server will
    // send us a message where we change our mp->goals.cur_goal_status to a terminated state.
    if (mp->goals.cur_goal_status == 0 || mp->goals.cur_goal_status == 1 || mp->goals.cur_goal_status == -2) {
        dbg->AddCircle(mp->goals.cur_goal, {0, 0, -1}, marker_rad, mp->glob_npview.goal_marker.color);
        auto dist_to_goal = (mp->base_link->GetWorldPosition() - mp->goals.cur_goal).Length();
        if (dist_to_goal < 0.25) {
            command_stop stop{};
            net_tx(*conn, stop);
        }
    }
    else {
        // If our current goal is no longer active and there are goals in our queue, we move the goal
        // from the queue to our active goal and set the goal status to -2
        // -2 indicates that we sent the server our active goal but have yet to receive a response
        mp->glob_npview.goal_marker.cur_anim_time = 0.0f;
        mp->glob_npview.entry_count = 0;
        if (!mp->goals.queued_goals.empty()) {
            command_goal cg;
            mp->goals.cur_goal = mp->goals.queued_goals.back();
            mp->goals.queued_goals.pop_back();
            cg.goal_p.pos = dvec3_from(mp->goals.cur_goal);
            mp->goals.cur_goal_status = -2;
            net_tx(*conn, cg);
        }
    }

    // Draw the lines connecting our active goal and any queued goals. These lines are just to indicate to the
    // user the order of the queued goals.
    for (int i = mp->goals.queued_goals.size() - 1; i >= 0; --i) {
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
    if (counter >= 0.016f) {
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
        if (mp->js_enabled ||
            (mp->view != tevent.vp.element_under_mouse && tevent.vp.element_under_mouse->GetPriority() > 3))
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
        if (dist < 1000) {
            auto pos = scrn_ray.origin_ + scrn_ray.direction_ * dist;
            pos.z_ -= 0.05f; // No z fighting!

            // Place at the front
            if (mp->toolbar.add_goal && mp->toolbar.add_goal->IsChecked()) {
                mp->goals.queued_goals.insert(mp->goals.queued_goals.begin(), pos);
                mp->toolbar.add_goal->SetChecked(false);
            }
            else if (mp->toolbar.enable_measure->IsChecked()) {
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
        if (elem == mp->view) {
            reposition_cam_view(mp, ui_inf);
            resize_path_length_text(mp, ui_inf);
            map_toggle_views_handle_resize(mp, ui_inf);
            mp->map.rend_texture->SetData(mp->map.image);
        }
    });

    mp->view->SubscribeToEvent(urho::E_TOGGLED, [mp, conn](urho::StringHash type, urho::VariantMap &ev_data) {
        auto elem = (urho::UIElement *)ev_data[urho::Toggled::P_ELEMENT].GetPtr();
        toolbar_handle_toggle(mp, elem);
        map_toggle_views_handle_toggle(mp, elem, conn);
    });
}

intern void update_meta_stats(map_panel *mp, const misc_stats &updated_stats)
{
    if (updated_stats.conn_count != mp->cur_stats.conn_count ||
        !fequals(updated_stats.cur_bw_mbps, mp->cur_stats.cur_bw_mbps, 0.01f) ||
        !fequals(updated_stats.avg_bw_mbps, mp->cur_stats.avg_bw_mbps, 0.01f)) {
        mp->cur_stats = updated_stats;

        int cur_bw(mp->cur_stats.cur_bw_mbps), avg_bw(mp->cur_stats.avg_bw_mbps);
        float fcur_bw_100 = (mp->cur_stats.cur_bw_mbps - cur_bw) * 10.0f;
        float favg_bw_100 = (mp->cur_stats.avg_bw_mbps - avg_bw) * 10.0f;
        int cur_bw_100(fcur_bw_100), avg_bw_100(favg_bw_100);
        int cur_bw_10((fcur_bw_100 - cur_bw_100) * 10.0f), avg_bw_10((favg_bw_100 - avg_bw_100) * 10.0f);

        urho::String str;
        str.AppendWithFormat("Clients: %d    BW:%d.%d%d (%d.%d%d avg) Mbps",
                             mp->cur_stats.conn_count,
                             cur_bw,
                             cur_bw_100,
                             cur_bw_10,
                             avg_bw,
                             avg_bw_100,
                             avg_bw_10);
        mp->conn_text->SetText(str);
    }
}

intern void update_image(map_panel *mp, const compressed_image &img)
{
    ivec2 sz{};
    int channels{0};
    u8 *converted_data = stbi_load_from_memory(img.data, img.meta.data_size, &sz.x_, &sz.y_, &channels, 3);

    auto cur_sz = ivec2{mp->cam_view.rend_text->GetWidth(), mp->cam_view.rend_text->GetHeight()};
    if (cur_sz != sz) {
        mp->cam_view.rend_text->SetSize(sz.x_, sz.y_, GL_RGB, urho::TEXTURE_DYNAMIC, 0, false);
        mp->cam_view.texture_view->SetFullImageRect();
        ilog("Resizing image to %dx%d", sz.x_, sz.y_);
    }
    mp->cam_view.rend_text->SetData(0, 0, 0, sz.x_, sz.y_, converted_data);
    stbi_image_free(converted_data);
}

void map_panel_init(map_panel *mp, const ui_info &ui_inf, net_connection *conn, input_data *inp)
{
    auto uctxt = ui_inf.ui_sys->GetContext();
    auto cache = uctxt->GetSubsystem<urho::ResourceCache>();
    ilog("Initializing map panel");

    create_3dview(mp, cache, ui_inf.ui_sys->GetRoot(), uctxt);
    setup_scene(mp, cache, mp->view->GetScene(), uctxt, conn->is_husky);

    // These must come before map toggle views as the pointers need to be valid
    toolbar_init(mp, ui_inf, conn->can_control);
    param_init(mp, conn, ui_inf);
    cam_init(mp, inp, ui_inf);
    create_image_view(mp, ui_inf);

    map_toggle_views_init(mp, ui_inf);
    setup_conn_text(mp, ui_inf);
    setup_path_length_text(mp, ui_inf);

    ss_connect(&mp->router, conn->scan_received, [mp](const lidar_scan &pckt) { update_scene_from_scan(mp, pckt); });

    ss_connect(&mp->router, conn->map_update_received, [mp](const occ_grid_update &pckt) {
        update_scene_map_from_occ_grid(&mp->map, pckt);
    });

    ss_connect(&mp->router, conn->glob_cm_update_received, [mp](const occ_grid_update &pckt) {
        update_scene_map_from_occ_grid(&mp->glob_cmap, pckt);
    });
    ss_connect(&mp->router, conn->loc_cm_update_received, [mp](const occ_grid_update &pckt) {
        update_scene_map_from_occ_grid(&mp->loc_cmap, pckt);
    });
    ss_connect(
        &mp->router, conn->transform_updated, [mp](const node_transform &pckt) { update_node_transform(mp, pckt); });
    ss_connect(&mp->router, conn->glob_nav_path_updated, [mp](const nav_path &pckt) { update_glob_nav_path(mp, pckt); });
    ss_connect(&mp->router, conn->loc_nav_path_updated, [mp](const nav_path &pckt) { update_loc_nav_path(mp, pckt); });
    ss_connect(&mp->router, conn->goal_status_updated, [mp](const current_goal_status &pckt) {
        update_cur_goal_status(mp, pckt);
    });
    ss_connect(&mp->router, conn->image_update, [mp](const compressed_image &img) { update_image(mp, img); });
    ss_connect(&mp->router, conn->image_update, [mp](const compressed_image &img) { update_image(mp, img); });
    ss_connect(&mp->router, conn->meta_stats_update, [mp](const misc_stats &ms) { update_meta_stats(mp, ms); });

    setup_input_actions(mp, ui_inf, conn, inp);
    setup_event_handlers(mp, ui_inf, conn);

    mp->mpoints.entry_count = 0;
}

void map_clear_occ_grid(occ_grid_map *ocg)
{
    ocg->image->SetSize(512, 512, 4);
    for (int h = 0; h < ocg->image->GetHeight(); ++h) {
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
