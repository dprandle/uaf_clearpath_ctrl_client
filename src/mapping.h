#pragma once

#include "camera.h"
#include "params.h"
#include "toolbar.h"
#include "map_toggle_views.h"
#include "ss_router.h"
#include "network.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Urho3D
{
class UI;
class XMLFile;
class View3D;
class BillboardSet;
class Texture2D;
class Image;
class ListView;
class Node;
class Text;
class Window;
} // namespace Urho3D

struct input_data;
struct robot_control_ctxt;

struct ui_info
{
    urho::UI *ui_sys{};
    urho::XMLFile *style{};
    float dev_pixel_ratio_inv{1.0};
};

struct updated_tform
{
    vec3 pos;
    quat orientation;
    bool dirty{false};
    float wait_timer{0.0f};
};

enum occ_grid_type
{
    OCC_GRID_TYPE_MAP,
    OCC_GRID_TYPE_GCOSTMAP,
    OCC_GRID_TYPE_LCOSTMAP
};

struct ogmap_colors
{
    urho::Color undiscovered{0, 0, 0, 0};
    urho::Color lethal{0, 0, 0, 0};
    urho::Color inscribed{0, 0, 0, 0};
    urho::Color possibly_circumscribed{0, 0, 0, 0};
    urho::Color no_collision{0, 0, 0, 0};
    urho::Color free_space{0, 0, 0, 0};
};

struct occ_grid_map
{
    urho::Node *node{};
    urho::Texture2D *rend_texture{};
    urho::Image *image{};
    urho::BillboardSet *bb_set{};
    ogmap_colors cols;
    float offset_z {0.0f};

    int map_type{OCC_GRID_TYPE_MAP};
};

struct cam_image_view
{
    urho::Window *window{};
    urho::BorderImage *texture_view{};
    urho::Texture2D *rend_text{};
};

struct nav_goals
{
    vec3 cur_goal{};
    i32 cur_goal_status{-1};
    std::vector<vec3> queued_goals{};
};

struct goal_marker_info
{
    float min_rad{0.25f};
    float max_rad{0.35f};
    float loop_anim_time{0.5f};
    float cur_anim_time{0.0f};
    urho::Color color{0, 0.6, 0.6, 1};
    urho::Color queued_color{0, 0.6, 0.6, 1};
};

struct nav_path_view
{
    static constexpr sizet MAX_LINE_ENTRIES = 10000;
    bool enabled{true};
    urho::Color color{0, 0.6, 0, 1};
    goal_marker_info goal_marker;
    sizet entry_count;
    vec3 path_entries[MAX_LINE_ENTRIES];
};

struct measure_points
{
    static constexpr sizet MAX_ENTRIES = 1000;
    vec3 entries[MAX_ENTRIES];
    float marker_rad{0.02f};
    urho::Color color{0, 0, 1.0, 1};
    i16 entry_count{0};
    urho::Text *path_len_text;
};

struct map_panel
{
    bool js_enabled{false};
    urho::View3D *view{};

    cam_control_widget cam_cwidget{};
    toolbar_widget toolbar{};
    accept_param_input_widget accept_inp{};
    text_notice_widget text_disp{};
    map_toggle_views_panel views_panel{};

    urho::Node *lidar_node{};
    urho::BillboardSet *scan_bb{};

    occ_grid_map map{};
    occ_grid_map glob_cmap{};
    occ_grid_map loc_cmap{};

    nav_path_view glob_npview{};
    nav_path_view loc_npview{};
    measure_points mpoints{};

    cam_image_view cam_view{};

    nav_goals goals{};

    urho::Node *base_link{};
    urho::Node *odom{};
    urho::Text *conn_text{};
    robot_control_ctxt *ctxt{};
    misc_stats cur_stats{};
    ss_router router;

    std::unordered_map<std::string, urho::Node *> node_lut;
};

void map_clear_occ_grid(occ_grid_map *ocg);
void map_panel_init(map_panel *jspanel, const ui_info &ui_inf, net_connection *conn, input_data *inp);
void map_panel_term(map_panel *jspanel);
