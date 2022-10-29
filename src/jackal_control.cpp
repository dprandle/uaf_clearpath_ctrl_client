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
#include "Urho3D/Math/Rect.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Resource/JSONValue.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/UI/BorderImage.h"
#include "Urho3D/UI/UIEvents.h"
#include "Urho3D/UI/Button.h"
#include "Urho3D/UI/UIElement.h"
#include "Urho3D/UI/Sprite.h"

#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "ss_router.h"
#include "typedefs.h"
#include "math_utils.h"
#include "jackal_control.h"

#if defined(__EMSCRIPTEN__)
const int SERVER_PORT = 8080;
#include <emscripten/emscripten.h>
#else
const int SERVER_PORT = 4000;
#endif
const char *SERVER_IP = "192.168.1.103";

int main(int argc, char **argv)
{
    urho::ParseArguments(argc, argv);
    urho::Context urho_ctxt{};
    jackal_control_ctxt jctrl{};

    jctrl_alloc(&jctrl, &urho_ctxt);
    if (!jctrl_init(&jctrl))
        return 0;

    jctrl_exec(&jctrl);
    jctrl_terminate(&jctrl);
}

jctrl_uevent_handlers::jctrl_uevent_handlers(urho::Context *ctxt) : urho::Object(ctxt)
{
}

void jctrl_uevent_handlers::subscribe()
{
}

void jctrl_uevent_handlers::unsubscribe()
{
    UnsubscribeFromAllEvents();
}

intern void setup_global_keys(jackal_control_ctxt *ictxt)
{
    ictxt->input_map.name = "global";
}

intern vec2 handle_jostick_move(const ivec2 &cur_mpos, joystick_panel * jspanel, float dev_pixel_ratio_inv)
{
    float max_r = jspanel->outer_ring->GetMaxOffset().x_ / 3.0f;
    vec2 dir_vec = vec2{cur_mpos.x_, cur_mpos.y_} - jspanel->cached_mouse_pos;
    auto l = dir_vec.Length();
    if (l > max_r)
        dir_vec = dir_vec * (max_r / l);
    ivec2 new_pos = jspanel->cached_js_pos + ivec2(dir_vec.x_, dir_vec.y_);
    jspanel->js->SetPosition(new_pos);
    return dir_vec * (-1.0f / max_r);
}

