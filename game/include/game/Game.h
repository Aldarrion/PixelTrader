#pragma once

#include "game/Camera.h"

#include "containers/Array.h"

#include "common/Enums.h"
#include "common/Types.h"


namespace hs
{

//------------------------------------------------------------------------------
struct Tile;
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

    Texture* groundTileTex_{};
    Tile* groundTile_[3 * 3]{};

    Texture* goldChestTex_{};
    Tile* goldChestTile_{};

    Texture* rockTex_[2]{};
    Tile* rockTile_[2]{};
    AnimationState rockIdle_{};
};

}
