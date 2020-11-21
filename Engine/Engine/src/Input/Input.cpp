#include "input/Input.h"

#include "Engine.h"

#include "render/Render.h"

#include "common/Logging.h"

#include "platform/hs_Windows.h"

namespace hs
{

//------------------------------------------------------------------------------
Input* g_Input{};

//------------------------------------------------------------------------------
RESULT CreateInput()
{
    g_Input = new Input();

    return R_OK;
}

//------------------------------------------------------------------------------
void DestroyInput()
{
    delete g_Input;
}

//------------------------------------------------------------------------------
RESULT Input::InitWin32(HWND hwnd)
{
    hwnd_ = hwnd;

    return R_OK;
}

//------------------------------------------------------------------------------
void Input::Update()
{
    if (mouseMode_ == MouseMode::Relative)
    {
        Vec2 mousePos = GetMousePos();
        mouseDelta_ = mousePos - Vec2{ g_Render->GetWidth() / 2.0f, g_Render->GetHeight() / 2.0f };

        CenterCursor();
    }
}

//------------------------------------------------------------------------------
void Input::EndFrame()
{
    memset(&buttons_, 0, sizeof(buttons_));
    keysDown_.Clear();
    keysUp_.Clear();
    mouseDelta_ = {};
}

//------------------------------------------------------------------------------
Vec2 Input::GetMousePos() const
{
    POINT cursorPos;
    if (GetCursorPos(&cursorPos) == 0)
    {
        Log(LogLevel::Error, "Could not retrieve the cursor position, error: %d", GetLastError());
    }
    else
    {
        if (!ScreenToClient(hwnd_, &cursorPos))
        {
            Log(LogLevel::Error, "Could not cast cursor pos to client pos, error: %d", GetLastError());
        }
        else
        {
            return Vec2{ (float)cursorPos.x, (float)cursorPos.y };
        }
    }

    return Vec2{};
}

//------------------------------------------------------------------------------
bool Input::IsKeyDown(int keyCode) const
{
    for (int i = 0; i < keysDown_.Count(); ++i)
    {
        if (keysDown_[i] == keyCode)
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool Input::IsKeyUp(int keyCode) const
{
    for (int i = 0; i < keysUp_.Count(); ++i)
    {
        if (keysUp_[i] == keyCode)
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool Input::GetState(int keyCode) const
{
    if (!g_Engine->IsWindowActive())
        return false;
    return (GetKeyState(keyCode) & 0x8000) != 0;
}

//------------------------------------------------------------------------------
bool Input::IsButtonDown(MouseButton btn) const
{
    hs_assert(btn < BTN_COUNT);

    return buttons_[btn] == ButtonState::Down;
}

//------------------------------------------------------------------------------
bool Input::IsButtonUp(MouseButton btn) const
{
    hs_assert(btn < BTN_COUNT);

    return buttons_[btn] == ButtonState::Up;
}

//------------------------------------------------------------------------------
void Input::KeyDown(int key)
{
    keysDown_.Add(key);
}

//------------------------------------------------------------------------------
void Input::KeyUp(int key)
{
    keysUp_.Add(key);
}

//------------------------------------------------------------------------------
void Input::ButtonDown(MouseButton button)
{
    hs_assert(button < BTN_COUNT);

    buttons_[button] = ButtonState::Down;
}

//------------------------------------------------------------------------------
void Input::ButtonUp(MouseButton button)
{
    hs_assert(button < BTN_COUNT);

    buttons_[button] = ButtonState::Up;
}

//------------------------------------------------------------------------------
void Input::CenterCursor()
{
    POINT cursorPos { (int)g_Render->GetWidth() / 2, (int)g_Render->GetHeight() / 2 };
    if (ClientToScreen(hwnd_, &cursorPos) == 0)
    {
        Log(LogLevel::Error, "Could not retrieve the window position, error: %d", GetLastError());
    }
    else
    {
        if (SetCursorPos(cursorPos.x, cursorPos.y) == 0)
            Log(LogLevel::Error, "Could not set the cursor position, error: %d", GetLastError());
    }
}

//------------------------------------------------------------------------------
void Input::SetMouseMode(MouseMode mode)
{
    if (mouseMode_ != mode)
    {
        mouseMode_ = mode;
        if (mouseMode_ == MouseMode::Relative)
        {
            CenterCursor();
            int showCount = ShowCursor(false);
            hs_assert(showCount < 0);
        }
        else
        {
            hs_assert(mouseMode_ == MouseMode::Absolute);
            int showCount = ShowCursor(true);
            hs_assert(showCount >= 0);
        }
    }
}

//------------------------------------------------------------------------------
Vec2 Input::GetMouseDelta() const
{
    return mouseDelta_;
}


}
