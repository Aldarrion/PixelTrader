#include "game/SpriteRenderer.h"

#include "render/Render.h"

#include "common/Logging.h"

namespace hs
{

//------------------------------------------------------------------------------
RESULT SpriteRenderer::Init()
{
    if (HS_FAILED(spriteMaterial_.Init()))
    {
        Log(LogLevel::Error, "Failed to init material");
        return R_FAIL;
    }

    return R_OK;
}

//------------------------------------------------------------------------------
void SpriteRenderer::ClearSprites()
{
    drawCalls_.Clear();
}

//------------------------------------------------------------------------------
void SpriteRenderer::AddSprite(Sprite* sprite, const Mat44& world)
{
    Vec2 pos = world.GetPositionXY();
    // TODO(pavel): This is not correct, we need to take into account the pivot and rotation as well, bounding box should be probably a part of the sprite
    const Box2D tileBoundBox = MakeBox2DMinMax(pos - sprite->size_, pos + sprite->size_);
    const Box2D frustum = g_Render->GetCamera().GetOrthoFrustum();

    if (!IsIntersecting(tileBoundBox, frustum))
        return;

    drawCalls_.Add(SpriteDrawCall{ sprite, world });
}

//------------------------------------------------------------------------------
static int SpriteDrawCallCmp(const void *a, const void *b)
{
    auto posA = ((SpriteDrawCall*)a)->world_.GetPosition();
    auto posB = ((SpriteDrawCall*)b)->world_.GetPosition();

    if (posA.z > posB.z)
        return -1;
    if (posA.z < posB.z)
        return 1;
    return 0;
}

//------------------------------------------------------------------------------
void SpriteRenderer::Draw()
{
    g_Render->ResetState();

    // TODO improve drawing and add batching
    qsort(drawCalls_.Data(), drawCalls_.Count(), sizeof(SpriteDrawCall), &SpriteDrawCallCmp);

    //Log(LogLevel::Info, "Sprite draw calls: %d", drawCalls_.Count());
    for (int i = 0, count = drawCalls_.Count(); i < count; ++i)
    {
        spriteMaterial_.DrawSprite(SpriteDrawData{
            drawCalls_[i].sprite_->texture_,
            drawCalls_[i].sprite_->uvBox_,
            drawCalls_[i].sprite_->size_,
            drawCalls_[i].world_
        });
    }
}

}
