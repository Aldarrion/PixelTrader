#include "game/Game.h"

#include "game/TileRenderer.h"

#include "render/Texture.h"
#include "render/VertexBuffer.h"
#include "render/ShaderManager.h"
#include "render/Render.h"
#include "render/hs_Image.h"

#include "input/Input.h"

#include "common/Logging.h"
#include "common/hs_Assert.h"

namespace hs
{

//------------------------------------------------------------------------------
RESULT AnimationState::Init(const Array<AnimationSegment>& segments)
{
    if (segments.IsEmpty())
        return R_FAIL;

    segments_ = segments;
    currentSegment_ = 0;
    timeToSwap_ = segments_[0].time_;

    return R_OK;
}

//------------------------------------------------------------------------------
void AnimationState::Update(float dTime)
{
    timeToSwap_ -= dTime;
    while (timeToSwap_ <= 0)
    {
        currentSegment_ = (currentSegment_ + 1) % segments_.Count();
        timeToSwap_ += segments_[currentSegment_].time_;
    }
}

//------------------------------------------------------------------------------
Tile* AnimationState::GetCurrentTile() const
{
    hs_assert(currentSegment_ < segments_.Count());
    return segments_[currentSegment_].tile_;
}

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
    delete goldChestTex_;
    delete forestTex_;
    delete forestDoorTex_;

    for (uint i = 0; i < hs_arr_len(rockTex_); ++i)
        delete rockTex_[i];
}

//------------------------------------------------------------------------------
void Game::AddTile(const Vec3& pos, Tile* tile)
{
    tiles_.Positions.Add(pos);
    tiles_.Tiles.Add(tile);
}

//------------------------------------------------------------------------------
void Game::AddAnimatedTile(const Vec3& pos, const AnimationState& animation)
{
    characters_.Positions.Add(pos);
    characters_.Animations.Add(animation);
    characters_.Tiles.Add(animation.GetCurrentTile());
}

//------------------------------------------------------------------------------
void Game::AnimateTiles()
{
    for (int i = 0; i < characters_.Animations.Count(); ++i)
    {
        characters_.Animations[i].Update(dTime_);
        characters_.Tiles[i] = characters_.Animations[i].GetCurrentTile();
    }
}

//------------------------------------------------------------------------------
Vec3 TilePos(float x, float y, float z = 0)
{
    return Vec3{ (float)x * TILE_SIZE, (float)y * TILE_SIZE, (float)z };
}

//------------------------------------------------------------------------------
RESULT MakeSimpleTile(const char* texPath, const char* name, Tile& t, Texture** tex)
{
    if (HS_FAILED(Texture::CreateTex2D(texPath, name, tex)))
        return R_FAIL;

    t.size_ = Vec2{ (float)(*tex)->GetWidth(), (float)(*tex)->GetHeight() };
    t.uvBox_ = Vec4{ 0, 0, 1, 1 };
    t.texture_ = *tex;

    return R_OK;
}

//------------------------------------------------------------------------------
RESULT Game::InitWin32()
{
    if (HS_FAILED(Texture::CreateTex2D("textures/Ground1.png", "GrassTile", &groundTileTex_)))
        return R_FAIL;

    constexpr float uvSize = 16.0f / (3 * 16.0f);
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            Tile t{};

            t.texture_ = groundTileTex_;
            t.uvBox_ = Vec4{ uvSize * x, uvSize * y, uvSize, uvSize };
            t.size_ = Vec2{ 16, 16 };

            groundTile_[3 * y + x] = t;
        }
    }

    if (HS_FAILED(MakeSimpleTile("textures/GoldChest.png", "GoldChest", goldChestTile_, &goldChestTex_)))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleTile("textures/Forest.png", "Forest", forestTile_, &forestTex_)))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleTile("textures/ForestDoor.png", "ForestDoor", forestDoorTile_, &forestDoorTex_)))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleTile("textures/Rock1.png", "Rock01", rockTile_[0], &rockTex_[0])))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleTile("textures/Rock2.png", "Rock02", rockTile_[1], &rockTex_[1])))
        return R_FAIL;

    Array<AnimationSegment> rockIdleSegments;
    for (uint i = 0; i < hs_arr_len(rockTex_); ++i)
        rockIdleSegments.Add(AnimationSegment{ &rockTile_[i], 0.5f });

    AnimationState rockIdle{};
    rockIdle.Init(rockIdleSegments);


    // ------------------------
    // Create initial map state
    AddTile(TilePos(0, 0.5f, 1), &goldChestTile_);

    int left = -6;
    int width = 15;
    int height = 10;

    AddTile(TilePos(left, 0.4f, 3), &forestTile_);

    AddTile(TilePos(left + 2, 0.3f, 2), &forestDoorTile_);

    int y = 0;
    AddTile(TilePos(left, y), &groundTile_[TOP_LEFT]);
    for (int i = 0; i < width; ++i)
        AddTile(TilePos(left + 1 + i, y), &groundTile_[TOP]);
    AddTile(TilePos(left + width + 1, y), &groundTile_[TOP_RIGHT]);
    
    int j = 0;
    for (; j < height; j++)
    {
        y = -1 - j;
        AddTile(TilePos(left, y), &groundTile_[MID_LEFT]);
        for (int i = 0; i < width; ++i)
            AddTile(TilePos(left + 1 + i, y), &groundTile_[MID]);
        AddTile(TilePos(left + width + 1, y), &groundTile_[MID_RIGHT]);
    }

    y = -j - 1;
    AddTile(TilePos(left, y), &groundTile_[BOT_LEFT]);
    for (int i = 0; i < width; ++i)
        AddTile(TilePos(left + 1 + i, y), &groundTile_[BOT]);
    AddTile(TilePos(left + width + 1, y), &groundTile_[BOT_RIGHT]);

    AddAnimatedTile(TilePos(-2, 0.5f, 1), rockIdle);

    return R_OK;
}


//------------------------------------------------------------------------------
void Game::Update(float dTime)
{
    dTime_ = dTime;

    // Move camera
    {
        // Assume "player" is the first character
        Vec3& pos = characters_.Positions[0];
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

        Camera& cam = g_Render->GetCamera();
        cam.SetPosition(Vec2{ pos.x, pos.y } + (characters_.Tiles[0]->size_ / 2.0f)); // Center the camera pos at the center of the player
        cam.UpdateMatrics();
    }

    AnimateTiles();

    // Draw calls
    TileRenderer* tr = g_Render->GetTileRenderer();

    tr->ClearTiles();

    // Regular tiles
    for (int i = 0; i < tiles_.Positions.Count(); ++i)
    {
        tr->AddTile(tiles_.Tiles[i], tiles_.Positions[i]);
    }

    // Animated tiles
    for (int i = 0; i < characters_.Tiles.Count(); ++i)
    {
        tr->AddTile(characters_.Tiles[i], characters_.Positions[i]);
    }
}

//------------------------------------------------------------------------------
float Game::GetDTime() const
{
    return dTime_;
}

//------------------------------------------------------------------------------
bool Game::IsWindowActive() const
{
    return isWindowActive_;
}

//------------------------------------------------------------------------------
void Game::SetWindowActive(bool isActive)
{
    isWindowActive_ = isActive;
}

}

