#pragma once

#include "common/Enums.h"

#include "render/Material.h"

#include "containers/Array.h"

namespace hs
{

//------------------------------------------------------------------------------
struct Tile
{
    Texture* texture_;
    Vec4 uvBox_;
};

//------------------------------------------------------------------------------
struct TileDrawCall
{
    Tile* tile_;
    Vec3 position_;
};

//------------------------------------------------------------------------------
class TileRenderer
{
public:
    RESULT Init();
    void Draw();

    void ClearTiles();
    void AddTile(Tile* tile, Vec3 position);

private:
    TileMaterial* tileMaterial_;
    Array<TileDrawCall> drawCalls_;
};

}
