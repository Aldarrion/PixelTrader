#include "game/Game.h"

#include "game/SpriteRenderer.h"
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
Sprite* AnimationState::GetCurrentSprite() const
{
    hs_assert(currentSegment_ < segments_.Count());
    return segments_[currentSegment_].sprite_;
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
void Game::AddSprite(const Vec3& pos, Sprite* sprite)
{
    sprites_.Positions.Add(pos);
    sprites_.Sprites.Add(sprite);
}

//------------------------------------------------------------------------------
void Game::AddObject(const Vec3& pos, Sprite* sprite, const Box2D* collider)
{
    objects_.Positions.Add(pos);
    objects_.Sprites.Add(sprite);
    if (!collider)
        objects_.Colliders.Add(MakeBox2DPosSize(Vec2::ZERO(), sprite->size_));
    else
        objects_.Colliders.Add(*collider);
}

//------------------------------------------------------------------------------
void Game::AddCharacter(const Vec3& pos, const AnimationState& animation, const Box2D& collider)
{
    characters_.Positions.Add(pos);
    characters_.Animations.Add(animation);
    characters_.Sprites.Add(animation.GetCurrentSprite());
    characters_.Colliders.Add(collider);
}

//------------------------------------------------------------------------------
void Game::AddProjectile(const Vec3& pos, float rotation, Sprite* sprite, const Circle& collider, Vec2 velocity)
{
    projectiles_.Positions.Add(pos);
    projectiles_.Rotations.Add(rotation);
    projectiles_.Sprites.Add(sprite);
    projectiles_.TipColliders.Add(collider);
    projectiles_.Velocities.Add(velocity);
}

//------------------------------------------------------------------------------
void Game::RemoveProjectile(uint idx)
{
    projectiles_.Positions.Remove(idx);
    projectiles_.Rotations.Remove(idx);
    projectiles_.Sprites.Remove(idx);
    projectiles_.TipColliders.Remove(idx);
    projectiles_.Velocities.Remove(idx);
}

//------------------------------------------------------------------------------
void Game::AddTarget(const Vec3& pos, Sprite* sprite,  const Circle& collider)
{
    targets_.Positions.Add(pos);
    targets_.Colliders.Add(collider);
    targets_.Sprites.Add(sprite);
}

//------------------------------------------------------------------------------
void Game::RemoveTarget(uint idx)
{
    targets_.Positions.Remove(idx);
    targets_.Colliders.Remove(idx);
    targets_.Sprites.Remove(idx);
}

//------------------------------------------------------------------------------
void Game::AnimateSprites()
{
    for (int i = 0; i < characters_.Animations.Count(); ++i)
    {
        characters_.Animations[i].Update(dTime_);
        characters_.Sprites[i] = characters_.Animations[i].GetCurrentSprite();
    }
}

//------------------------------------------------------------------------------
static constexpr Color COLLIDER_COLOR = Color(0, 1, 0, 1);

//------------------------------------------------------------------------------
static void DrawCollider(const Box2D& collider)
{
    Vec3 verts[5];
    verts[0] = verts[4] = Vec3(collider.min_.x, collider.min_.y, 0);
    verts[1] = Vec3(collider.max_.x, collider.min_.y, 0);
    verts[2] = Vec3(collider.max_.x, collider.max_.y, 0);
    verts[3] = Vec3(collider.min_.x, collider.max_.y, 0);

    g_Render->GetDebugShapeRenderer()->AddShape(MakeSpan(verts), COLLIDER_COLOR);
}

//------------------------------------------------------------------------------
static void DrawCollider(const Circle& collider, const Mat44& world)
{
    constexpr uint VERT_COUNT = 32;
    Vec3 verts[VERT_COUNT];
    constexpr float step = HS_TAU / (VERT_COUNT - 1);

    for (int i = 0; i < VERT_COUNT; ++i)
    {
        float x = cosf(step * i) * collider.radius_;
        float y = sinf(step * i) * collider.radius_;
        verts[i] = Vec3(collider.center_.x + x, collider.center_.y + y, 0);
        verts[i] = world.TransformPos(verts[i]);
    }

    g_Render->GetDebugShapeRenderer()->AddShape(MakeSpan(verts), COLLIDER_COLOR);
}

//------------------------------------------------------------------------------
void Game::DrawColliders()
{
    g_Render->GetDebugShapeRenderer()->ClearShapes();

    if (!visualizeColliders_)
        return;

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

    for (int i = 0; i < projectiles_.TipColliders.Count(); ++i)
    {
        DrawCollider(projectiles_.TipColliders[i], MakeTransform(projectiles_.Positions[i], projectiles_.Rotations[i], projectiles_.Sprites[i]->pivot_));
    }

    for (int i = 0; i < targets_.Colliders.Count(); ++i)
    {
        DrawCollider(targets_.Colliders[i].Offset(targets_.Positions[i].XY()), Mat44::Identity());
    }
}

//------------------------------------------------------------------------------
static Vec3 TilePos(float x, float y, float z = 0)
{
    return Vec3{ (float)x * TILE_SIZE, (float)y * TILE_SIZE, (float)z };
}

//------------------------------------------------------------------------------
static RESULT MakeSimpleSprite(const char* texPath, Sprite& t, Vec2 pivot)
{
    Texture* tex;
    if (HS_FAILED(g_ResourceManager->LoadTexture2D(texPath, &tex)))
        return R_FAIL;

    t.size_ = Vec2(tex->GetWidth(), tex->GetHeight());
    t.uvBox_ = Vec4{ 0, 0, 1, 1 };
    t.texture_ = tex;
    t.pivot_ = Vec2(t.size_.x * pivot.x, t.size_.y * pivot.y);

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
            Sprite t{};

            t.texture_ = groundTileTex;
            t.uvBox_ = Vec4{ uvSize * x, uvSize * y, uvSize, uvSize };
            t.size_ = Vec2{ 16, 16 };

            groundSprite_[3 * y + x] = t;
        }
    }

    if (HS_FAILED(MakeSimpleSprite("textures/GoldChest.png", goldChestSprite_, Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Forest.png", forestSprite_, Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/ForestDoor.png", forestDoorSprite_, Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Rock1.png", rockSprite_[0], Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Rock2.png", rockSprite_[1], Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Arrow.png", arrowSprite_, Vec2(0.5f, 0.5f))))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Target.png", targetSprite_, Vec2::ZERO())))
        return R_FAIL;

    Array<AnimationSegment> rockIdleSegments;
    for (uint i = 0; i < hs_arr_len(rockSprite_); ++i)
        rockIdleSegments.Add(AnimationSegment{ &rockSprite_[i], 0.5f });

    AnimationState rockIdle{};
    rockIdle.Init(rockIdleSegments);


    // ------------------------
    // Create initial map state
    Box2D chestCollider = MakeBox2DMinMax(Vec2(3, 1), Vec2(29, 25));
    AddObject(TilePos(0, 0.5f, 1), &goldChestSprite_, &chestCollider);

    int left = -6;
    int width = 15;
    int height = 10;

    AddSprite(Vec3(left * TILE_SIZE, 0.4f, 3), &forestSprite_);

    AddSprite(TilePos(left + 2, 0.3f, 2), &forestDoorSprite_);

    int y = 0;
    AddSprite(TilePos(left, y), &groundSprite_[TOP_LEFT]);
    for (int i = 0; i < width; ++i)
        AddSprite(TilePos(left + 1 + i, y), &groundSprite_[TOP]);
    AddSprite(TilePos(left + width + 1, y), &groundSprite_[TOP_RIGHT]);
    
    int j = 0;
    for (; j < height; j++)
    {
        y = -1 - j;
        AddSprite(TilePos(left, y), &groundSprite_[MID_LEFT]);
        for (int i = 0; i < width; ++i)
            AddSprite(TilePos(left + 1 + i, y), &groundSprite_[MID]);
        AddSprite(TilePos(left + width + 1, y), &groundSprite_[MID_RIGHT]);
    }

    y = -j - 1;
    AddSprite(TilePos(left, y), &groundSprite_[BOT_LEFT]);
    for (int i = 0; i < width; ++i)
        AddSprite(TilePos(left + 1 + i, y), &groundSprite_[BOT]);
    AddSprite(TilePos(left + width + 1, y), &groundSprite_[BOT_RIGHT]);

    Box2D rockCollider = MakeBox2DPosSize(Vec2(6, 1), Vec2(18, 29));
    AddCharacter(Vec3(-2 * TILE_SIZE, 0.5f * TILE_SIZE + 50, 1), rockIdle, rockCollider);

    Box2D groundCollider = MakeBox2DMinMax(Vec2(left * TILE_SIZE, -10 * TILE_SIZE), Vec2((left + width) * TILE_SIZE, 9));
    ground_.Colliders.Add(groundCollider);
    ground_.Tags.Add(ColliderTag::Ground);

    return R_OK;
}

