#include "game/Game.h"

#include "game/TileRenderer.h"
#include "game/DebugShapeRenderer.h"

#include "render/Texture.h"
#include "render/VertexBuffer.h"
#include "render/ShaderManager.h"
#include "render/Render.h"
#include "render/hs_Image.h"

#include "resources/ResourceManager.h"

#include "input/Input.h"

#include "common/Logging.h"
#include "common/hs_Assert.h"

namespace hs
{

//------------------------------------------------------------------------------
Game* g_Game{};

//------------------------------------------------------------------------------
static constexpr int TILE_SIZE = 16;


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
Game::~Game() = default;

//------------------------------------------------------------------------------
void Game::AddTile(const Vec3& pos, Tile* tile)
{
    tiles_.Positions.Add(pos);
    tiles_.Tiles.Add(tile);
}

//------------------------------------------------------------------------------
void Game::AddObject(const Vec3& pos, Tile* tile, const Box2D* collider)
{
    objects_.Positions.Add(pos);
    objects_.Tiles.Add(tile);
    if (!collider)
        objects_.Colliders.Add(MakeBox2DPosSize(Vec2::ZERO(), tile->size_));
    else
        objects_.Colliders.Add(*collider);
}

//------------------------------------------------------------------------------
void Game::AddCharacter(const Vec3& pos, const AnimationState& animation, const Box2D& collider)
{
    characters_.Positions.Add(pos);
    characters_.Animations.Add(animation);
    characters_.Tiles.Add(animation.GetCurrentTile());
    characters_.Colliders.Add(collider);
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
static constexpr Color COLLIDER_COLOR = Color(0, 1, 0, 1);

//------------------------------------------------------------------------------
void Game::DrawColliders()
{
    g_Render->GetDebugShapeRenderer()->ClearShapes();

    if (!visualizeColliders_)
        return;

    auto DrawCollider = [](const Box2D& collider)
    {
        Vec3 verts[5];
        verts[0] = verts[4] = Vec3(collider.min_.x, collider.min_.y, 0);
        verts[1] = Vec3(collider.max_.x, collider.min_.y, 0);
        verts[2] = Vec3(collider.max_.x, collider.max_.y, 0);
        verts[3] = Vec3(collider.min_.x, collider.max_.y, 0);

        g_Render->GetDebugShapeRenderer()->AddShape(MakeSpan(verts), COLLIDER_COLOR);
    };

    for (int i = 0; i < characters_.Colliders.Count(); ++i)
    {
        DrawCollider(characters_.Colliders[i].Offset(characters_.Positions[i].XY()));
    }

    for (int i = 0; i < ground_.Colliders.Count(); ++i)
    {
        DrawCollider(ground_.Colliders[i]);
    }

    for (int i = 0; i < objects_.Colliders.Count(); ++i)
    {
        DrawCollider(objects_.Colliders[i].Offset(objects_.Positions[i].XY()));
    }
}

//------------------------------------------------------------------------------
Vec3 TilePos(float x, float y, float z = 0)
{
    return Vec3{ (float)x * TILE_SIZE, (float)y * TILE_SIZE, (float)z };
}

//------------------------------------------------------------------------------
RESULT MakeSimpleTile(const char* texPath, Tile& t)
{
    Texture* tex;
    if (HS_FAILED(g_ResourceManager->LoadTexture2D(texPath, &tex)))
        return R_FAIL;

    t.size_ = Vec2(tex->GetWidth(), tex->GetHeight());
    t.uvBox_ = Vec4{ 0, 0, 1, 1 };
    t.texture_ = tex;

    return R_OK;
}

//------------------------------------------------------------------------------
RESULT Game::InitWin32()
{
    Texture* groundTileTex;
    if (HS_FAILED(g_ResourceManager->LoadTexture2D("textures/Ground1.png", &groundTileTex)))
        return R_FAIL;

    constexpr float uvSize = 16.0f / (3 * 16.0f);
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 3; ++x)
        {
            Tile t{};

            t.texture_ = groundTileTex;
            t.uvBox_ = Vec4{ uvSize * x, uvSize * y, uvSize, uvSize };
            t.size_ = Vec2{ 16, 16 };

            groundTile_[3 * y + x] = t;
        }
    }

