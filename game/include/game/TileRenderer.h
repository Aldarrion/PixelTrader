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
    Vec2 size_;
};

//------------------------------------------------------------------------------
struct TileDrawCall
{
    Tile* tile_;
    Vec3 position_;
    float rotation_;
    Vec2 pivot_;
};

//------------------------------------------------------------------------------
class TileRenderer
{
public:
    RESULT Init();
    void Draw();

    void ClearTiles();
    void AddTile(Tile* tile, Vec3 position, float rotation = 0.0f, Vec2 pivot = Vec2::ZERO());

private:
    TileMaterial tileMaterial_;
    Array<TileDrawCall> drawCalls_;
};

}
