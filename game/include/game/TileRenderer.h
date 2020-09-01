#pragma once

#include "common/Enums.h"

#include "render/Material.h"

namespace hs
{

//------------------------------------------------------------------------------
class TileRenderer
{
public:
    RESULT Init();
    void Draw();

private:
    TileMaterial* tileMaterial_;
};

}
