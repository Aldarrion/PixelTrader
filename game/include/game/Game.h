#pragma once

#include "game/Camera.h"
#include "game/SpriteRenderer.h"

#include "ecs/Ecs.h"

#include "containers/Array.h"

#include "common/Enums.h"
#include "common/Types.h"


namespace hs
{

//------------------------------------------------------------------------------
class Texture;

//------------------------------------------------------------------------------
extern class Game* g_Game;

//------------------------------------------------------------------------------
RESULT CreateGame();
void DestroyGame();

//------------------------------------------------------------------------------
struct AnimationSegment
{
    Sprite* sprite_;
    float time_;
};

//------------------------------------------------------------------------------
struct AnimationState;

//------------------------------------------------------------------------------
class Game
{
public:
    ~Game();

    RESULT InitWin32();
    RESULT OnWindowResized();
    void Update(float dTime);

    float GetDTime() const;
    bool IsWindowActive() const;
    void SetWindowActive(bool isActive);

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

    UniquePtr<EcsWorld> world_;

    bool isWindowActive_{};

    float dTime_{};

    Sprite groundSprite_[3 * 3]{};
    Sprite goldChestSprite_{};
    Sprite rockSprite_[2]{};
    Sprite pumpkinSprite_[2]{};
    Sprite forestSprite_{};
    Sprite forestDoorSprite_{};
    Sprite arrowSprite_{};
    Sprite targetSprite_{};

    Entity_t character_{};

    // Debug
    bool visualizeColliders_{};

    void InitEcs();
    void InitCamera();

    void AddSprite(const Vec3& pos, Sprite* sprite);
    void AddObject(const Vec3& pos, const AnimationState& animation, const Box2D* collider);
    Entity_t AddCharacter(const Vec3& pos, const AnimationState& animation, const Box2D& collider);

    void AddProjectile(const Vec3& pos, float rotation, Sprite* sprite, const Circle& tipCollider, Vec2 velocity);
    void RemoveProjectile(uint idx);

    void AddTarget(const Vec3& pos, Sprite* sprite, const Circle& collider);
    void RemoveTarget(uint idx);


    void AnimateSprites();
    void DrawColliders();
};

}
