
#include "game/Game.h"
#include "render/Render.h"
#include "input/Input.h"
#include "resources/ResourceManager.h"

#include "common/Logging.h"
#include "common/Types.h"
#include "common/hs_Assert.h"

#include "platform/hs_Windows.h"

#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui.h"

#include <chrono>

// Forward declarations
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

//------------------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

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
static hs::RESULT InitWindow(int width, int height, HINSTANCE instance)
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
        Log(hs::LogLevel::Error, "Failed to register window class, terminating");
        return hs::R_FAIL;
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
        hs::Log(hs::LogLevel::Error, "Failed to create window, terminating");
        return hs::R_FAIL;
    }

    auto hr = ShowWindow(g_hwnd, SW_SHOW);

    if (FAILED(hr))
    {
        hs::Log(hs::LogLevel::Error, "Failed to show window, terminating");
        return hs::R_FAIL;
    }

    return hs::R_OK;
}

//------------------------------------------------------------------------------
static hs::RESULT HsInitImguiWin32()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hwnd);

    return hs::R_OK;
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
    uint wsInt = (static_cast<uint>(g_WindowState) + 1) % static_cast<uint>(WindowState::Count);
    g_WindowState = static_cast<WindowState>(wsInt);

    int width = 1280;
    int height = 720;
    if (g_WindowState == WindowState::BorderlessFs)
    {
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }

    uint windowStyle = GetWindowStyle(g_WindowState);
    SetWindowLongPtrA(g_hwnd, GWL_STYLE, windowStyle);

    RECT rc = { 0, 0, width, height};
    AdjustWindowRect(&rc, windowStyle, FALSE);

    SetWindowPos(g_hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_FRAMECHANGED | SWP_NOMOVE);
    ShowWindow(g_hwnd, SW_SHOW);

    hs::g_Render->ResizeWindow(width, height);
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
            width = w;
            height = h;
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
int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    hs::Log(hs::LogLevel::Info, "VkRenderer start\n");

    uint width = 1280;
    uint height = 720;

    ParseCmdLine(cmdLine, width, height, g_WindowState);

    // Window
    if (FAILED(InitWindow(width, height, instance)))
        return -1;

    // Resource manager
    if (FAILED(hs::CreateResourceManager()))
    {
        hs::Log(hs::LogLevel::Error, "Failed to create resource manager");
        return -1;
    }
    hs_assert(hs::g_ResourceManager);

    if (FAILED(hs::g_ResourceManager->InitWin32()))
    {
        hs::Log(hs::LogLevel::Error, "Failed to init resource manager");
        return -1;
    }

    // Render
    if (FAILED(hs::CreateRender(width, height)))
    {
        hs::Log(hs::LogLevel::Error, "Failed to create render");
        return -1;
    }
    hs_assert(hs::g_Render);

    if (FAILED(hs::g_Render->InitWin32(g_hwnd, instance)))
    {
        hs::Log(hs::LogLevel::Error, "Failed to init render");
        return -1;
    }

    // Imgui
    if (FAILED(HsInitImguiWin32()))
    {
        hs::Log(hs::LogLevel::Error, "Failed to init Imgui for Win32");
        return -1;
    }

    if (FAILED(hs::g_Render->InitImgui()))
    {
        hs::Log(hs::LogLevel::Error, "Failed to init render for Imgui");
        return -1;
    }

    // Input
    if (FAILED(hs::CreateInput()))
    {
        hs::Log(hs::LogLevel::Error, "Failed to crete input");
        return -1;
    }
    hs_assert(hs::g_Input);

    if (FAILED(hs::g_Input->InitWin32(g_hwnd)))
    {
        hs::Log(hs::LogLevel::Error, "Failed to init input");
        return -1;
    }

    // Game
    if (FAILED(hs::CreateGame()))
    {
        hs::Log(hs::LogLevel::Error, "Failed to crete game");
        return -1;
    }
    hs_assert(hs::g_Game);

    if (FAILED(hs::g_Game->InitWin32()))
    {
        hs::Log(hs::LogLevel::Error, "Failed to init game");
        return -1;
    }

    bool shouldQuit = false;

    auto start = std::chrono::high_resolution_clock::now();

    MSG msg{};
    while (!shouldQuit)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE | PM_NOYIELD))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (g_hwnd && GetForegroundWindow() != g_hwnd)
                continue;

            // TODO(pavel): This is ify, could this be a problem for messges such as WM_QUIT? Also add imgui activation.
            ImGui_ImplWin32_WndProcHandler(g_hwnd, msg.message, msg.wParam, msg.lParam);
            const auto& io = ImGui::GetIO();
            if (io.WantCaptureMouse || io.WantCaptureKeyboard)
                continue;

            switch (msg.message)
            {
                case WM_SYSCHAR:
                    if (msg.wParam == VK_RETURN && (HIWORD(msg.lParam) & KF_ALTDOWN))
                        ToggleFullscreen();
                    break;
                case WM_QUIT:
                    shouldQuit = true;
                    break;
                case WM_KEYDOWN:
                {
                    // If bit 30 is 1 the key was down even before, and this is just a repeat event
                    if (msg.lParam & (1 << 30))
                        break;

                    if (msg.wParam == VK_ESCAPE)
                        shouldQuit = true;
                    hs::g_Input->KeyDown(msg.wParam);
                    break;
                }
                case WM_KEYUP:
                {
                    if (msg.wParam == VK_F5)
                        hs::g_Render->ReloadShaders();
                    hs::g_Input->KeyUp(msg.wParam);
                    break;
                }
                case WM_LBUTTONDOWN:
                    hs::g_Input->ButtonDown(hs::BTN_LEFT);
                    break;
                case WM_RBUTTONDOWN:
                    hs::g_Input->ButtonDown(hs::BTN_RIGHT);
                    break;
                case WM_MBUTTONDOWN:
                    hs::g_Input->ButtonDown(hs::BTN_MIDDLE);
                    break;
                case WM_LBUTTONUP:
                    hs::g_Input->ButtonUp(hs::BTN_LEFT);
                    break;
                case WM_RBUTTONUP:
                    hs::g_Input->ButtonUp(hs::BTN_RIGHT);
                    break;
                case WM_MBUTTONUP:
                    hs::g_Input->ButtonUp(hs::BTN_MIDDLE);
                    break;

                default:
                    break;
            }
        }
        else
        {
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            start = std::chrono::high_resolution_clock::now();

            float dTime = elapsed.count() / (1000.0f * 1000 * 1000);

            hs::g_Game->SetWindowActive(g_isWindowActive);

            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            hs::g_Input->Update();
            hs::g_Game->Update(dTime);
            hs::g_Render->Update(dTime);

            hs::g_Input->EndFrame();
        }
    }

    // Cleanup
    hs::DestroyGame();
    hs::DestroyInput();
    hs::DestroyRender();
    hs::DestroyResourceManager();
    HsDestroyImgui();

    return 0;
}

