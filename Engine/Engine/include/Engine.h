#pragma once

#include "Config.h"

#include "Common/Enums.h"

namespace hs
{

//------------------------------------------------------------------------------
extern class Engine* g_Engine;

//------------------------------------------------------------------------------
RESULT CreateEngine();
void DestroyEngine();

//------------------------------------------------------------------------------
class Engine
{
public:
    ~Engine() = default;

    RESULT InitWin32();

    bool IsWindowActive() const;
    void SetWindowActive(bool isActive);
    float GetDTime() const;

    void Update(float dTime);

private:
    float dTime_{};
    bool isWindowActive_{};
};

}
