#pragma once

#include "World/Camera.h"
#include "Game/SpriteRenderer.h"

#include "Ecs/Ecs.h"

#include "Game/GameBase.h"

#include "Containers/Array.h"

#include "Common/Enums.h"
#include "Common/Types.h"


namespace hs
{

//------------------------------------------------------------------------------
class Texture;

//------------------------------------------------------------------------------
extern class Game* g_Game;

//------------------------------------------------------------------------------
struct AnimationSegment
{
    Sprite* sprite_;
    float time_;
};

//------------------------------------------------------------------------------
struct PlayerInfo
{
    Entity_t playerEntity_;
    Entity_t weaponEntity_;
};

//------------------------------------------------------------------------------
struct AnimationState;

//------------------------------------------------------------------------------
class Game : public GameBase
{
public:
    ~Game();

    RESULT Init();
    RESULT OnWindowResized();
    void Update();

private:
    enum GroundTile
    {
        TOP_LEFT,
        TOP,
        TOP_RIGHT,
        MID_LEFT,
        MID,
        MID_RIGHT,
        BOT_LEFT,
        BOT,
        BOT_RIGHT,
    };

    static constexpr uint   MAX_PLAYERS{ 2 };
    static constexpr float  SHOOT_COOLDOWN{ 0.5f };
    static constexpr float  TARGET_COOLDOWN{ 3.0f };
    static constexpr float  PLAYER_RESPAWN_TIME{ 5.0f };
    static constexpr int    TARGET_DESTROY_SCORE{ 1 };
    static constexpr int    PLAYER_KILL_SCORE{ 5 };

    static constexpr float  LAYER_WEAPON{ 0.4f };

    UniquePtr<EcsWorld> world_;

    Sprite groundSprite_[3 * 3]{};
    Sprite goldChestSprite_{};
    Sprite rockSprite_[2]{};
    Sprite pumpkinSprite_[2]{};
    Sprite forestSprite_{};
    Sprite forestDoorSprite_{};
    Sprite arrowSprite_{};
    Sprite targetSprite_{};
    Sprite bowSprite_{};

    int         playerCount_{ 0 };
    PlayerInfo  players_[MAX_PLAYERS]{};

    int         gamepadForPlayer_[MAX_PLAYERS]{};
    float       timeToShoot_[MAX_PLAYERS]{};
    int         playerScore_[MAX_PLAYERS]{};
    bool        isGrounded_[MAX_PLAYERS]{};

    // Debug
    bool visualizeColliders_{};

    void InitEcs();
    void InitCamera();

    float GetDTime();

    void AddSprite(const Vec3& pos, Sprite* sprite);
    void AddObject(const Vec3& pos, const AnimationState& animation, const Box2D* collider);
    Entity_t SpawnPlayer();
    [[nodiscard]] PlayerInfo RespawnPlayer(int playerId);

    void AddProjectile(const Vec3& pos, float rotation, Sprite* sprite, const Circle& tipCollider, Vec2 velocity, int playerId);
    void RemoveProjectile(uint idx);

    void AddTarget(const Vec3& pos, Sprite* sprite, const Circle& collider);
    void RemoveTarget(uint idx);

    void AnimateSprites();
    void DrawColliders();

    RESULT LoadMap();
};

}
