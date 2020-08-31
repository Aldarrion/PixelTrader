
#include "game/Game.h"
#include "render/Render.h"
#include "input/Input.h"

#include "common/Logging.h"
#include "common/Types.h"
#include "common/hs_Assert.h"

#include "platform/hs_Windows.h"

#include <chrono>

//------------------------------------------------------------------------------
HWND g_hwnd{};

//------------------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
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
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    g_hwnd = CreateWindowA(
        wCls.lpszClassName,
        "PixelTrader",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
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
int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    hs::Log(hs::LogLevel::Info, "VkRenderer start\n");

    int WIDTH = 1280;
    int HEIGHT = 720;

    // Window
    if (FAILED(InitWindow(WIDTH, HEIGHT, instance)))
        return -1;

    // Render
    if (FAILED(hs::CreateRender(WIDTH, HEIGHT)))
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

            auto hw = GetForegroundWindow();
            if (GetForegroundWindow() != g_hwnd)
                continue;

            switch (msg.message)
            {
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

            hs::g_Input->Update();
            hs::g_Game->Update(dTime);
            hs::g_Render->Update(dTime);

            hs::g_Input->EndFrame();
        }
    }

    hs::DestroyGame();
    hs::DestroyInput();
    hs::DestroyRender();

    return 0;
}

