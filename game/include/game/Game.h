#pragma once

#include "game/Camera.h"
#include "game/SpriteRenderer.h"

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
struct SpriteArchetype
{
    Array<Vec3>     Positions;
    Array<Sprite*>  Sprites;
};

//------------------------------------------------------------------------------
struct ObjectArchetype
{
    Array<Vec3>     Positions;
    Array<Sprite*>  Sprites;
    Array<Box2D>    Colliders;
};

//------------------------------------------------------------------------------
struct CharacterArchetype
{
    Array<Vec3>             Positions;
    Array<Vec2>             Velocities;
    Array<Sprite*>          Sprites;
    Array<AnimationState>   Animations;
    Array<Box2D>            Colliders;
};

//------------------------------------------------------------------------------
struct ProjectileArchetype
{
    Array<Vec3>     Positions;
    Array<float>    Rotations;
    Array<Sprite*>  Sprites;
    Array<Vec2>     Velocities;
    Array<Circle>   TipColliders;
};

//------------------------------------------------------------------------------
struct TargetArchetype
{
    Array<Vec3>     Positions;
    Array<Circle>   Colliders;
    Array<Sprite*>  Sprites;
};

//------------------------------------------------------------------------------
enum class ColliderTag
{
    None,
    Ground
};

//------------------------------------------------------------------------------
struct GroundArchetype
{
    Array<Box2D>        Colliders;
    Array<ColliderTag>  Tags;
};

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

    bool isWindowActive_{};

    float dTime_{};

    Sprite groundSprite_[3 * 3]{};
    Sprite goldChestSprite_{};
    Sprite rockSprite_[2]{};
    Sprite forestSprite_{};
    Sprite forestDoorSprite_{};
    Sprite arrowSprite_{};
    Sprite targetSprite_{};

    // Archetypes
    SpriteArchetype     sprites_{};
    CharacterArchetype  characters_{};
    GroundArchetype     ground_{};
    ObjectArchetype     objects_{};
    ProjectileArchetype projectiles_{};
    TargetArchetype     targets_{};

    // Debug
    bool visualizeColliders_{};

    void InitCamera();

    void AddSprite(const Vec3& pos, Sprite* sprite);
    void AddObject(const Vec3& pos, Sprite* sprite, const Box2D* collider);
    void AddCharacter(const Vec3& pos, const AnimationState& animation, const Box2D& collider);

    void AddProjectile(const Vec3& pos, float rotation, Sprite* sprite, const Circle& tipCollider, Vec2 velocity);
    void RemoveProjectile(uint idx);

    void AddTarget(const Vec3& pos, Sprite* sprite, const Circle& collider);
    void RemoveTarget(uint idx);


    void AnimateSprites();
    void DrawColliders();
};

}
