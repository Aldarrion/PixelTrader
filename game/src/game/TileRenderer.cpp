#include "game/TileRenderer.h"

#include "render/Render.h"

#include "common/Logging.h"

namespace hs
{

//------------------------------------------------------------------------------
RESULT TileRenderer::Init()
{
    tileMaterial_ = new TileMaterial();

    if (HS_FAILED(tileMaterial_->Init()))
    {
        Log(LogLevel::Error, "Failed to init material");
        return R_FAIL;
    }

    return R_OK;
}

//------------------------------------------------------------------------------
void TileRenderer::ClearTiles()
{
    drawCalls_.Clear();
}

//------------------------------------------------------------------------------
void TileRenderer::AddTile(Tile* tile, Vec3 position)
{
    const Box2D tileBoundBox(Vec2(position.x, position.y), Vec2(position.x + tile->size_.x, position.y + tile->size_.y));
    const Box2D frustum = g_Render->GetCamera().GetOrthoFrustum();

    if (!IsIntersecting(tileBoundBox, frustum))
        return;

    drawCalls_.Add(TileDrawCall{ tile, position });
}

//------------------------------------------------------------------------------
int TileDrawCallCmp(const void *a, const void *b)
{
    auto ta = (TileDrawCall*)a;
    auto tb = (TileDrawCall*)b;

    if (ta->position_.z > tb->position_.z)
        return -1;
    if (ta->position_.z < tb->position_.z)
        return 1;
    return 0;
}

//------------------------------------------------------------------------------
void TileRenderer::Draw()
{
    // TODO improve drawing and add batching
    qsort(drawCalls_.Data(), drawCalls_.Count(), sizeof(TileDrawCall), &TileDrawCallCmp);

    for (int i = 0, count = drawCalls_.Count(); i < count; ++i)
    {
        tileMaterial_->DrawTile(TileDrawData{
            drawCalls_[i].tile_->texture_,
            drawCalls_[i].tile_->uvBox_,
            drawCalls_[i].tile_->size_,
            drawCalls_[i].position_
        });
    }
}

}
