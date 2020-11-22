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

#include "Engine.h"

#include "common/Logging.h"
#include "common/hs_Assert.h"

#include "imgui/imgui.h"

namespace hs
{

//------------------------------------------------------------------------------
// Components
//------------------------------------------------------------------------------
struct Position : Vec3
{
};

//------------------------------------------------------------------------------
struct Velocity : Vec2
{
};

//------------------------------------------------------------------------------
struct Rotation
{
    float angle_;
};

//------------------------------------------------------------------------------
struct SpriteComponent
{
    Sprite* sprite_;
};

//------------------------------------------------------------------------------
struct ColliderComponent
{
    Box2D collider_;
};

//------------------------------------------------------------------------------
struct TipCollider
{
    Circle collider_;
};

//------------------------------------------------------------------------------
struct TargetCollider
{
    Circle collider_;
};

//------------------------------------------------------------------------------
struct AnimationState
{
    RESULT Init(const Array<AnimationSegment>& segments);
    void Update(float dTime);
    Sprite* GetCurrentSprite() const;

    Array<AnimationSegment> segments_;
    uint currentSegment_{};
    float timeToSwap_;
};

//------------------------------------------------------------------------------
enum class ColliderTag
{
    None,
    Ground
};

//------------------------------------------------------------------------------
struct CharacterTag
{
};

//------------------------------------------------------------------------------
void Game::InitEcs()
{
    #define INIT_COMPONENT(type) TypeInfo<type>::InitTypeId()

    INIT_COMPONENT(Entity_t);
    INIT_COMPONENT(Position);
    INIT_COMPONENT(Velocity);
    INIT_COMPONENT(Rotation);
    INIT_COMPONENT(SpriteComponent);
    INIT_COMPONENT(ColliderComponent);
    INIT_COMPONENT(TipCollider);
    INIT_COMPONENT(TargetCollider);
    INIT_COMPONENT(AnimationState);
    INIT_COMPONENT(ColliderTag);
    INIT_COMPONENT(CharacterTag);

    #undef INIT_COMPONENT

    world_ = MakeUnique<EcsWorld>();
}

//------------------------------------------------------------------------------
// Game
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
    hs_assert(!g_GameBase);
    hs_assert(!g_Game);

    g_Game = new Game();
    g_GameBase = g_Game;

    return R_OK;
}

//------------------------------------------------------------------------------
void DestroyGame()
{
    delete g_Game;

    g_Game = nullptr;
    g_GameBase = nullptr;
}

//------------------------------------------------------------------------------
Game::~Game() = default;

//------------------------------------------------------------------------------
void Game::AddSprite(const Vec3& pos, Sprite* sprite)
{
     world_->CreateEntity(Position{ pos }, SpriteComponent{ sprite });
}

//------------------------------------------------------------------------------
void Game::AddObject(const Vec3& pos, const AnimationState& animation, const Box2D* collider)
{
    auto sprite = animation.GetCurrentSprite();

    Box2D col;
    if (!collider)
        col = MakeBox2DPosSize(Vec2::ZERO(), sprite->size_);
    else
        col = *collider;

    world_->CreateEntity(Position{ pos }, animation, SpriteComponent{ sprite }, ColliderComponent{ col });
}

//------------------------------------------------------------------------------
Entity_t Game::AddCharacter(const Vec3& pos, const AnimationState& animation, const Box2D& collider)
{
    auto eid = world_->CreateEntity(
        Position{ pos },
        Velocity{ Vec2::ZERO() },
        animation,
        ColliderComponent{ collider },
        SpriteComponent{ animation.GetCurrentSprite() },
        CharacterTag{}
    );
    return eid;
}

//------------------------------------------------------------------------------
void Game::AddProjectile(const Vec3& pos, float rotation, Sprite* sprite, const Circle& collider, Vec2 velocity)
{
    world_->CreateEntity(
        Position{ pos },
        Rotation{ rotation },
        SpriteComponent{ sprite },
        TipCollider{ collider },
        Velocity{ velocity }
    );
}

//------------------------------------------------------------------------------
void Game::RemoveProjectile(Entity_t eid)
{
    world_->DeleteEntity(eid);
}

//------------------------------------------------------------------------------
void Game::AddTarget(const Vec3& pos, Sprite* sprite,  const Circle& collider)
{
    world_->CreateEntity(
        Position{ pos },
        SpriteComponent{ sprite },
        TargetCollider{ collider }
    );
}