//------------------------------------------------------------------------------
bool isGrounded = false;
float height = 32;
float timeToJump = 0.3f;
float jumpVelocity = (2 * height) / (timeToJump);
float gravity = (-2 * height) / Sqr(timeToJump);
Vec2 velocity = Vec2::ZERO();
float groundLevel = 8;

//------------------------------------------------------------------------------
constexpr float SHOOT_COOLDOWN = 0.5f;
float timeToShoot{};
float projectileGravity = gravity / 10;
float projectileSpeed = 150.0f;

//------------------------------------------------------------------------------
constexpr float TARGET_COOLDOWN = 3.0f;
float targetTimeToSpawn{};

//------------------------------------------------------------------------------
static Vec2 ClosestNormal(const Box2D& a, const Box2D& b)
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
// TODO(pavel): move to camera? or input?
static Vec2 CursorToWorld()
{
    const Camera& cam = g_Render->GetCamera();
    Mat44 worldToProj = cam.ToCamera() * cam.ToProjection();
    Mat44 projToWorld = worldToProj.GetInverseTransform();

    Vec2 wndSize = g_Render->GetDimensions();
    Vec2 mousePos = g_Input->GetMousePos();
    Vec4 ndcMouse((mousePos.x / wndSize.x) * 2 - 1, (1 - (mousePos.y / wndSize.y)) * 2 - 1, 0, 1);

    Vec4 worldMouse = ndcMouse * projToWorld;

    return Vec2(worldMouse.x, worldMouse.y);
}

