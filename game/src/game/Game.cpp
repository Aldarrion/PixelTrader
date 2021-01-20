#include "Game/Game.h"

#include "Game/SpriteRenderer.h"
#include "Game/DebugShapeRenderer.h"

#include "Render/Texture.h"
#include "Render/VertexBuffer.h"
#include "Render/ShaderManager.h"
#include "Render/Render.h"
#include "Render/hs_Image.h"

#include "Resources/ResourceManager.h"

#include "Input/Input.h"

#include "Engine.h"

#include "Common/Logging.h"
#include "Common/hs_Assert.h"

#include "imgui/imgui.h"

#include <cstdio>

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
struct TargetRespawnTimer
{
    Vec3 position_;
    float timeLeft_;
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
struct SpawnPoint
{
};

//------------------------------------------------------------------------------
struct PlayerRespawnTimer
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
    INIT_COMPONENT(TargetRespawnTimer);
    INIT_COMPONENT(SpawnPoint);
    INIT_COMPONENT(PlayerRespawnTimer);

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
Entity_t Game::SpawnPlayer()
{
    Array<AnimationSegment> rockIdleSegments;
    for (uint i = 0; i < HS_ARR_LEN(rockSprite_); ++i)
        rockIdleSegments.Add(AnimationSegment{ &rockSprite_[i], 0.5f });

    AnimationState rockIdle{};
    if (HS_FAILED(rockIdle.Init(rockIdleSegments)))
    {
        hs_assert(false);
        return 0;
    }

    Box2D rockCollider = MakeBox2DPosSize(Vec2(6, 1), Vec2(18, 29));

    // Assign gamepad
    for (int gamepadI = 0; gamepadI < GLFW_JOYSTICK_LAST; ++gamepadI)
    {
        bool gamepadOk = true;
        if (g_Input->IsGamepadConnected(gamepadI))
        {
            for (int playerI = 0; playerI < playerCount_; ++playerI)
            {
                if (gamepadForPlayer_[playerI] == gamepadI)
                    gamepadOk = false;
            }
        }
        else
        {
            gamepadOk = false;
        }

        if (gamepadOk)
        {
            gamepadForPlayer_[playerCount_] = gamepadI;
            break;
        }
    }

    Array<Vec3> spawnPositions;
    EcsWorld::Iter<const Position, const SpawnPoint>(world_.Get()).Each(
        [&spawnPositions](const Position pos, const SpawnPoint)
        {
            spawnPositions.Add(pos);
        }
    );

    const auto spawnIdx = (uint)((rand() * 1.0f / RAND_MAX) * spawnPositions.Count());
    const Vec3 spawnPos = spawnPositions[spawnIdx];

    players_[playerCount_] = AddCharacter(spawnPos, rockIdle, rockCollider);
    ++playerCount_;

    return players_[playerCount_ - 1];
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
RESULT Game::LoadMap()
{
    Array<AnimationSegment> pumpkinIdleSegments;
    for (uint i = 0; i < HS_ARR_LEN(pumpkinSprite_); ++i)
        pumpkinIdleSegments.Add(AnimationSegment{ &pumpkinSprite_[i], 0.5f });
    
    AnimationState pumpkinIdle{};
    if (HS_FAILED(pumpkinIdle.Init(pumpkinIdleSegments)))
        return R_FAIL;
    Box2D pumpkinCollider = MakeBox2DPosSize(Vec2(2, 0), Vec2(12, 10));
    AddObject(Vec3(3 * TILE_SIZE, 9, 1), pumpkinIdle, &pumpkinCollider);
    AddObject(Vec3(1 * TILE_SIZE, 1.5 * TILE_SIZE + 9, 1), pumpkinIdle, &pumpkinCollider);

    Array<AnimationSegment> chestIdleSegments;
    chestIdleSegments.Add(AnimationSegment{ &goldChestSprite_, 1.0f });
    
    AnimationState chestIdle{};
    if (HS_FAILED(chestIdle.Init(chestIdleSegments)))
        return R_FAIL;

    Box2D chestCollider = MakeBox2DMinMax(Vec2(3, 1), Vec2(29, 25));
    AddObject(TilePos(0, 0.5f, 1), chestIdle, &chestCollider);

    int bot = - 1;
    int left = -12;
    int width = 22;
    int height = 15;

    AddSprite(TilePos(left, bot + 1), &groundSprite_[TOP_LEFT]);
    for (int i = 0; i < width; ++i)
        AddSprite(TilePos(left + 1 + i, bot + 1), &groundSprite_[TOP]);
    AddSprite(TilePos(left + width + 1, bot + 1), &groundSprite_[TOP_RIGHT]);

    for (int i = 0; i < height; ++i)
        AddSprite(TilePos(left, bot + i, 0.1f), &groundSprite_[MID_RIGHT]);

    for (int i = 0; i < height; ++i)
        AddSprite(TilePos(left + width + 1, bot + i, 0.1f), &groundSprite_[MID_LEFT]);

    AddSprite(TilePos(left, bot), &groundSprite_[BOT_LEFT]);
    for (int i = 0; i < width; ++i)
        AddSprite(TilePos(left + 1 + i, bot), &groundSprite_[BOT]);
    AddSprite(TilePos(left + width + 1, bot), &groundSprite_[BOT_RIGHT]);

    Box2D groundCollider = MakeBox2DMinMax(Vec2((left + 0.25f) * TILE_SIZE, -10 * TILE_SIZE), Vec2((left + width + 1.75f) * TILE_SIZE, 9));
    world_->CreateEntity(ColliderComponent{ groundCollider }, Position{ Vec3::ZERO() }, ColliderTag::Ground);

    Box2D leftWallCollider = MakeBox2DMinMax(Vec2((left) * TILE_SIZE, bot * TILE_SIZE), Vec2((left + 0.75f) * TILE_SIZE, height * TILE_SIZE));
    world_->CreateEntity(ColliderComponent{ leftWallCollider }, Position{ Vec3::ZERO() }, ColliderTag::Ground);

    Box2D rightWallCollider = MakeBox2DMinMax(Vec2((left + width + 1.25f) * TILE_SIZE, bot * TILE_SIZE), Vec2((left + width + 2) * TILE_SIZE, height * TILE_SIZE));
    world_->CreateEntity(ColliderComponent{ rightWallCollider }, Position{ Vec3::ZERO() }, ColliderTag::Ground);

    world_->CreateEntity(TargetRespawnTimer{ TilePos(5, 5, 2.5f), TARGET_COOLDOWN });
    world_->CreateEntity(TargetRespawnTimer{ TilePos(-5, 5, 2.5f), TARGET_COOLDOWN });
    world_->CreateEntity(TargetRespawnTimer{ TilePos(5, 7, 2.5f), TARGET_COOLDOWN });
    world_->CreateEntity(TargetRespawnTimer{ TilePos(-5, 7, 2.5f), TARGET_COOLDOWN });

    world_->CreateEntity(Position{ Vec3(-2 * TILE_SIZE, 0.5f * TILE_SIZE + 50, 1) }, SpawnPoint{});
    world_->CreateEntity(Position{ Vec3(2 * TILE_SIZE, 0.5f * TILE_SIZE + 50, 1) }, SpawnPoint{});

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
RESULT Game::Init()
{
    //srand(42);

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

    if (HS_FAILED(LoadMap()))
        return R_FAIL;

    SpawnPlayer();

    Camera& cam = g_Render->GetCamera();
    cam.SetPosition(Vec2{ 0, 6.5f * TILE_SIZE });
    cam.UpdateMatrices();

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
static constexpr float height = 32;
static constexpr float timeToJump = 0.3f;
static constexpr float jumpVelocity = (2 * height) / (timeToJump);
static constexpr float gravity = (-2 * height) / Sqr(timeToJump);
static constexpr float groundLevel = 8;

//------------------------------------------------------------------------------
static constexpr float projectileGravity = gravity / 10;
float projectileSpeed = 150.0f;

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
    if (g_Input->IsKeyDown(KC_C))
    {
        visualizeColliders_ = !visualizeColliders_;
    }

    // Player menu
    int newPlayerCount = playerCount_;
    ImGui::Begin("Players");
        ImGui::InputInt("Player count", &newPlayerCount);
        newPlayerCount = Clamp((uint)newPlayerCount, 1u, MAX_PLAYERS);
        
        for (int playerI = 0; playerI < playerCount_; ++playerI)
        {
            ImGui::Text("Player %d input", playerI);
            for (int gamepadI = 0; gamepadI < GLFW_JOYSTICK_LAST; ++gamepadI)
            {
                if (g_Input->IsGamepadConnected(gamepadI))
                {
                    char buff[128];
                    sprintf(buff, "P%d Gamepad %d", playerI, gamepadI);
                    ImGui::RadioButton(buff, &gamepadForPlayer_[playerI], gamepadI);
                }
            }
        }
    ImGui::End();

    while (playerCount_ < newPlayerCount)
    {
        SpawnPlayer();
    }

    ImGui::Begin("Settings");
        ImGui::SliderFloat("Projectile speed", &projectileSpeed, 0.0f, 500.0f);
    ImGui::End();

    // Movement
    float focusMultiplier[MAX_PLAYERS]{};
    for (int playerI = 0; playerI < playerCount_; ++playerI)
    {
        if (g_Input->GetState(KC_LSHIFT) || g_Input->GetAxis(gamepadForPlayer_[playerI], GLFW_GAMEPAD_AXIS_LEFT_TRIGGER) > -0.5)
            focusMultiplier[playerI] = 0.25f;
        else
            focusMultiplier[playerI] = 1.0;

        Vec2& velocity = world_->GetComponent<Velocity>(players_[playerI]);
        velocity.y += gravity * g_Engine->GetDTime() * focusMultiplier[playerI];
        velocity.x = 0;

        float characterSpeed{ 80 };
        if (isGrounded_[playerI] && (g_Input->IsKeyDown(KC_SPACE) || g_Input->IsButtonDown(gamepadForPlayer_[playerI], GLFW_GAMEPAD_BUTTON_A)))
        {
            velocity.y = jumpVelocity;
        }

        if (g_Input->GetState(KC_D))
        {
            velocity.x += characterSpeed;
        }
        else if (g_Input->GetState(KC_A))
        {
            velocity.x -= characterSpeed;
        }

        const float xAxis = g_Input->GetAxis(gamepadForPlayer_[playerI], GLFW_GAMEPAD_AXIS_LEFT_X);
        if (xAxis)
        {
            velocity.x += characterSpeed * xAxis;
        }

        Vec3& pos = world_->GetComponent<Position>(players_[playerI]);

        isGrounded_[playerI] = false;

        Vec2 dtVel = velocity * g_Engine->GetDTime() * focusMultiplier[playerI];

        Vec2 pos2 = pos.XY();
        const ColliderComponent& originalCollider = world_->GetComponent<ColliderComponent>(players_[playerI]);
        Box2D playerCollider = originalCollider.collider_.Offset(pos.XY());

        auto SolveIntersection = [this, pos2, &dtVel, &originalCollider, &playerCollider](const Box2D& groundCollider, int playerI)
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
                        isGrounded_[playerI] = true;
                    }
                }
            }
        };