//------------------------------------------------------------------------------
void Game::RemoveTarget(Entity_t eid)
{
    // TODO
    //world_->DeleteEntity(eid);
}

//------------------------------------------------------------------------------
void Game::AnimateSprites()
{
    EcsWorld::Iter<AnimationState, SpriteComponent>(world_.Get()).Each(
        [dTime = GetDTime()]
        (AnimationState& anim, SpriteComponent& sprite)
        {
            anim.Update(dTime);
            sprite.sprite_ = anim.GetCurrentSprite();
        }
    );
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

    EcsWorld::Iter<const Position, const ColliderComponent>(world_.Get()).Each(
        []
        (const Position& pos, const ColliderComponent& collider)
        {
            DrawCollider(collider.collider_.Offset(pos.XY()));
        }
    );

    EcsWorld::Iter<const ColliderComponent>(world_.Get()).Each(
        []
        (const ColliderComponent& collider)
        {
            DrawCollider(collider.collider_);
        }
    );

    EcsWorld::Iter<const Position, const Rotation, const TipCollider, const SpriteComponent>(world_.Get()).Each(
        []
        (const Position& pos, const Rotation rotation, const TipCollider& collider, const SpriteComponent sprite)
        {
            DrawCollider(collider.collider_, MakeTransform(pos, rotation.angle_, sprite.sprite_->pivot_));
        }
    );

    EcsWorld::Iter<const Position, const TargetCollider>(world_.Get()).Each(
        []
        (const Position& pos, const TargetCollider& collider)
        {
            DrawCollider(collider.collider_.Offset(pos.XY()), Mat44::Identity());
        }
    );
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
void Game::InitCamera()
{
    constexpr uint PIXEL_PER_TEXEL = 5;
    // Divide by 2 to account for width going from -1 to 1 in NDC, then we rely on 1 texel being one unit when we draw meshes
    g_Render->GetCamera().SetHorizontalExtent(g_Render->GetWidth() / 2 / PIXEL_PER_TEXEL);
}

//------------------------------------------------------------------------------
RESULT Game::InitWin32()
{
    InitEcs();
    InitCamera();

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

    if (HS_FAILED(MakeSimpleSprite("textures/Pumpkin1.png", pumpkinSprite_[0], Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Pumpkin2.png", pumpkinSprite_[1], Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Arrow.png", arrowSprite_, Vec2(0.5f, 0.5f))))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Target.png", targetSprite_, Vec2::ZERO())))
        return R_FAIL;

    //
    Array<AnimationSegment> rockIdleSegments;
    for (uint i = 0; i < hs_arr_len(rockSprite_); ++i)
        rockIdleSegments.Add(AnimationSegment{ &rockSprite_[i], 0.5f });

    AnimationState rockIdle{};
    if (HS_FAILED(rockIdle.Init(rockIdleSegments)))
        return R_FAIL;

    // ------------------------
    // Create initial map state
    Array<AnimationSegment> pumpkinIdleSegments;
    for (uint i = 0; i < hs_arr_len(pumpkinSprite_); ++i)
        pumpkinIdleSegments.Add(AnimationSegment{ &pumpkinSprite_[i], 0.5f });

    AnimationState pumpkinIdle{};
    if (HS_FAILED(pumpkinIdle.Init(pumpkinIdleSegments)))
        return R_FAIL;
    Box2D pumpkinCollider = MakeBox2DPosSize(Vec2(2, 0), Vec2(12, 10));
    AddObject(Vec3(3 * TILE_SIZE, 9, 1), pumpkinIdle, &pumpkinCollider);
    AddObject(Vec3(1 * TILE_SIZE, 1.5 * TILE_SIZE + 9, 1), pumpkinIdle, &pumpkinCollider);

    //
    Array<AnimationSegment> chestIdleSegments;
    chestIdleSegments.Add(AnimationSegment{ &goldChestSprite_, 1.0f });

    AnimationState chestIdle{};
    if (HS_FAILED(chestIdle.Init(chestIdleSegments)))
        return R_FAIL;

    Box2D chestCollider = MakeBox2DMinMax(Vec2(3, 1), Vec2(29, 25));
    AddObject(TilePos(0, 0.5f, 1), chestIdle, &chestCollider);

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
    character_ = AddCharacter(Vec3(-2 * TILE_SIZE, 0.5f * TILE_SIZE + 50, 1), rockIdle, rockCollider);

    Box2D groundCollider = MakeBox2DMinMax(Vec2((left + 0.5) * TILE_SIZE, -10 * TILE_SIZE), Vec2((left + width + 2) * TILE_SIZE, 9));
    world_->CreateEntity(ColliderComponent{ groundCollider }, Position{ Vec3::ZERO() }, ColliderTag::Ground);

    return R_OK;
}

//------------------------------------------------------------------------------
float Game::GetDTime()
{
    return g_Engine->GetDTime();
}

//------------------------------------------------------------------------------
RESULT Game::OnWindowResized()
{
    InitCamera();
    return R_OK;
}

//------------------------------------------------------------------------------
bool isGrounded = false;
float height = 32;
float timeToJump = 0.3f;
float jumpVelocity = (2 * height) / (timeToJump);
float gravity = (-2 * height) / Sqr(timeToJump);
float groundLevel = 8;

//------------------------------------------------------------------------------
constexpr float SHOOT_COOLDOWN = 0.5f;
float timeToShoot{};
float projectileGravity = gravity / 10;
float projectileSpeed = 150.0f;

//------------------------------------------------------------------------------
//constexpr float TARGET_COOLDOWN = 3.0f;
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
void Game::Update()
{
    // Debug
    if (g_Input->IsKeyDown('C'))
    {
        visualizeColliders_ = !visualizeColliders_;
    }

    // Movement
    float focusMultiplier = 1.0f;
    {
        if (g_Input->GetState(VK_LSHIFT))
        {
            focusMultiplier = 0.25f;
        }

        Vec2& velocity = world_->GetComponent<Velocity>(character_);
        velocity.y += gravity * g_Engine->GetDTime() * focusMultiplier;
        velocity.x = 0;

        float characterSpeed{ 80 };
        if (isGrounded && g_Input->IsKeyDown(VK_SPACE))
        {
            velocity.y = jumpVelocity;
        }

        if (g_Input->GetState('D'))
        {
            velocity.x += characterSpeed;
        }
        else if (g_Input->GetState('A'))
        {
            velocity.x -= characterSpeed;
        }

        Vec3& pos = world_->GetComponent<Position>(character_);

        isGrounded = false;

        Vec2 dtVel = velocity * g_Engine->GetDTime() * focusMultiplier;

        Vec2 pos2 = pos.XY();
        const ColliderComponent& originalCollider = world_->GetComponent<ColliderComponent>(character_);
        Box2D playerCollider = originalCollider.collider_.Offset(pos.XY());

        auto SolveIntersection = [pos2, &dtVel, &originalCollider, &playerCollider](const Box2D& groundCollider)
        {
            IntersectionResult intersection = IsIntersecting(groundCollider, playerCollider, dtVel);
            if (intersection.isIntersecting_)
            {
                Vec2 tmpPos = pos2 + dtVel * intersection.tFirst_;
                Box2D tmpCollider = originalCollider.collider_.Offset(tmpPos);
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

        EcsWorld::Iter<const ColliderComponent, const Position>(world_.Get()).EachExcept<CharacterTag>(
            [SolveIntersection]
            (const ColliderComponent& collider, const Position& pos)
            {
                SolveIntersection(collider.collider_.Offset(pos.XY()));
            }
        );

        if (isGrounded)
        {
            velocity.y = 0;
        }

        pos.x += dtVel.x;
        pos.y += dtVel.y;

        Camera& cam = g_Render->GetCamera();
        cam.SetPosition(Vec2{ pos.x, pos.y } + (world_->GetComponent<SpriteComponent>(character_).sprite_->size_ / 2.0f)); // Center the camera pos at the center of the player
        cam.UpdateMatrics();

        static Vec2 maxPlayerVelocity(Vec2::ZERO());
        static Vec2 minPlayerVelocity(Vec2::ZERO());
        maxPlayerVelocity.x = Max(maxPlayerVelocity.x, velocity.x);
        maxPlayerVelocity.y = Max(maxPlayerVelocity.y, velocity.y);
        minPlayerVelocity.x = Min(minPlayerVelocity.x, velocity.x);
        minPlayerVelocity.y = Min(minPlayerVelocity.y, velocity.y);

        ImGui::Text("Cur player velocity: [%.2f, %.2f]", velocity.x, velocity.y);
        ImGui::Text("Max player velocity: [%.2f, %.2f]", maxPlayerVelocity.x, maxPlayerVelocity.y);
        ImGui::Text("Min player velocity: [%.2f, %.2f]", minPlayerVelocity.x, minPlayerVelocity.y);
    }

    // Shooting
    {
        ImGui::Begin("Settings");
            ImGui::SliderFloat("Projectile speed", &projectileSpeed, 0.0f, 500.0f);
        ImGui::End();

        timeToShoot = Max(timeToShoot - GetDTime(), 0.0f);
        if (g_Input->IsButtonDown(BTN_LEFT) && timeToShoot <= 0)
        {
            timeToShoot = SHOOT_COOLDOWN;

            Vec2 to = CursorToWorld();
            Vec2 projPos = world_->GetComponent<Position>(character_).XY() + world_->GetComponent<SpriteComponent>(character_).sprite_->size_ / 2;
            Vec2 dir(to - projPos);
            dir.Normalize();

            float angle = RotationFromDirection(dir);

            constexpr float PLAYER_VELOCITY_WEIGHT = 0.7f;
            Vec2 projectileVelocity = dir * projectileSpeed + world_->GetComponent<Velocity>(character_) * PLAYER_VELOCITY_WEIGHT * focusMultiplier;
            AddProjectile(
                Vec3(projPos.x, projPos.y, 0.5f),
                angle,
                &arrowSprite_,
                Circle(Vec2(7, 2.5f), 2.5f),
                projectileVelocity
            );
        }

        // Move projectiles
        {
            EcsWorld::Iter<const Entity_t, Position, Velocity, Rotation>(world_.Get()).Each(
                [this](Entity_t eid, Position& position, Velocity& velocity, Rotation& rotation)
                {
                    velocity.y += projectileGravity * g_Engine->GetDTime();
                    position.AddXY(velocity * g_Engine->GetDTime());

                    rotation.angle_ = RotationFromDirection(velocity.Normalized());

                    ImGui::Text("Projectile velocity: [%.2f, %.2f]", velocity.x, velocity.y);

                    if (position.y < -1000)
                        RemoveProjectile(eid);
                }
            );
        }

        // Collide projectiles
        {
            EcsWorld::Iter<Position, Velocity, Rotation>(world_.Get()).Each(
                [](Position&, Velocity&, Rotation&)
                {
                    // TODO
                    //Mat44 projectileTransform = MakeTransform(projectiles_.Positions[i], projectiles_.Rotations[i], projectiles_.Sprites[i]->pivot_);
                    //Vec2 projPos = projectileTransform.TransformPos(projectiles_.TipColliders[i].center_);
                    //
                    //for (int j = 0; j < targets_.Positions.Count(); ++j)
                    //{
                    //    Vec2 tgtPos = targets_.Positions[j].XY() + targets_.Colliders[j].center_;
                    //    if (IsIntersecting(Circle(projPos, projectiles_.TipColliders[i].radius_), Circle(tgtPos, targets_.Colliders[j].radius_)))
                    //    {
                    //        //RemoveProjectile(i); // TODO
                    //        //RemoveTarget(j); // TODO
                    //        targetTimeToSpawn = TARGET_COOLDOWN;
                    //        break;
                    //    }
                    //}
                }
            );
        }
    }

    // Spawn target
    {
        //targetTimeToSpawn -= GetDTime();
        //if (targets_.Positions.IsEmpty() && targetTimeToSpawn <= 0)
        //{
        //    AddTarget(TilePos(5, 5, 2.5f), &targetSprite_, Circle(targetSprite_.size_ / 2.0f, 8));
        //}
    }

    AnimateSprites();

    // Draw calls
    SpriteRenderer* sr = g_Render->GetSpriteRenderer();

    sr->ClearSprites();

    // Regular tiles
    EcsWorld::Iter<const SpriteComponent, const Position>(world_.Get()).EachExcept<Rotation>(
        [sr](const SpriteComponent sprite, const Position& position)
        {
            sr->AddSprite(sprite.sprite_, Mat44::Translation(position));
        }
    );

    // Projectiles

    EcsWorld::Iter<const SpriteComponent, const Position, const Rotation>(world_.Get()).Each(
        [sr](const SpriteComponent sprite, const Position& position, const Rotation rotation)
        {
            sr->AddSprite(
                sprite.sprite_,
                // TODO(pavel): With ECS we should calculate the transform once during update and save it to entity and just read it here and in collider drawing, physics etc.
                MakeTransform(
                    position,
                    rotation.angle_,
                    sprite.sprite_->pivot_
                )
            );
        }
    );

    DrawColliders();
}

}

