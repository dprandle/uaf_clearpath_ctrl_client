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
#include "Urho3D/UI/Button.h"
#include "Urho3D/UI/UIElement.h"
#include "Urho3D/UI/Sprite.h"

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
const int SERVER_PORT = 3000;
#endif
const char *SERVER_IP = "127.0.0.1";

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

intern void create_visuals(jackal_control_ctxt *ctxt)
{
    auto uctxt = ctxt->urho_ctxt;
    auto ueng = ctxt->urho_engine;
    auto graphics = uctxt->GetSubsystem<urho::Graphics>();
    graphics->SetWindowTitle("Jackal Control");

    // Get default style
    urho::ResourceCache *cache = uctxt->GetSubsystem<urho::ResourceCache>();
    auto *xmlFile = cache->GetResource<urho::XMLFile>("UI/jackal_style.xml");
    urho::UI *usi = uctxt->GetSubsystem<urho::UI>();
    urho::UIElement *root = usi->GetRoot();

    // auto btn = new urho::Button(uctxt);
    // root->AddChild(btn);
    // btn->SetStyleAuto(xmlFile);
    // btn->SetEnableAnchor(true);
    // btn->SetMinAnchor(0.1, 0.25);
    // btn->SetMaxAnchor(0.75, 0.75);

    auto joystick = new urho::Button(uctxt);
    root->AddChild(joystick);
    joystick->SetStyle("Joystick", xmlFile);
    //joystick->SetPosition(vec2(500,500));
    joystick->SetEnableAnchor(true);
    int offset = 132;
    joystick->SetMinOffset({0, 0});
    joystick->SetMaxOffset({offset, offset});
    joystick->SetMinAnchor(0.5f, 0.75f);
    joystick->SetMaxAnchor(0.5f, 0.75f);
    joystick->SetPivot(0.5f, 0.5f);
    //joystick->SetSize(1000,1000);
    //joystick->SetFullImageRect();


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

int socket_connect()
{
    struct pollfd sckt_fd;

    sckt_fd.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sckt_fd.fd == -1)
    {
        elog("Failed to create socket");
        return 0;
    }
    fcntl(sckt_fd.fd, F_SETFL, O_NONBLOCK);

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (!inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr))
    {
        elog("Failed PTON");
        return -1;
    }

    if (connect(sckt_fd.fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0 && errno != EINPROGRESS)
    {
        elog("Failed connecting to server at %s on port %d - resulting fd %d - error %s", SERVER_IP, SERVER_PORT, sckt_fd.fd, strerror(errno));
        return -1;
    }

    #ifndef __EMSCRIPTEN__
    int pret = poll(&sckt_fd, 1, 10000);
    if (pret == 0)
    {
        elog("Poll timed out connecting to server %s on port %d", SERVER_IP, SERVER_PORT);
        return -1;
    }
    else if (pret == -1)
    {
        elog("Failed poll on trying to connect to server at %s on port %d - resulting fd %d - error %s", SERVER_IP, SERVER_PORT, sckt_fd.fd, strerror(errno));
        return -1;
    }
    else
    {
        if (test_flags(sckt_fd.revents, POLLERR))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLERR", SERVER_IP, SERVER_PORT, sckt_fd.fd);
            return -1;
        }
        else if (test_flags(sckt_fd.revents, POLLHUP))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLHUP", SERVER_IP, SERVER_PORT, sckt_fd.fd);
            return -1;
        }
        else if (test_flags(sckt_fd.revents, POLLNVAL))
        {
            elog("Failed connecting to server at %s on port %d - resulting fd %d - POLLNVAL", SERVER_IP, SERVER_PORT, sckt_fd.fd);
            return -1;
        }
    }
    #endif

    ilog("Successfully connected to server at %s on port %d - resulting fd %d", SERVER_IP, SERVER_PORT, sckt_fd.fd);
    return sckt_fd.fd;
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
    engine_params[urho::EP_WINDOW_WIDTH] = 1440;
    engine_params[urho::EP_WINDOW_HEIGHT] = 2960;
#if !defined(__EMSCRIPTEN__)
    engine_params[urho::EP_HIGH_DPI] = true;
    engine_params[urho::EP_WINDOW_RESIZABLE] = true;
#endif
    if (!ctxt->urho_engine->Initialize(engine_params))
        return false;

    create_visuals(ctxt);

    input_init(&ctxt->input_dispatch);
    setup_global_keys(ctxt);
    ctxt->input_dispatch.context_stack.push_back(&ctxt->input_map);

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
        if (rd_cnt >= 0)
        {
            ilog("Read %d bytes from socket", rd_cnt);
            for (int i = 0; i < rd_cnt; ++i)
            {
                ilog("%x", read_buf[i]);
            }
        }
        else if (rd_cnt != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            elog("Got read error %d", strerror(errno));
        }
    }
}