intern void joystick_panel_init(velocity_info * write_vel, joystick_panel * jsp, urho::Context* uctxt, double dev_pixel_ratio_inv)
{
    // Get default style
    urho::ResourceCache *cache = uctxt->GetSubsystem<urho::ResourceCache>();
    auto *xmlFile = cache->GetResource<urho::XMLFile>("UI/jackal_style.xml");
    urho::UI *usi = uctxt->GetSubsystem<urho::UI>();
    urho::UIElement *root = usi->GetRoot();


    jsp->frame = new urho::BorderImage(uctxt);
    root->AddChild(jsp->frame);
    jsp->frame->SetColor({0.2f, 0.2f, 0.2f, 0.7f});
    jsp->frame->SetEnableAnchor(true);
    jsp->frame->SetMinAnchor(0.0f, 0.7f);
    jsp->frame->SetMaxAnchor(1.0f, 1.0f);

    jsp->outer_ring = new urho::BorderImage(uctxt);
    jsp->frame->AddChild(jsp->outer_ring);
    jsp->outer_ring->SetStyle("JoystickBorder", xmlFile);
    jsp->outer_ring->SetEnableAnchor(true);
    jsp->outer_ring->SetMinAnchor(0.5f, 0.5f);
    jsp->outer_ring->SetMaxAnchor(0.5f, 0.5f);

    ivec2 offset = jsp->outer_ring->GetImageRect().Size();
    offset.x_ *= dev_pixel_ratio_inv;
    offset.y_ *= dev_pixel_ratio_inv;

    jsp->outer_ring->SetMaxOffset(offset);
    jsp->outer_ring->SetPivot(0.5f, 0.5f);
    
    jsp->js = new urho::Button(uctxt);
    jsp->outer_ring->AddChild(jsp->js);
    jsp->js->SetStyle("Joystick", xmlFile);
    jsp->js->SetEnableAnchor(true);
    jsp->js->SetMinOffset({0, 0});

    offset = jsp->js->GetImageRect().Size();
    offset.x_ *= dev_pixel_ratio_inv;
    offset.y_ *= dev_pixel_ratio_inv;

    jsp->js->SetMaxOffset(offset);
    jsp->js->SetMinAnchor(0.5f, 0.5f);
    jsp->js->SetMaxAnchor(0.5f, 0.5f);
    jsp->js->SetPivot(0.5f, 0.5f);

    jsp->js->SubscribeToEvent(urho::E_DRAGMOVE, [jsp, dev_pixel_ratio_inv, write_vel](urho::StringHash type, urho::VariantMap& ev_data) {
        using namespace urho::DragMove;
        auto elem = (urho::UIElement*)ev_data[P_ELEMENT].GetPtr();
        if (elem == jsp->js)
        {
            auto dvec = handle_jostick_move({ev_data[P_X].GetInt(), ev_data[P_Y].GetInt()}, jsp, dev_pixel_ratio_inv);
            write_vel->angular = dvec.x_;
            write_vel->linear = dvec.y_;
        }
    });

    jsp->js->SubscribeToEvent(urho::E_DRAGEND, [jsp, write_vel](urho::StringHash type, urho::VariantMap& ev_data) {
        using namespace urho::DragEnd;
        auto elem = (urho::UIElement*)ev_data[P_ELEMENT].GetPtr();
        if (elem == jsp->js)
        {
            jsp->js->SetEnableAnchor(true);
            jsp->cached_js_pos = {};
            jsp->cached_mouse_pos = {};
            *write_vel = {};
        }
    });

    jsp->js->SubscribeToEvent(urho::E_DRAGBEGIN, [jsp, dev_pixel_ratio_inv](urho::StringHash type, urho::VariantMap& ev_data) {
        using namespace urho::DragBegin;
        auto elem = (urho::UIElement*)ev_data[P_ELEMENT].GetPtr();
        if (elem == jsp->js)
        {
            ivec2 pos = jsp->js->GetPosition();
            jsp->cached_js_pos = pos;
            ivec2 cmp;
            SDL_GetMouseState(&cmp.x_, &cmp.y_);
            jsp->cached_mouse_pos = vec2{cmp.x_, cmp.y_} * dev_pixel_ratio_inv;
            jsp->js->SetEnableAnchor(false);
        }
    });

}

intern void create_visuals(jackal_control_ctxt *ctxt)
{
    auto uctxt = ctxt->urho_ctxt;
    auto ueng = ctxt->urho_engine;
    auto graphics = uctxt->GetSubsystem<urho::Graphics>();
    graphics->SetWindowTitle("Jackal Control");

    joystick_panel_init(&ctxt->vel_cmd.vinfo, &ctxt->js_panel, ctxt->urho_ctxt, ctxt->dev_pixel_ratio_inv);

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
    zn->SetFogColor({0.0, 0.0, 0.0, 1.0});

    // Create a directional light
    urho::Node *light_node = ctxt->scene->CreateChild("dirlight");
    light_node->SetDirection(vec3(-0.0f, -0.5f, -1.0f));
    urho::Light *light = light_node->CreateComponent<urho::Light>();
    light->SetLightType(urho::LIGHT_DIRECTIONAL);
    light->SetColor(Color(1.0f, 1.0f, 1.0f));
    light->SetSpecularIntensity(5.0f);
    light->SetBrightness(1.0f);

    auto dpi = graphics->GetDisplayDPI();
}

void jctrl_alloc(jackal_control_ctxt *ctxt, urho::Context *urho_ctxt)
{
    ctxt->urho_ctxt = urho_ctxt;
    ctxt->urho_engine = new urho::Engine(ctxt->urho_ctxt);
    ctxt->event_handler = new jctrl_uevent_handlers(ctxt->urho_ctxt);
    input_alloc(&ctxt->input_dispatch, urho_ctxt);
}