//------------------------------------------------------------------------------
static float RotationFromDirection(Vec2 dirNormalized)
{
    float dotX = dirNormalized.Dot(Vec2::RIGHT());
    float dotY = dirNormalized.Dot(Vec2::UP());
    float angle = acosf(dotX);
    if (dotY < 0)
        angle = HS_TAU - angle;

    return angle;
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
        cam.SetPosition(Vec2{ pos.x, pos.y } + (characters_.Sprites[0]->size_ / 2.0f)); // Center the camera pos at the center of the player
        cam.UpdateMatrics();
    }

    // Shooting
    {
        timeToShoot = Max(timeToShoot - dTime, 0.0f);
        if (g_Input->IsButtonDown(BTN_LEFT) && timeToShoot <= 0)
        {
            timeToShoot = SHOOT_COOLDOWN;

            Vec2 to = CursorToWorld();
            Vec2 projPos = characters_.Positions[0].XY() + characters_.Sprites[0]->size_ / 2;
            Vec2 dir(to - projPos);
            dir.Normalize();

            float angle = RotationFromDirection(dir);

            AddProjectile(
                Vec3(projPos.x, projPos.y, 0.5f),
                angle,
                &arrowSprite_,
                Circle(Vec2(7, 2.5f), 2.5f),
                dir * projectileSpeed
            );
        }

        // Move projectiles
        {
            for (int i = 0; i < projectiles_.Positions.Count();)
            {
                projectiles_.Velocities[i].y += projectileGravity * GetDTime();
                projectiles_.Positions[i].AddXY(projectiles_.Velocities[i] * GetDTime());

                projectiles_.Rotations[i] = RotationFromDirection(projectiles_.Velocities[i].Normalized());

                if (projectiles_.Positions[i].y < -1000)
                    RemoveProjectile(i);
                else
                    ++i;
            }
        }

        // Collide projectiles
        {
            for (int i = 0; i < projectiles_.Positions.Count();)
            {
                Mat44 projectileTransform = MakeTransform(projectiles_.Positions[i], projectiles_.Rotations[i], projectiles_.Sprites[i]->pivot_);
                Vec2 projPos = projectileTransform.TransformPos(projectiles_.TipColliders[i].center_);

                for (int j = 0; j < targets_.Positions.Count(); ++j)
                {
                    Vec2 tgtPos = targets_.Positions[j].XY() + targets_.Colliders[j].center_;
                    if (IsIntersecting(Circle(projPos, projectiles_.TipColliders[i].radius_), Circle(tgtPos, targets_.Colliders[j].radius_)))
                    {
                        RemoveProjectile(i);
                        RemoveTarget(j);
                        targetTimeToSpawn = TARGET_COOLDOWN;
                        break;
                    }
                }

                ++i;
            }
        }
    }

    // Spawn target
    {
        targetTimeToSpawn -= GetDTime();
        if (targets_.Positions.IsEmpty() && targetTimeToSpawn <= 0)
        {
            AddTarget(TilePos(5, 5, 2.5f), &targetSprite_, Circle(targetSprite_.size_ / 2.0f, 8));
        }
    }

    AnimateSprites();

    // Draw calls
    SpriteRenderer* sr = g_Render->GetSpriteRenderer();

    sr->ClearSprites();

    // Regular tiles
    for (int i = 0; i < sprites_.Positions.Count(); ++i)
    {
        sr->AddSprite(sprites_.Sprites[i], Mat44::Translation(sprites_.Positions[i]));
    }

    for (int i = 0; i < objects_.Positions.Count(); ++i)
    {
        sr->AddSprite(objects_.Sprites[i], Mat44::Translation(objects_.Positions[i]));
    }

    // Characters
    for (int i = 0; i < characters_.Sprites.Count(); ++i)
    {
        sr->AddSprite(characters_.Sprites[i], Mat44::Translation(characters_.Positions[i]));
    }

    // Projectiles
    for (int i = 0; i < projectiles_.Sprites.Count(); ++i)
    {
        Vec2 pos = projectiles_.Positions[i].XY();
        sr->AddSprite(
            projectiles_.Sprites[i],
            // TODO(pavel): With ECS we should calculate the transform once during update and save it to entity and just read it here and in collider drawing, physics etc.
            MakeTransform(
                Vec3(pos.x, pos.y, projectiles_.Positions[i].z),
                projectiles_.Rotations[i],
                projectiles_.Sprites[i]->pivot_
            )
        );
    }

    // Targets
    for (int i = 0; i < targets_.Sprites.Count(); ++i)
    {
        sr->AddSprite(targets_.Sprites[i], Mat44::Translation(targets_.Positions[i]));
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

