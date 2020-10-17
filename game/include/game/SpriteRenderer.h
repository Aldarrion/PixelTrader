#pragma once

#include "common/Enums.h"

#include "render/Material.h"

#include "containers/Array.h"

namespace hs
{

//------------------------------------------------------------------------------
struct Sprite
{
    Texture* texture_;
    Vec4 uvBox_;
    Vec2 size_;
};

//------------------------------------------------------------------------------
struct SpriteDrawCall
{
    Sprite* sprite_;
    Vec3 position_;
    float rotation_;
    Vec2 pivot_;
};

//------------------------------------------------------------------------------
class SpriteRenderer
{
public:
    RESULT Init();
    void Draw();

    void ClearSprites();
    void AddSprite(Sprite* sprite, Vec3 position, float rotation = 0.0f, Vec2 pivot = Vec2::ZERO());

private:
    SpriteMaterial spriteMaterial_;
    Array<SpriteDrawCall> drawCalls_;
};

}
