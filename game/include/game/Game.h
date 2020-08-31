#pragma once

#include "common/Enums.h"

#include "common/Types.h"

namespace hs
{

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
    float dTime_{};
};

}
