#pragma once

#include "game/Camera.h"
#include "game/TileRenderer.h"

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
    Tile* tile_;
    float time_;
};

//------------------------------------------------------------------------------
struct AnimationState
{
    RESULT Init(const Array<AnimationSegment>& segments);
    void Update(float dTime);
    Tile* GetCurrentTile() const;

    Array<AnimationSegment> segments_;
    uint currentSegment_{};
    float timeToSwap_;
};

//------------------------------------------------------------------------------
struct TileArchetype
{
    Array<Vec3>     Positions;
    Array<Tile*>    Tiles;
};

//------------------------------------------------------------------------------
struct ObjectArchetype
{
    Array<Vec3>     Positions;
    Array<Tile*>    Tiles;
    Array<Box2D>    Colliders;
};

//------------------------------------------------------------------------------
struct CharacterArchetype
{
    Array<Vec3>             Positions;
    Array<Tile*>            Tiles;
    Array<AnimationState>   Animations;
    Array<Box2D>            Colliders;
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

    Tile groundTile_[3 * 3]{};
    Tile goldChestTile_{};
    Tile rockTile_[2]{};
    Tile forestTile_{};
    Tile forestDoorTile_{};

    // Archetypes
    TileArchetype       tiles_{};
    CharacterArchetype  characters_{};
    GroundArchetype     ground_{};
    ObjectArchetype     objects_{};

    // Debug
    bool visualizeColliders_{};

    void AddTile(const Vec3& pos, Tile* tile);
    void AddObject(const Vec3& pos, Tile* tile, const Box2D* collider);
    void AddCharacter(const Vec3& pos, const AnimationState& animation, const Box2D& collider);
    void AnimateTiles();
    void DrawColliders();
};

}