    if (HS_FAILED(MakeSimpleTile("textures/GoldChest.png", goldChestTile_)))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleTile("textures/Forest.png", forestTile_)))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleTile("textures/ForestDoor.png", forestDoorTile_)))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleTile("textures/Rock1.png", rockTile_[0])))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleTile("textures/Rock2.png", rockTile_[1])))
        return R_FAIL;

    Array<AnimationSegment> rockIdleSegments;
    for (uint i = 0; i < hs_arr_len(rockTile_); ++i)
        rockIdleSegments.Add(AnimationSegment{ &rockTile_[i], 0.5f });

    AnimationState rockIdle{};
    rockIdle.Init(rockIdleSegments);


    // ------------------------
    // Create initial map state
    Box2D chestCollider(Vec2(3, 1), Vec2(29, 25));
    AddObject(TilePos(0, 0.5f, 1), &goldChestTile_, &chestCollider);

    int left = -6;
    int width = 15;
    int height = 10;

    AddTile(Vec3(left * TILE_SIZE, 0.4f, 3), &forestTile_);

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

    Box2D rockCollider = MakeBox2DPosSize(Vec2(6, 1), Vec2(18, 29));
    AddCharacter(Vec3(-2 * TILE_SIZE, 0.5f * TILE_SIZE + 50, 1), rockIdle, rockCollider);

    Box2D groundCollider(Vec2(left * TILE_SIZE, -10 * TILE_SIZE), Vec2((left + width) * TILE_SIZE, 9));
    ground_.Colliders.Add(groundCollider);
    ground_.Tags.Add(ColliderTag::Ground);

    return R_OK;
}

bool isGrounded = false;
float height = 32;
float timeToJump = 0.3f;
float jumpVelocity = (2 * height) / (timeToJump);
float gravity = (-2 * height) / Sqr(timeToJump);
Vec2 velocity = Vec2::ZERO();
float groundLevel = 8;

//------------------------------------------------------------------------------
Vec2 ClosestNormal(const Box2D& a, const Box2D& b)
{
    float minDist = FLT_MAX;
    Vec2 normal{};
    float dist = abs(a.max_.x - b.min_.x);
    if (dist < minDist)
    {
        minDist = dist;
        normal = Vec2::RIGHT();
    }

    dist = abs(a.min_.x - b.max_.x);
    if (dist < minDist)
    {
        minDist = dist;
        normal = -Vec2::RIGHT();
    }

    dist = abs(a.max_.y - b.min_.y);
    if (dist < minDist)
    {
        minDist = dist;
        normal = Vec2::UP();
    }

    dist = abs(a.min_.y - b.max_.y);
    if (dist < minDist)
    {
        minDist = dist;
        normal = -Vec2::UP();
    }

    return normal;
}

//------------------------------------------------------------------------------
void Game::Update(float dTime)
{
    dTime_ = dTime;

    // Debug
    if (g_Input->IsKeyDown('C'))
    {
        visualizeColliders_ = !visualizeColliders_;
    }

    // Movement
    {
        velocity.y += gravity * GetDTime();
        velocity.x = 0;

        float speed{ 60 };
        if (isGrounded && g_Input->IsKeyDown('W'))
        {
            velocity.y = jumpVelocity;
        }

        if (g_Input->GetState('D'))
        {
            velocity.x += speed;
        }
        else if (g_Input->GetState('A'))
        {
            velocity.x -= speed;
        }

        Vec3& pos = characters_.Positions[0];

        isGrounded = false;

        Vec2 dtVel = velocity * GetDTime();

        Vec2 pos2 = pos.XY();
        const Box2D& originalCollider = characters_.Colliders[0];
        Box2D playerCollider = characters_.Colliders[0].Offset(pos.XY());

        auto SolveIntersection = [pos2, &dtVel, &originalCollider, &playerCollider](const Box2D& groundCollider)
        {
            IntersectionResult intersection = IsIntersecting(groundCollider, playerCollider, dtVel);
            if (intersection.isIntersecting_)
            {
                Vec2 tmpPos = pos2 + dtVel * intersection.tFirst_;
                Box2D tmpCollider = originalCollider.Offset(tmpPos);
                Vec2 closeNormal = ClosestNormal(groundCollider, tmpCollider);

                float d = closeNormal.Dot(dtVel);
                if (d < 0)
                {
                    Vec2 dir = d * closeNormal;
                    // TODO(pavel): Can we get rid of the epsilon here?
                    // It's here to avoid standing next to a block and getting on top of it just by jumping straight up.
                    // Then, since the sides align perfectly we collide and stand on top of it.
                    // TODO(pavel): Also solve why does the character sink in to things a little bit sometimes
                    dtVel -= (1 - (intersection.tFirst_ - 0.0001f)) * dir;
                    if (closeNormal == Vec2::UP())
                    {
                        isGrounded = true;
                    }
                }
            }
        };

        for (int i = 0; i < ground_.Colliders.Count(); ++i)
        {
            SolveIntersection(ground_.Colliders[i]);
        }

        for (int i = 0; i < objects_.Colliders.Count(); ++i)
        {
            SolveIntersection(objects_.Colliders[i].Offset(objects_.Positions[i].XY()));
        }


        if (isGrounded)
        {
            velocity.y = 0;
        }

        pos.x += dtVel.x;
        pos.y += dtVel.y;

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

    for (int i = 0; i < objects_.Positions.Count(); ++i)
    {
        tr->AddTile(objects_.Tiles[i], objects_.Positions[i]);
    }

    // Characters
    for (int i = 0; i < characters_.Tiles.Count(); ++i)
    {
        tr->AddTile(characters_.Tiles[i], characters_.Positions[i]);
    }

    DrawColliders();
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

