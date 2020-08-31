#include "game/Game.h"

#include "common/hs_Assert.h"

#include "render/Texture.h"
#include "render/VertexBuffer.h"
#include "render/ShaderManager.h"
#include "render/Render.h"
#include "render/hs_Image.h"

namespace hs
{

Game* g_Game{};

//------------------------------------------------------------------------------
RESULT CreateGame()
{
    hs_assert(!g_Game);
    g_Game = new Game();

    return R_OK;
}

//------------------------------------------------------------------------------
void DestroyGame()
{
    delete g_Game;
}

//------------------------------------------------------------------------------
Game::~Game()
{
}

//------------------------------------------------------------------------------
RESULT Game::InitWin32()
{
    return R_OK;
}

//------------------------------------------------------------------------------
void Game::Update(float dTime)
{
    dTime_ = dTime;
}

//------------------------------------------------------------------------------
float Game::GetDTime() const
{
    return dTime_;
}

}

