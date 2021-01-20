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

    UniquePtr<EcsWorld> world_;

    Sprite groundSprite_[3 * 3]{};
    Sprite goldChestSprite_{};
    Sprite rockSprite_[2]{};
    Sprite pumpkinSprite_[2]{};
    Sprite forestSprite_{};
    Sprite forestDoorSprite_{};
    Sprite arrowSprite_{};
    Sprite targetSprite_{};

    int         playerCount_{ 0 };
    Entity_t    players_[MAX_PLAYERS]{};

    int         gamepadForPlayer_[MAX_PLAYERS]{};
    float       timeToShoot_[MAX_PLAYERS]{};
    bool        isGrounded_[MAX_PLAYERS]{};

    // Debug
    bool visualizeColliders_{};

    void InitEcs();
    void InitCamera();

    float GetDTime();

    void AddSprite(const Vec3& pos, Sprite* sprite);
    void AddObject(const Vec3& pos, const AnimationState& animation, const Box2D* collider);
    Entity_t AddCharacter(const Vec3& pos, const AnimationState& animation, const Box2D& collider, int playerId);
    Entity_t SpawnPlayer();

    void AddProjectile(const Vec3& pos, float rotation, Sprite* sprite, const Circle& tipCollider, Vec2 velocity, int playerId);
    void RemoveProjectile(uint idx);

    void AddTarget(const Vec3& pos, Sprite* sprite, const Circle& collider);
    void RemoveTarget(uint idx);

    void AnimateSprites();
    void DrawColliders();

    RESULT LoadMap();
};

}
