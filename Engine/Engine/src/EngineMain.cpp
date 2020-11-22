
#include "Game/GameBase.h"
#include "Render/Render.h"
#include "Input/Input.h"
#include "Resources/ResourceManager.h"
#include "Engine.h"

#include "Common/Logging.h"
#include "Common/Types.h"
#include "Common/hs_Assert.h"

#include "Platform/hs_Windows.h"

#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui.h"

#include <chrono>

// Forward declarations
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace hs
{
//------------------------------------------------------------------------------
HWND g_hwnd{};

//------------------------------------------------------------------------------
static bool g_isWindowActive = false;

//------------------------------------------------------------------------------
enum class WindowState
{
    Windowed,
    BorderlessFs,
    Count,
};
static WindowState g_WindowState = WindowState::Windowed;
static uint g_WindowWidth = 1280;
static uint g_WindowHeight = 720;
static bool g_DisableSizeChange = false;

//------------------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //LOG_DBG("Send: %x", msg);
    switch (msg)
    {
        case WM_NCACTIVATE:
        case WM_ACTIVATEAPP:
        case WM_ACTIVATE:
        {
            if (wParam)
                g_isWindowActive = true;
            else
                g_isWindowActive = false;
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        case WM_CLOSE:
            DestroyWindow(hWnd);
            g_hwnd = nullptr;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_WINDOWPOSCHANGING:
        {
            if (g_DisableSizeChange)
                ((WINDOWPOS*)lParam)->flags |= SWP_NOSIZE;
            else
                return DefWindowProc(hWnd, msg, wParam, lParam);
            break;
        }
        case WM_SYSCOMMAND:
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

//------------------------------------------------------------------------------
static uint GetWindowStyle(WindowState state)
{
    uint windowStyle;
    if (state == WindowState::BorderlessFs)
    {
        windowStyle = WS_POPUP;
    }
    else // WindowState::Windowed
    {
        windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    }

    return windowStyle;
}

//------------------------------------------------------------------------------
static RESULT InitWindow(int width, int height, HINSTANCE instance)
{
    WNDCLASSA wCls{};
    wCls.style = CS_HREDRAW | CS_VREDRAW;
    wCls.lpfnWndProc = WndProc;
    wCls.cbClsExtra = 0;
    wCls.cbWndExtra = 0;
    wCls.hInstance = instance;
    wCls.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wCls.lpszClassName = "VkRenderWindowClass";
    if (!RegisterClassA(&wCls))
    {
        Log(LogLevel::Error, "Failed to register window class, terminating");
        return R_FAIL;
    }

    RECT rc = { 0, 0, width, height};

    uint windowStyle = GetWindowStyle(g_WindowState);

    AdjustWindowRect(&rc, windowStyle, FALSE);

    g_hwnd = CreateWindowA(
        wCls.lpszClassName,
        "PixelTrader",
        windowStyle,
        0, 0,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr,
        instance,
        nullptr
    );

    if (!g_hwnd)
    {
        Log(LogLevel::Error, "Failed to create window, terminating");
        return R_FAIL;
    }

    auto hr = ShowWindow(g_hwnd, SW_SHOW);

    if (HS_FAILED(hr))
    {
        Log(LogLevel::Error, "Failed to show window, terminating");
        return R_FAIL;
    }

    return R_OK;
}

//------------------------------------------------------------------------------
static RESULT HsInitImguiWin32()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hwnd);

    return R_OK;
}

//------------------------------------------------------------------------------
static void HsDestroyImgui()
{
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

//------------------------------------------------------------------------------
static void ToggleFullscreen()
{
    g_DisableSizeChange = false;
    uint wsInt = (static_cast<uint>(g_WindowState) + 1) % static_cast<uint>(WindowState::Count);
    auto newState = static_cast<WindowState>(wsInt);

    int width = g_WindowWidth;
    int height = g_WindowHeight;
    if (newState == WindowState::BorderlessFs)
    {
        HMONITOR monitor = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO monitorInfo{ sizeof(MONITORINFO) };
        if (GetMonitorInfo(monitor, &monitorInfo) == 0)
        {
            LOG_ERR("GetMonitorInfoA failed");
            return;
        }

        const RECT& rect = monitorInfo.rcMonitor;
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;

        hs_assert(width > 0 && height > 0);
    }

    uint windowStyle = GetWindowStyle(newState);
    SetWindowLongPtrA(g_hwnd, GWL_STYLE, windowStyle);

    RECT rc = { 0, 0, width, height};
    AdjustWindowRect(&rc, windowStyle, FALSE);

    SetWindowPos(g_hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_FRAMECHANGED | SWP_NOMOVE);
    ShowWindow(g_hwnd, SW_SHOW);

    if (HS_FAILED(g_Render->OnWindowResized(width, height)))
    {
        LOG_ERR("Failed to Render handle resize window");
        // TODO(pavel): Do something more useful here, but this fail is pretty serious, Render needs to be able to recover too
        hs_assert(false);
        return;
    }

    if (HS_FAILED(g_GameBase->OnWindowResized()))
    {
        LOG_ERR("Failed to Game handle resize window");
        return;
    }

    g_WindowState = newState;
    g_DisableSizeChange = true;
}

//------------------------------------------------------------------------------
void ParseCmdLine(const char* commandLine, uint& width, uint& height, WindowState& windowStyle)
{
    constexpr const char* WINDOW_STR = "-window";
    constexpr const char* FULLSCREEN_STR = "-fullscreen";

    const char* windowStart = strstr(commandLine, WINDOW_STR);
    if (windowStart)
    {
        windowStart += strlen(WINDOW_STR);
        uint w, h;

        int matched = sscanf(windowStart, "=%u,%u%*c", &w, &h);
        if (matched == 2)
        {
            g_WindowWidth = width = w;
            g_WindowHeight = height = h;
            windowStyle = WindowState::Windowed;
        }
    }

    const char* fullscreenStart = strstr(commandLine, FULLSCREEN_STR);
    if (!windowStart && fullscreenStart)
    {
        windowStyle = WindowState::BorderlessFs;
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }
}

//------------------------------------------------------------------------------
int EngineMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    Log(LogLevel::Info, "VkRenderer start\n");

    uint width = g_WindowWidth;
    uint height = g_WindowHeight;

    ParseCmdLine(cmdLine, width, height, g_WindowState);

    // Window
    if (HS_FAILED(InitWindow(width, height, instance)))
        return -1;

    // Engine
    if (HS_FAILED(CreateEngine()))
    {
        Log(LogLevel::Error, "Failed to create engine");
        return -1;
    }
    hs_assert(g_Engine);

    if (HS_FAILED(g_Engine->InitWin32()))
    {
        Log(LogLevel::Error, "Failed to init engine");
        return -1;
    }

    // Resource manager
    if (HS_FAILED(CreateResourceManager()))
    {
        Log(LogLevel::Error, "Failed to create resource manager");
        return -1;
    }
    hs_assert(g_ResourceManager);

    if (HS_FAILED(g_ResourceManager->InitWin32()))
    {
        Log(LogLevel::Error, "Failed to init resource manager");
        return -1;
    }

    // Render
    if (HS_FAILED(CreateRender(width, height)))
    {
        Log(LogLevel::Error, "Failed to create render");
        return -1;
    }
    hs_assert(g_Render);

    if (HS_FAILED(g_Render->InitWin32(g_hwnd, instance)))
    {
        Log(LogLevel::Error, "Failed to init render");
        return -1;
    }

    // Imgui
    if (HS_FAILED(HsInitImguiWin32()))
    {
        Log(LogLevel::Error, "Failed to init Imgui for Win32");
        return -1;
    }

    if (HS_FAILED(g_Render->InitImgui()))
    {
        Log(LogLevel::Error, "Failed to init render for Imgui");
        return -1;
    }

    // Input
    if (HS_FAILED(CreateInput()))
    {
        Log(LogLevel::Error, "Failed to crete input");
        return -1;
    }
    hs_assert(g_Input);

    if (HS_FAILED(g_Input->InitWin32(g_hwnd)))
    {
        Log(LogLevel::Error, "Failed to init input");
        return -1;
    }

    // Game
    if (HS_FAILED(CreateGame()))
    {
        Log(LogLevel::Error, "Failed to crete game");
        return -1;
    }
    hs_assert(g_GameBase);

    if (HS_FAILED(g_GameBase->InitWin32()))
    {
        Log(LogLevel::Error, "Failed to init game");
        return -1;
    }

    bool shouldQuit = false;

    auto start = std::chrono::high_resolution_clock::now();

    MSG msg{};
    g_DisableSizeChange = true;
    while (!shouldQuit)
    {
        //LOG_DBG("--- Frame");
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE | PM_NOYIELD))
        {
            // TODO(pavel): This is ify, could this be a problem for messges such as WM_QUIT? Also add imgui activation.
            ImGui_ImplWin32_WndProcHandler(g_hwnd, msg.message, msg.wParam, msg.lParam);
            const auto& io = ImGui::GetIO();
            if (io.WantCaptureMouse || io.WantCaptureKeyboard)
                continue;

            //LOG_DBG("Post: %x", msg.message);
            switch (msg.message)
            {
                case WM_SYSCHAR:
                    if (msg.wParam == VK_RETURN && (HIWORD(msg.lParam) & KF_ALTDOWN))
                        ToggleFullscreen();
                    else if (msg.wParam == VK_F4 && (HIWORD(msg.lParam) & KF_ALTDOWN))
                        shouldQuit = true;
                    break;
                case WM_QUIT:
                    shouldQuit = true;
                    break;
                case WM_KEYDOWN:
                {
                    // If bit 30 is 1 the key was down even before, and this is just a repeat event
                    if (msg.lParam & (1 << 30))
                        break;

                    g_Input->KeyDown(msg.wParam);
                    break;
                }
                case WM_KEYUP:
                {
                    if (msg.wParam == VK_F5)
                        (void)g_Render->ReloadShaders();
                    g_Input->KeyUp(msg.wParam);
                    break;
                }
                case WM_LBUTTONDOWN:
                    g_Input->ButtonDown(BTN_LEFT);
                    break;
                case WM_RBUTTONDOWN:
                    g_Input->ButtonDown(BTN_RIGHT);
                    break;
                case WM_MBUTTONDOWN:
                    g_Input->ButtonDown(BTN_MIDDLE);
                    break;
                case WM_LBUTTONUP:
                    g_Input->ButtonUp(BTN_LEFT);
                    break;
                case WM_RBUTTONUP:
                    g_Input->ButtonUp(BTN_RIGHT);
                    break;
                case WM_MBUTTONUP:
                    g_Input->ButtonUp(BTN_MIDDLE);
                    break;
                case WM_NCMOUSELEAVE:
                    break;
                default:
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    break;
            }
        }
        else
        {
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            start = std::chrono::high_resolution_clock::now();

            float dTime = elapsed.count() / (1000.0f * 1000 * 1000);
            dTime = Min(dTime, 0.5f);

            g_Engine->SetWindowActive(g_isWindowActive);

            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            g_Input->Update();
            g_Engine->Update(dTime);
            g_GameBase->Update();
            g_Render->Update(dTime);

            g_Input->EndFrame();
        }
    }

    // Cleanup
    DestroyGame();
    DestroyInput();
    DestroyRender();
    DestroyResourceManager();
    DestroyEngine();
    HsDestroyImgui();

    return 0;
}

}