        EcsWorld::Iter<const ColliderComponent, const Position>(world_.Get()).EachExcept<CharacterTag>(
            [SolveIntersection, playerI]
            (const ColliderComponent& collider, const Position& pos)
            {
                SolveIntersection(collider.collider_.Offset(pos.XY()), playerI);
            }
        );

        if (isGrounded_[playerI])
        {
            velocity.y = 0;
        }

        pos.x += dtVel.x;
        pos.y += dtVel.y;

        //Camera& cam = g_Render->GetCamera();
        //cam.SetPosition(Vec2{ pos.x, pos.y } + (world_->GetComponent<SpriteComponent>(players_[playerI]).sprite_->size_ / 2.0f)); // Center the camera pos at the center of the player
        //cam.UpdateMatrices();

        static Vec2 maxPlayerVelocity(Vec2::ZERO());
        static Vec2 minPlayerVelocity(Vec2::ZERO());
        maxPlayerVelocity.x = Max(maxPlayerVelocity.x, velocity.x);
        maxPlayerVelocity.y = Max(maxPlayerVelocity.y, velocity.y);
        minPlayerVelocity.x = Min(minPlayerVelocity.x, velocity.x);
        minPlayerVelocity.y = Min(minPlayerVelocity.y, velocity.y);

        ImGui::Text("Cur player %d velocity: [%.2f, %.2f]", playerI, velocity.x, velocity.y);
        ImGui::Text("Max player %d velocity: [%.2f, %.2f]", playerI, maxPlayerVelocity.x, maxPlayerVelocity.y);
        ImGui::Text("Min player %d velocity: [%.2f, %.2f]", playerI, minPlayerVelocity.x, minPlayerVelocity.y);

