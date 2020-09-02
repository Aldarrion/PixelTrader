#pragma once

#include "game/Camera.h"

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
class Game
{
public:
    ~Game();

    RESULT InitWin32();
    void Update(float dTime);

    float GetDTime() const;

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

    float dTime_{};

    Texture* groundTileTex_;
    Tile* groundTile_[3 * 3]{};
};

}