int socket_connect()
{
    struct pollfd sckt_fd[1];

    sckt_fd[0].fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sckt_fd[0].fd == -1)
    {
        elog("Failed to create socket");
        return 0;
    }
    else
    {
        ilog("Created socket with fd %d", sckt_fd[0].fd);
    }
    
    fcntl(sckt_fd[0].fd, F_SETFL, O_NONBLOCK);
    sckt_fd[0].events = POLLOUT;

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (!inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr))
    {
        elog("Failed PTON");
        return -1;
    }

    if (connect(sckt_fd[0].fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0 && errno != EINPROGRESS)
    {
        elog("Failed connecting to server at %s on port %d - resulting fd %d - error %s", SERVER_IP, SERVER_PORT, sckt_fd[0].fd, strerror(errno));
        close(sckt_fd[0].fd);
        return -1;
    }
    else
    {
        ilog("Connected to server at %s on port %d", SERVER_IP, SERVER_PORT);
    }

    #ifndef __EMSCRIPTEN__
    int pret = poll(sckt_fd, 1, -1);
    if (pret == 0)
    {
        elog("Poll timed out connecting to server %s on port %d for fd %d", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
        close(sckt_fd[0].fd);
        return -1;
    }
    else if (pret == -1)
    {
        elog("Failed poll on trying to connect to server at %s on port %d - resulting fd %d - error %s", SERVER_IP, SERVER_PORT, sckt_fd[0].fd, strerror(errno));
        close(sckt_fd[0].fd);
        return -1;
    }
    else
    {
        if (test_flags(sckt_fd[0].revents, POLLERR))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLERR", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
            close(sckt_fd[0].fd);
            return -1;
        }
        else if (test_flags(sckt_fd[0].revents, POLLHUP))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLHUP", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
            close(sckt_fd[0].fd);
            return -1;
        }
        else if (test_flags(sckt_fd[0].revents, POLLNVAL))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLNVAL", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
            close(sckt_fd[0].fd);
            return -1;
        }
    }
    #endif

    ilog("Successfully connected to server at %s on port %d - resulting fd %d", SERVER_IP, SERVER_PORT, sckt_fd[0].fd);
    return sckt_fd[0].fd;
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
    if (!ctxt->urho_engine->Initialize(engine_params))
        return false;

    #if defined(__EMSCRIPTEN__)
    ctxt->dev_pixel_ratio_inv = 1.0 / emscripten_get_device_pixel_ratio();
    #endif

    create_visuals(ctxt);

    input_init(&ctxt->input_dispatch);
    setup_global_keys(ctxt);
    ctxt->input_dispatch.context_stack.push_back(&ctxt->input_map);

    ilog("Device pixel ratio inverse: %f", ctxt->dev_pixel_ratio_inv);

    ctxt->socket_fd = socket_connect();

    ctxt->event_handler->subscribe();
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
    if (ctxt->socket_fd > 0)
    {
        close(ctxt->socket_fd);
    }
}

void jctrl_run_frame(jackal_control_ctxt *ctxt)
{
    static constexpr int BUF_SIZE = 10 * MB_SIZE;
    static u8 read_buf[BUF_SIZE];
    // static int cur_ind = 0;

    ctxt->urho_engine->RunFrame();

    if (ctxt->socket_fd > 0)
    {
        int rd_cnt = read(ctxt->socket_fd, read_buf, BUF_SIZE);
        if (rd_cnt >1)
        {
            ilog("Read %d bytes from socket", rd_cnt);
            for (int i = 0; i < rd_cnt; ++i)
            {
                ilog("%x", read_buf[i]);
            }
        }
        else if (rd_cnt != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            elog("Got read error %s", strerror(errno));
        }

        if (!ctxt->js_panel.js->GetEnableAnchor())
        {
            sizet bytes = sizeof(command_velocity);
            sizet bwritten = write(ctxt->socket_fd, &ctxt->vel_cmd, sizeof(command_velocity));
            if (bwritten == -1)
            {
                elog("Got write error %d", strerror(errno));
            }
            else if (bytes != bwritten)
            {
                wlog("Bytes written was only %d when tried to write %d", bwritten, bytes);
            }
        }

    }
}