#pragma once

#include "Config.h"
#include "common/Types.h"
#include "common/Enums.h"
#include "containers/Array.h"

#include "math/hs_Math.h"

#include "platform/hs_Windows.h"

namespace hs
{

//------------------------------------------------------------------------------
extern class Input* g_Input;

//------------------------------------------------------------------------------
RESULT CreateInput();
void DestroyInput();

//------------------------------------------------------------------------------
enum MouseButton
{
    BTN_LEFT,
    BTN_MIDDLE,
    BTN_RIGHT,
    BTN_COUNT
};

//------------------------------------------------------------------------------
enum class MouseMode
{
    Absolute,
    Relative,
};

//------------------------------------------------------------------------------
template<class KeyT, class DataT>
struct Pair
{
    KeyT    Key;
    DataT   Data;
};

//------------------------------------------------------------------------------
class Input
{
public:
    RESULT InitWin32(HWND hwnd);

    void Update();
    void EndFrame();

    Vec2 GetMousePos() const;

    bool IsKeyDown(int keyCode) const;
    bool IsKeyUp(int keyCode) const;
    
    // For both keys and buttons
    bool GetState(int keyCode) const;

    void KeyDown(int key);
    void KeyUp(int key);

    bool IsButtonDown(MouseButton button) const;
    bool IsButtonUp(MouseButton button) const;

    void ButtonDown(MouseButton button);
    void ButtonUp(MouseButton button);

    void SetMouseMode(MouseMode mode);

    Vec2 GetMouseDelta() const;

private:
    // Win32
    HWND hwnd_;

    enum class ButtonState
    {
        None,
        Down,
        Up
    };

    ButtonState buttons_[BTN_COUNT]{};

    Array<int> keysDown_;
    Array<int> keysUp_;

    MouseMode mouseMode_{};
    Vec2 mouseDelta_{};

    void CenterCursor();
};

}