        // Shooting
        timeToShoot_[playerI] = Max(timeToShoot_[playerI] - GetDTime(), 0.0f);
        if (timeToShoot_[playerI] <= 0)
        {
            const Vec2 projPos = world_->GetComponent<Position>(players_[playerI]).XY() + world_->GetComponent<SpriteComponent>(players_[playerI]).sprite_->size_ / 2;
            Vec2 dir;
            bool shouldShoot = false;

            if (g_Input->IsButtonDown(BTN_LEFT))
            {
                shouldShoot = true;
                Vec2 to = CursorToWorld();
                dir = (to - projPos);
            }
            else if (g_Input->IsButtonDown(gamepadForPlayer_[playerI], GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER))
            {
                shouldShoot = true;
                dir.x = g_Input->GetAxis(gamepadForPlayer_[playerI], GLFW_GAMEPAD_AXIS_RIGHT_X);
                dir.y = -g_Input->GetAxis(gamepadForPlayer_[playerI], GLFW_GAMEPAD_AXIS_RIGHT_Y);

                if (dir.LengthSqr() == 0)
                    dir.x = world_->GetComponent<Velocity>(players_[playerI]).x;

                if (dir.LengthSqr() == 0)
                    dir.x = 1;
            }

            if (shouldShoot)
            {
                timeToShoot_[playerI] = SHOOT_COOLDOWN;
                dir.Normalize();

                float angle = RotationFromDirection(dir);

                constexpr float PLAYER_VELOCITY_WEIGHT = 0.7f;
                Vec2 projectileVelocity = dir * projectileSpeed + world_->GetComponent<Velocity>(players_[playerI]) * PLAYER_VELOCITY_WEIGHT * focusMultiplier[playerI];
                AddProjectile(
                    Vec3(projPos.x, projPos.y, 0.5f),
                    angle,
                    &arrowSprite_,
                    Circle(Vec2(7, 2.5f), 2.5f),
                    projectileVelocity
                );
            }
        }
    }

    {
        // Move projectiles
        {
            Array<Entity_t> projectilesToRemove;
            EcsWorld::Iter<const Entity_t, Position, Velocity, Rotation>(world_.Get()).Each(
                [this, &projectilesToRemove](Entity_t eid, Position& position, Velocity& velocity, Rotation& rotation)
                {
                    velocity.y += projectileGravity * g_Engine->GetDTime();
                    position.AddXY(velocity * g_Engine->GetDTime());

                    rotation.angle_ = RotationFromDirection(velocity.Normalized());

                    ImGui::Text("Projectile velocity: [%.2f, %.2f]", velocity.x, velocity.y);

                    if (position.y < -1000)
                        projectilesToRemove.Add(eid);
                }
            );

            for (int i = 0; i < projectilesToRemove.Count(); ++i)
                RemoveProjectile(projectilesToRemove[i]);
        }

        // Collide projectiles
        {
            Array<Entity_t> toRemove;
            Array<TargetRespawnTimer> toCreate;
            EcsWorld::Iter<const Entity_t, Position, Rotation, TipCollider, SpriteComponent>(world_.Get()).Each(
                [this, &toRemove, &toCreate](Entity_t projectile, Position& position, Rotation& rotation, TipCollider& collider, SpriteComponent& sprite)
                {
                    Mat44 projectileTransform = MakeTransform(position, rotation.angle_, sprite.sprite_->pivot_);
                    Vec2 projPos = projectileTransform.TransformPos(collider.collider_.center_);
                    bool shouldRemove = false;

                    EcsWorld::Iter<const Entity_t, Position, TargetCollider>(world_.Get()).Each(
                        [this, &shouldRemove, &toRemove, &toCreate, projPos, &tipCollider = collider.collider_](Entity_t target, Position& targetPos, TargetCollider targetCollider)
                        {
                            Vec2 tgtPos = targetPos.XY() + targetCollider.collider_.center_;
                            if (IsIntersecting(Circle(projPos, tipCollider.radius_), Circle(tgtPos, targetCollider.collider_.radius_)))
                            {
                                shouldRemove = true;
                                toRemove.AddUnique(target);
                                toCreate.Add(TargetRespawnTimer{ targetPos, TARGET_COOLDOWN });
                            }
                        }
                    );

                    if (shouldRemove)
                        toRemove.AddUnique(projectile);
                }
            );

            for (int i = 0; i < toRemove.Count(); ++i)
                world_->DeleteEntity(toRemove[i]);

            for (int i = 0; i < toCreate.Count(); ++i)
                world_->CreateEntity(toCreate[i]);
        }
    }

    // Spawn target
    {
        Array<Entity_t> timersToRemove;
        EcsWorld::Iter<const Entity_t, TargetRespawnTimer>(world_.Get()).Each(
            [this, dTime = GetDTime(), &timersToRemove](Entity_t eid, TargetRespawnTimer& timer)
            {
                timer.timeLeft_ -= dTime;
                if (timer.timeLeft_ <= 0)
                {
                    timersToRemove.Add(eid);
                    world_->CreateEntity(Position{ timer.position_ }, SpriteComponent{ &targetSprite_ }, TargetCollider{ Circle(targetSprite_.size_ / 2.0f, 8) });
                }
            }
        );

        for (int i = 0; i < timersToRemove.Count(); ++i)
            world_->DeleteEntity(timersToRemove[i]);
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

