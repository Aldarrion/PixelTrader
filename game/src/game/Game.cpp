#include "game/Game.h"

#include "game/TileRenderer.h"

#include "render/Texture.h"
#include "render/VertexBuffer.h"
#include "render/ShaderManager.h"
#include "render/Render.h"
#include "render/hs_Image.h"

#include "input/Input.h"

#include "common/hs_Assert.h"

namespace hs
{

//------------------------------------------------------------------------------
Game* g_Game{};

//------------------------------------------------------------------------------
static constexpr int TILE_SIZE = 16;

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
    delete groundTileTex_;
    for (uint i = 0; i < hs_arr_len(groundTile_); ++i)
    {
        delete groundTile_[i];
    }

    delete goldChestTex_;
    delete goldChestTile_;
}

//------------------------------------------------------------------------------
RESULT Game::InitWin32()
{
    if (HS_FAILED(Texture::CreateTex2D("textures/Ground1.png", "GrassTile", &groundTileTex_)))
        return R_FAIL;

    if (HS_FAILED(Texture::CreateTex2D("textures/GoldChest.png", "GoldChest", &goldChestTex_)))
        return R_FAIL;

    constexpr float uvSize = 16.0f / (3 * 16.0f);
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            Tile* t = new Tile();

            t->texture_ = groundTileTex_;
            t->uvBox_ = Vec4{ uvSize * x, uvSize * y, uvSize, uvSize };
            t->size_ = Vec2{ 16, 16 };

            groundTile_[3 * y + x] = t;
        }
    }

    goldChestTile_ = new Tile();
    goldChestTile_->size_ = Vec2{ 16, 16 };
    goldChestTile_->uvBox_ = Vec4{ 0, 0, 1, 1 };
    goldChestTile_->texture_ = goldChestTex_;

    return R_OK;
}

//------------------------------------------------------------------------------
Vec3 TilePos(float x, float y, float z = 0)
{
    return Vec3{ (float)x * TILE_SIZE, (float)y * TILE_SIZE, (float)z };
}

//------------------------------------------------------------------------------
void Game::Update(float dTime)
{
    dTime_ = dTime;

    // Move camera
    {
        Camera& cam = g_Render->GetCamera();
        Vec3 pos = cam.Position();

        float speed{ 40 };
        if (g_Input->GetState('W'))
        {
            pos += Vec3::UP() * speed * GetDTime();
        }
        else if (g_Input->GetState('S'))
        {
            pos -= Vec3::UP() * speed * GetDTime();
        }
    
        if (g_Input->GetState('D'))
        {
            pos += Vec3::RIGHT() * speed * GetDTime();
        }
        else if (g_Input->GetState('A'))
        {
            pos -= Vec3::RIGHT() * speed * GetDTime();
        }

        cam.SetPosition(pos);
        cam.UpdateMatrics();
    }

    // Draw calls
    TileRenderer* tr = g_Render->GetTileRenderer();

    tr->ClearTiles();

    tr->AddTile(goldChestTile_, TilePos(0, 0.5f, 1));

    int y = 0;
    tr->AddTile(groundTile_[TOP_LEFT], TilePos(-5, y));
    for (int i = 0; i < 10; ++i)
        tr->AddTile(groundTile_[TOP], TilePos(-4 + i, y));
    tr->AddTile(groundTile_[TOP_RIGHT], TilePos(6, y));
    
    int j = 0;
    for (; j < 10; j++)
    {
        y = -1 - j;
        tr->AddTile(groundTile_[MID_LEFT], TilePos(-5, y));
        for (int i = 0; i < 10; ++i)
            tr->AddTile(groundTile_[MID], TilePos(-4 + i, y));
        tr->AddTile(groundTile_[MID_RIGHT], TilePos(6, y));
    }

    y = -j - 1;
    tr->AddTile(groundTile_[BOT_LEFT], TilePos(-5, y));
    for (int i = 0; i < 10; ++i)
        tr->AddTile(groundTile_[BOT], TilePos(-4 + i, y));
    tr->AddTile(groundTile_[BOT_RIGHT], TilePos(6, y));

}

//------------------------------------------------------------------------------
float Game::GetDTime() const
{
    return dTime_;
}

}

