#include "Game/Game.h"

#include "Game/SpriteRenderer.h"
#include "Game/DebugShapeRenderer.h"

#include "Gui/Font.h"
#include "Gui/GuiRenderer.h"

#include "Render/Texture.h"
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
struct Projectile
{
    int shooterId_;
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
struct PlayerComponent
{
    int playerId_;
};

//------------------------------------------------------------------------------
struct SpawnPoint
{
};

//------------------------------------------------------------------------------
struct PlayerRespawnTimer
{
    int playerEntity_;
    float timeLeft_;
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
    INIT_COMPONENT(PlayerComponent);
    INIT_COMPONENT(TargetRespawnTimer);
    INIT_COMPONENT(SpawnPoint);
    INIT_COMPONENT(PlayerRespawnTimer);
    INIT_COMPONENT(Projectile);

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
PlayerInfo Game::RespawnPlayer(int playerId)
{
    // Prepare to create entity
    Array<AnimationSegment> rockIdleSegments;
    for (uint i = 0; i < HS_ARR_LEN(rockSprite_); ++i)
        rockIdleSegments.Add(AnimationSegment{ &rockSprite_[i], 0.5f });

    AnimationState rockIdle{};
    if (HS_FAILED(rockIdle.Init(rockIdleSegments)))
    {
        hs_assert(false);
        return {};
    }

    Box2D rockCollider = MakeBox2DPosSize(Vec2(6, 1), Vec2(18, 29));

    // Spawn
    Array<Vec3> spawnPositions;
    EcsWorld::Iter<const Position, const SpawnPoint>(world_.Get()).Each(
        [&spawnPositions](const Position pos, const SpawnPoint)
        {
            spawnPositions.Add(pos);
        }
    );

    const auto spawnIdx = (uint)((rand() * 1.0f / RAND_MAX) * spawnPositions.Count());
    const Vec3 spawnPos = spawnPositions[spawnIdx];

    PlayerInfo playerInfo{};
    playerInfo.playerEntity_ = world_->CreateEntity(
        Position{ spawnPos },
        Velocity{ Vec2::ZERO() },
        rockIdle,
        ColliderComponent{ rockCollider },
        SpriteComponent{ rockIdle.GetCurrentSprite() },
        PlayerComponent{ playerId }
    );

    Vec2 weaponPosOffset(rockIdle.GetCurrentSprite()->size_ / 2.0f);
    playerInfo.weaponEntity_ = world_->CreateEntity(
        Position{ Vec3(spawnPos.x + weaponPosOffset.x, spawnPos.x + weaponPosOffset.y, LAYER_WEAPON) },
        Rotation { 0.0f },
        SpriteComponent{ &bowSprite_ }
    );

    return playerInfo;
}

//------------------------------------------------------------------------------
Entity_t Game::SpawnPlayer()
{
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

    players_[playerCount_] = RespawnPlayer(playerCount_);
    ++playerCount_;

    return players_[playerCount_ - 1].playerEntity_;
}

//------------------------------------------------------------------------------
void Game::AddProjectile(const Vec3& pos, float rotation, Sprite* sprite, const Circle& collider, Vec2 velocity, int playerId)
{
    world_->CreateEntity(
        Position{ pos },
        Rotation{ rotation },
        SpriteComponent{ sprite },
        TipCollider{ collider },
        Velocity{ velocity },
        Projectile{ playerId }
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

    auto MakePumpkin = [this, &pumpkinCollider, &pumpkinIdle](float x, float y, int offsetX, int offsetY)
    {
        AddObject(Vec3(x * TILE_SIZE + offsetX, y * TILE_SIZE + 9 + offsetY, 1), pumpkinIdle, &pumpkinCollider);
    };

    auto MakeAmanita = [this](float x, float y, int height)
    {
        AddSprite(Vec3(x * TILE_SIZE, y * TILE_SIZE + 5 + height, LAYER_CLUTTER), &amanitaSprite_);
    };

    auto MakeFlowerSmall = [this](float  tileX, float tileY, int offsetX, int height)
    {
        AddSprite(Vec3(tileX * TILE_SIZE + offsetX, tileY * TILE_SIZE + 6 + height, LAYER_CLUTTER), &flowerSmallSprite_);
    };

    auto MakeFlowerSmallCluster = [this, MakeFlowerSmall](float tileX, float tileY, int offsetX)
    {
        MakeFlowerSmall(tileX, tileY, offsetX + 0, 3);
        MakeFlowerSmall(tileX, tileY, offsetX + -4, 2);
        MakeFlowerSmall(tileX, tileY, offsetX + 4, 1);
    };

    auto MakeSunflower = [this](float tileX, float tileY, int offsetX, int offsetY)
    {
        AddSprite(Vec3(tileX * TILE_SIZE + offsetX, tileY * TILE_SIZE + 9 + offsetY, LAYER_CLUTTER), &sunflowerSprite_);
    };

    Array<AnimationSegment> crystalIdleSegments;
    crystalIdleSegments.Add(AnimationSegment{ &crystalSprite_, 0.5f });

    AnimationState crystalIdle{};
    if (HS_FAILED(crystalIdle.Init(crystalIdleSegments)))
        return R_FAIL;
    Box2D mainCrystalCollider = MakeBox2DPosSize(Vec2(2, 0), Vec2(26, 10));

    auto MakeCrystal = [this, &crystalIdle, &mainCrystalCollider](float tileX, float tileY, int offsetX, int yOffset, float zOffset)
    {
        const Vec3 pos(tileX * TILE_SIZE + offsetX, tileY * TILE_SIZE + yOffset, 1 + zOffset);
        AddObject(pos, crystalIdle, &mainCrystalCollider);

        Box2D centerCrystalCollider = MakeBox2DPosSize(Vec2(10, 0), Vec2(7, 18));
        world_->CreateEntity(ColliderComponent{ centerCrystalCollider }, Position{ pos });
    };

    // Main arena
    {
        int left = 0;
        int bot = 0;
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

        Box2D groundCollider = MakeBox2DMinMax(Vec2((left + 0.25f) * TILE_SIZE, bot * TILE_SIZE), Vec2((left + width + 1.75f) * TILE_SIZE, 1.5f * TILE_SIZE));
        world_->CreateEntity(ColliderComponent{ groundCollider }, Position{ Vec3::ZERO() }, ColliderTag::Ground);

        Box2D leftWallCollider = MakeBox2DMinMax(Vec2((left) * TILE_SIZE, bot * TILE_SIZE), Vec2((left + 0.75f) * TILE_SIZE, height * TILE_SIZE));
        world_->CreateEntity(ColliderComponent{ leftWallCollider }, Position{ Vec3::ZERO() }, ColliderTag::Ground);

        Box2D rightWallCollider = MakeBox2DMinMax(Vec2((left + width + 1.25f) * TILE_SIZE, bot * TILE_SIZE), Vec2((left + width + 2) * TILE_SIZE, height * TILE_SIZE));
        world_->CreateEntity(ColliderComponent{ rightWallCollider }, Position{ Vec3::ZERO() }, ColliderTag::Ground);
    }

    // Platforms
    auto MakePlatform = [this](float left, float bot, float width)
    {
        int height = 1;

        Box2D groundCollider = MakeBox2DMinMax(
            Vec2((left + 0.25f) * TILE_SIZE, bot * TILE_SIZE),
            Vec2((left + 0.75f + width) * TILE_SIZE, (bot + height - 0.5f) * TILE_SIZE)
        );
        world_->CreateEntity(ColliderComponent{ groundCollider }, Position{ Vec3::ZERO() }, ColliderTag::Ground);

        AddSprite(TilePos(left, bot + height - 1), &groundSprite_[TOP_LEFT]);
        for (float x = left + 1; x < left + width; ++x)
            AddSprite(TilePos(x, bot + height - 1), &groundSprite_[TOP]);
        AddSprite(TilePos(left + width, bot + height - 1), &groundSprite_[TOP_RIGHT]);
    };

    MakePlatform(1, 4, 3);
    MakePlatform(5, 7.5f, 3);
    MakePlatform(14, 6, 4);
    MakePlatform(20, 10, 2);
    MakePlatform(20, 4.5, 1);

    MakeFlowerSmallCluster(7, 7.5f, 0);

    MakeFlowerSmallCluster(15, 6, 0);
    MakePumpkin(18, 6, -6, 0);

    MakeAmanita(1, 1, 4);
    MakeAmanita(1.875f, 1, 3);
    MakeAmanita(4, 1, 4);

    MakeFlowerSmall(1, 1, -6, 3);
    MakeFlowerSmall(5, 1, 0, 3);

    MakeFlowerSmallCluster(3, 1, 0);
    MakeFlowerSmallCluster(2, 4, 0);

    MakeCrystal(8, 1.5f, 0, -3, 0);
    MakeCrystal(10, 1.5f, 0, 0, -0.1f);
    MakeCrystal(11, 1.5f, 2, -2, 0.0f);
    MakeCrystal(11, 2.0f, 0, -1, 0.1f);

    MakeSunflower(19, 1, 3, -8);
    MakeSunflower(20, 1, 0, 0);
    MakeSunflower(21, 1, -2, -5);

    MakeSunflower(22, 10, 0, -5);

    MakePumpkin(22, 1, 6, 0);

    // Targets
    world_->CreateEntity(TargetRespawnTimer{ TilePos(21, 12, LAYER_TARGET), TARGET_COOLDOWN });
    world_->CreateEntity(TargetRespawnTimer{ TilePos(16, 5, LAYER_TARGET), TARGET_COOLDOWN });
    //world_->CreateEntity(TargetRespawnTimer{ TilePos(17, 8, LAYER_TARGET), TARGET_COOLDOWN });
    //world_->CreateEntity(TargetRespawnTimer{ TilePos(7, 8, LAYER_TARGET), TARGET_COOLDOWN });

    // Spawn points
    world_->CreateEntity(Position{ Vec3(20 * TILE_SIZE, 5 * TILE_SIZE, 1) }, SpawnPoint{});
    world_->CreateEntity(Position{ Vec3(6 * TILE_SIZE, 8 * TILE_SIZE, 1) }, SpawnPoint{});
    world_->CreateEntity(Position{ Vec3(2 * TILE_SIZE, 1.5 * TILE_SIZE, 1) }, SpawnPoint{});
    world_->CreateEntity(Position{ Vec3(10 * TILE_SIZE, 0.5f * TILE_SIZE + 50, 1) }, SpawnPoint{});

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

    //
    font_ = MakeUnique<Font>();
    if (HS_FAILED(font_->Init("PixelFont")))
        return R_FAIL;

    // TODO(pavel): Abstract this to the engine, only play sounds from game
    SDL_AudioSpec musicSpec;
    if (!SDL_LoadWAV("sounds/music.wav", &musicSpec, &musicBuffer_, &musicLength_))
    {
        LOG_ERR("Failed to load music");
        return R_FAIL;
    }

    SDL_AudioSpec want{};
    SDL_AudioSpec have{};

    want.freq = musicSpec.freq;
    want.format = musicSpec.format;
    want.channels = musicSpec.channels;
    want.samples = musicSpec.samples;
    want.callback = nullptr;

    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    SDL_PauseAudioDevice(audioDevice_, 0);

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

    if (HS_FAILED(MakeSimpleSprite("textures/AmanitaMuscaria.png", amanitaSprite_, Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Crystal.png", crystalSprite_, Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Sunflower.png", sunflowerSprite_, Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/FlowerSmall.png", flowerSmallSprite_, Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Arrow.png", arrowSprite_, Vec2(0.5f, 0.5f))))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/Target.png", targetSprite_, Vec2::ZERO())))
        return R_FAIL;

    if (HS_FAILED(MakeSimpleSprite("textures/BowSimple.png", bowSprite_, Vec2(0.1f, 0.5f))))
        return R_FAIL;

    if (HS_FAILED(LoadMap()))
        return R_FAIL;

    SpawnPlayer();

    Camera& cam = g_Render->GetCamera();
    cam.SetPosition(Vec2{ 12 * TILE_SIZE, 7.5f * TILE_SIZE });
    cam.UpdateMatrices();

    return R_OK;
}

//------------------------------------------------------------------------------
void Game::Free()
{
    SDL_FreeWAV(musicBuffer_);
}

//------------------------------------------------------------------------------
float Game::GetDTime()
{
    const float dtime = g_Engine->GetDTime();
    return timeScale_ * dtime;
}

//------------------------------------------------------------------------------
RESULT Game::OnWindowResized()
{
    InitCamera();
    return R_OK;
}

//------------------------------------------------------------------------------
static float height = 32;
static float timeToJump = 0.3f;
static float jumpVelocity = (2 * height) / (timeToJump);
static float gravity = (-2 * height) / Sqr(timeToJump);
static float groundLevel = 8;

//------------------------------------------------------------------------------
static float projectileGravity = gravity / 10;
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
    hs_assert(fabs(dirNormalized.Length() - 1.0f) < 0.001f && "Direction must be normalized");

    float dotX = dirNormalized.Dot(Vec2::RIGHT());
    float dotY = dirNormalized.Dot(Vec2::UP());
    float angle = acosf(dotX);
    if (dotY < 0)
        angle = HS_TAU - angle;

    return angle;
}

//------------------------------------------------------------------------------
static Vec2 DirectionFromRotation(float rotation)
{
    return Vec2(cosf(rotation), sinf(rotation));
}

//------------------------------------------------------------------------------
void Game::Update()
{
    // Audio
    if (!muteAudio_ && SDL_GetQueuedAudioSize(audioDevice_) < 2 * musicLength_)
    {
        if (SDL_QueueAudio(audioDevice_, musicBuffer_, musicLength_) != 0)
        {
            LOG_ERR("Failed to queue audio %s", SDL_GetError());
            SDL_ClearError();
        }
    }

    // Debug
    if (g_Input->IsKeyDown(KC_C))
    {
        visualizeColliders_ = !visualizeColliders_;
    }

    ImGui::Begin("Score");
        for (int playerI = 0; playerI < playerCount_; ++playerI)
        {
            ImGui::Text("Player %d: %d", playerI, playerScore_[playerI]);
        }
    ImGui::End();

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

    float aimDeadzone = 0.2f;
    ImGui::Begin("Settings");
        ImGui::SliderFloat("Aim deadzone", &aimDeadzone, 0.0f, 1.0f);
        ImGui::SliderFloat("Projectile speed", &projectileSpeed, 0.0f, 500.0f);
        ImGui::SliderFloat("Time scale", &timeScale_, 0.0f, 4.0f);
    ImGui::End();

    // Movement
    float focusMultiplier[MAX_PLAYERS]{};
    for (int playerI = 0; playerI < playerCount_; ++playerI)
    {
        if (players_[playerI].playerEntity_ == NULL_ENTITY)
            continue;

        if (g_Input->GetState(KC_LSHIFT) || (gamepadForPlayer_[playerI] != -1 && g_Input->GetAxis(gamepadForPlayer_[playerI], GLFW_GAMEPAD_AXIS_LEFT_TRIGGER) > -0.5))
            focusMultiplier[playerI] = 0.25f;
        else
            focusMultiplier[playerI] = 1.0;

        Vec2& velocity = world_->GetComponent<Velocity>(players_[playerI].playerEntity_);
        velocity.y += gravity * GetDTime() * focusMultiplier[playerI];
        velocity.x = 0;

        if (!isGrounded_[playerI])
            coyoteTimeRemaining_[playerI] -= GetDTime();

        float characterSpeed{ 80 };
        if (g_Input->IsKeyDown(KC_SPACE)
            || (gamepadForPlayer_[playerI] != -1 && g_Input->IsButtonDown(gamepadForPlayer_[playerI], GLFW_GAMEPAD_BUTTON_A))
            || (gamepadForPlayer_[playerI] != -1 && g_Input->IsButtonDown(gamepadForPlayer_[playerI], GLFW_GAMEPAD_BUTTON_LEFT_BUMPER)))
        {
            if (isGrounded_[playerI] || coyoteTimeRemaining_[playerI] > 0)
            {
                velocity.y = jumpVelocity;
                coyoteTimeRemaining_[playerI] = 0;
            }
            else if (!hasDoubleJumped_[playerI])
            {
                velocity.y = jumpVelocity;
                hasDoubleJumped_[playerI] = true;
            }
        }

        if (g_Input->GetState(KC_D))
        {
            velocity.x += characterSpeed;
        }
        else if (g_Input->GetState(KC_A))
        {
            velocity.x -= characterSpeed;
        }

        const float xAxis = gamepadForPlayer_[playerI] == -1 ? 0 :g_Input->GetAxis(gamepadForPlayer_[playerI], GLFW_GAMEPAD_AXIS_LEFT_X);
        if (xAxis)
        {
            velocity.x += characterSpeed * xAxis;
        }

        Vec3& pos = world_->GetComponent<Position>(players_[playerI].playerEntity_);

        isGrounded_[playerI] = false;

        Vec2 dtVel = velocity * GetDTime() * focusMultiplier[playerI];

        Vec2 pos2 = pos.XY();
        const ColliderComponent& originalCollider = world_->GetComponent<ColliderComponent>(players_[playerI].playerEntity_);
        Box2D playerCollider = originalCollider.collider_.Offset(pos.XY());

        auto SolveIntersection = [this, pos2, &dtVel, &originalCollider, &playerCollider, &velocity](const Box2D& groundCollider, int playerI)
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
                        hasDoubleJumped_[playerI] = false;
                        coyoteTimeRemaining_[playerI] = coyoteTimeSec_;
                    }
                    else if (closeNormal == -Vec2::UP())
                    {
                        // Bounce from the object that is above us - stop the jump
                        dtVel.y = 0;
                        velocity.y = 0;
                    }
                }
            }
        };

        EcsWorld::Iter<const ColliderComponent, const Position>(world_.Get()).EachExcept<PlayerComponent>(
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

        static Vec2 maxPlayerVelocity(Vec2::ZERO());
        static Vec2 minPlayerVelocity(Vec2::ZERO());
        maxPlayerVelocity.x = Max(maxPlayerVelocity.x, velocity.x);
        maxPlayerVelocity.y = Max(maxPlayerVelocity.y, velocity.y);
        minPlayerVelocity.x = Min(minPlayerVelocity.x, velocity.x);
        minPlayerVelocity.y = Min(minPlayerVelocity.y, velocity.y);

        ImGui::Text("IsGrounded %d", isGrounded_[0]);
        ImGui::Text("Cur player %d velocity: [%.2f, %.2f]", playerI, velocity.x, velocity.y);
        ImGui::Text("Max player %d velocity: [%.2f, %.2f]", playerI, maxPlayerVelocity.x, maxPlayerVelocity.y);
        ImGui::Text("Min player %d velocity: [%.2f, %.2f]", playerI, minPlayerVelocity.x, minPlayerVelocity.y);

        // Weapon update
        {
            const Sprite* playerSprite = world_->GetComponent<AnimationState>(players_[playerI].playerEntity_).GetCurrentSprite();
            Vec3 weaponPos(0, 0, LAYER_WEAPON);
            weaponPos.x = pos.x + playerSprite->size_.x / 2.0f;
            weaponPos.y = pos.y + playerSprite->size_.y / 2.0f;

            Vec2 dir;
            dir.x = gamepadForPlayer_[playerI] == -1 ? 0 : g_Input->GetAxis(gamepadForPlayer_[playerI], GLFW_GAMEPAD_AXIS_RIGHT_X);
            dir.y = gamepadForPlayer_[playerI] == -1 ? 0 : -g_Input->GetAxis(gamepadForPlayer_[playerI], GLFW_GAMEPAD_AXIS_RIGHT_Y);
            if (dir.Length() > aimDeadzone)
            {
                Vec2 dirNormalized = dir.Normalized();
                constexpr float AIM_STEP = HS_TAU / (36.0f * 2);

                const float angle = RotationFromDirection(dirNormalized);
                const float inNumbers = (angle * 0.5f) / AIM_STEP;
                const float snapNumber = round(inNumbers);
                const float snapAngle = (snapNumber * AIM_STEP) / 0.5f;

                world_->SetComponents<Rotation>(players_[playerI].weaponEntity_, Rotation { snapAngle });
            }

            world_->SetComponents<Position>(players_[playerI].weaponEntity_, Position{ weaponPos });
        }

        // Shooting
        timeToShoot_[playerI] = Max(timeToShoot_[playerI] - GetDTime(), 0.0f);
        if (timeToShoot_[playerI] <= 0)
        {
            const Vec2 projPos = world_->GetComponent<Position>(players_[playerI].playerEntity_).XY() + world_->GetComponent<SpriteComponent>(players_[playerI].playerEntity_).sprite_->size_ / 2;
            Vec2 dir;
            bool shouldShoot = false;

            if (g_Input->IsButtonDown(BTN_LEFT))
            {
                shouldShoot = true;
                Vec2 to = CursorToWorld();
                dir = (to - projPos);
            }
            else if (gamepadForPlayer_[playerI] != -1 && g_Input->IsButtonDown(gamepadForPlayer_[playerI], GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER))
            {
                shouldShoot = true;
                /*dir.x = g_Input->GetAxis(gamepadForPlayer_[playerI], GLFW_GAMEPAD_AXIS_RIGHT_X);
                dir.y = -g_Input->GetAxis(gamepadForPlayer_[playerI], GLFW_GAMEPAD_AXIS_RIGHT_Y);

                if (dir.LengthSqr() == 0)
                    dir.x = world_->GetComponent<Velocity>(players_[playerI].playerEntity_).x;

                if (dir.LengthSqr() == 0)
                    dir.x = 1;*/
                dir = DirectionFromRotation(world_->GetComponent<Rotation>(players_[playerI].weaponEntity_).angle_);
            }

            if (shouldShoot)
            {
                timeToShoot_[playerI] = SHOOT_COOLDOWN;
                dir.Normalize();

                float angle = RotationFromDirection(dir);

                constexpr float PLAYER_VELOCITY_WEIGHT = 0.7f;
                Vec2 projectileVelocity = dir * projectileSpeed + world_->GetComponent<Velocity>(players_[playerI].playerEntity_) * PLAYER_VELOCITY_WEIGHT * focusMultiplier[playerI];
                AddProjectile(
                    Vec3(projPos.x, projPos.y, 0.5f),
                    angle,
                    &arrowSprite_,
                    Circle(Vec2(7, 2.5f), 1.5f),
                    projectileVelocity,
                    playerI
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
                    velocity.y += projectileGravity * GetDTime();
                    position.AddXY(velocity * GetDTime());

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
            Array<TargetRespawnTimer> toCreateTargetRespawn;
            Array<PlayerRespawnTimer> toCreatePlayerRespawn;
            EcsWorld::Iter<const Entity_t, Position, Rotation, TipCollider, SpriteComponent, const Projectile>(world_.Get()).Each(
                [this, &toRemove, &toCreateTargetRespawn, &toCreatePlayerRespawn]
                (Entity_t projectile, Position& position, Rotation& rotation, TipCollider& collider, SpriteComponent& sprite, Projectile projectileComponent)
                {
                    Mat44 projectileTransform = MakeTransform(position, rotation.angle_, sprite.sprite_->pivot_);
                    Vec2 projPos = projectileTransform.TransformPos(collider.collider_.center_);
                    bool shouldRemove = false;

                    EcsWorld::Iter<const Entity_t, const Position, const TargetCollider>(world_.Get()).Each(
                        [this, &shouldRemove, &toRemove, &toCreateTargetRespawn, projPos, &tipCollider = collider.collider_, projectileComponent]
                        (Entity_t target, Position targetPos, TargetCollider targetCollider)
                        {
                            Vec2 tgtPos = targetPos.XY() + targetCollider.collider_.center_;
                            if (IsIntersecting(Circle(projPos, tipCollider.radius_), Circle(tgtPos, targetCollider.collider_.radius_)))
                            {
                                shouldRemove = true;

                                playerScore_[projectileComponent.shooterId_] += TARGET_DESTROY_SCORE;
                                toRemove.AddUnique(target);
                                toCreateTargetRespawn.Add(TargetRespawnTimer{ targetPos, TARGET_COOLDOWN });
                            }
                        }
                    );

                    EcsWorld::Iter<const Entity_t, const Position, const ColliderComponent, const PlayerComponent>(world_.Get()).Each(
                        [this, &shouldRemove, &toRemove, &toCreatePlayerRespawn, projPos, &tipCollider = collider.collider_, projectileComponent]
                        (Entity_t playerEntity, Position playerPos, ColliderComponent playerCollider, const PlayerComponent& player)
                        {
                            Box2D playerColliderWorld = playerCollider.collider_.Offset(playerPos.XY());
                            if (player.playerId_ != projectileComponent.shooterId_ && IsIntersecting(playerColliderWorld, Circle(projPos, tipCollider.radius_)))
                            {
                                shouldRemove = true;

                                playerScore_[projectileComponent.shooterId_] += PLAYER_KILL_SCORE;
                                toRemove.AddUnique(playerEntity);
                                toRemove.AddUnique(players_[player.playerId_].weaponEntity_);
                                toCreatePlayerRespawn.Add(PlayerRespawnTimer{ player.playerId_, PLAYER_RESPAWN_TIME });
                                players_[player.playerId_] = { NULL_ENTITY, NULL_ENTITY };
                                LOG_DBG("Player %d killed by player %d, score: %d, %d", player.playerId_, projectileComponent.shooterId_, playerScore_[0], playerScore_[1]);
                            }
                        }
                    );

                    EcsWorld::Iter<const ColliderComponent, const Position>(world_.Get()).EachExcept<PlayerComponent>(
                        [&shouldRemove, &tipCollider = collider.collider_, projPos]
                        (const ColliderComponent& collider, const Position& pos)
                        {
                            if (IsIntersecting(collider.collider_.Offset(pos.XY()), Circle(projPos, tipCollider.radius_)))
                            {
                                shouldRemove = true;
                                return;
                            }
                        }
                    );

                    if (shouldRemove)
                        toRemove.AddUnique(projectile);
                }
            );

            for (int i = 0; i < toRemove.Count(); ++i)
                world_->DeleteEntity(toRemove[i]);

            for (const auto& targetRespawn : toCreateTargetRespawn)
                world_->CreateEntity(targetRespawn);

            for (int i = 0; i < toCreatePlayerRespawn.Count(); ++i)
                world_->CreateEntity(toCreatePlayerRespawn[i]);
        }
    }

    // Spawn players
    {
        Array<Entity_t> timersToRemove;
        EcsWorld::Iter<const Entity_t, PlayerRespawnTimer>(world_.Get()).Each(
            [this, dTime = GetDTime(), &timersToRemove](Entity_t eid, PlayerRespawnTimer& timer)
            {
                timer.timeLeft_ -= dTime;
                if (timer.timeLeft_ <= 0)
                {
                    timersToRemove.Add(eid);
                    players_[timer.playerEntity_] = RespawnPlayer(timer.playerEntity_);
                }
            }
        );

        for (int i = 0; i < timersToRemove.Count(); ++i)
            world_->DeleteEntity(timersToRemove[i]);
    }

    // Spawn targets
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

    // TODO(pavel): 0,0 for UI top left or bottom left?
    g_Render->GetGuiRenderer()->AddText(font_.Get(), StringView("HELLO"), Vec2(100, 200));
}

}

