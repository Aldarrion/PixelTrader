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
void SpriteRenderer::AddSprite(Sprite* tile, Vec3 position, float rotation, Vec2 pivot)
{
    const Box2D tileBoundBox = MakeBox2DMinMax(Vec2(position.x, position.y), Vec2(position.x + tile->size_.x, position.y + tile->size_.y));
    const Box2D frustum = g_Render->GetCamera().GetOrthoFrustum();

    if (!IsIntersecting(tileBoundBox, frustum))
        return;

    drawCalls_.Add(SpriteDrawCall{ tile, position, rotation, pivot });
}

//------------------------------------------------------------------------------
static int SpriteDrawCallCmp(const void *a, const void *b)
{
    auto ta = (SpriteDrawCall*)a;
    auto tb = (SpriteDrawCall*)b;

    if (ta->position_.z > tb->position_.z)
        return -1;
    if (ta->position_.z < tb->position_.z)
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
            drawCalls_[i].position_,
            drawCalls_[i].rotation_,
            drawCalls_[i].pivot_
        });
    }
}

}
