#include "Engine.h"

namespace hs
{

//------------------------------------------------------------------------------
Engine* g_Engine;

//------------------------------------------------------------------------------
RESULT CreateEngine()
{
    g_Engine = new Engine();

    return R_OK;
}

//------------------------------------------------------------------------------
void DestroyEngine()
{
    delete g_Engine;
}

//------------------------------------------------------------------------------
RESULT Engine::InitWin32()
{
    return R_OK;
}

//------------------------------------------------------------------------------
bool Engine::IsWindowActive() const
{
    return isWindowActive_;
}

//------------------------------------------------------------------------------
void Engine::SetWindowActive(bool isActive)
{
    isWindowActive_ = isActive;
}

//------------------------------------------------------------------------------
void Engine::Update(float dTime)
{
    dTime_ = dTime;
}

//------------------------------------------------------------------------------
float Engine::GetDTime() const
{
    return dTime_;
}

}
