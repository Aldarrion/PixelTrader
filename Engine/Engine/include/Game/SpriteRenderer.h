#pragma once

#include "Common/Enums.h"

#include "Render/Material.h"

#include "Containers/Array.h"

namespace hs
{

//------------------------------------------------------------------------------
struct Sprite
{
    Texture* texture_;
    Vec4 uvBox_;
    Vec2 size_;
    Vec2 pivot_;
};

//------------------------------------------------------------------------------
struct SpriteDrawCall
{
    Sprite* sprite_;
    Mat44 world_;
};

//------------------------------------------------------------------------------
class SpriteRenderer
{
public:
    RESULT Init();
    void Draw();

    void ClearSprites();
    void AddSprite(Sprite* sprite, const Mat44& world);

private:
    SpriteMaterial spriteMaterial_;
    Array<SpriteDrawCall> drawCalls_;
};

}
