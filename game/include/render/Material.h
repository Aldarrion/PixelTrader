#pragma once

#include "Config.h"

#include "containers/Span.h"

#include "common/Pointers.h"
#include "common/Enums.h"
#include "common/Types.h"
#include "math/hs_Math.h"

namespace hs
{

//------------------------------------------------------------------------------
class Material
{
public:
    virtual ~Material() = default;
    virtual RESULT Init() = 0;
    virtual void Draw() = 0;
};

//------------------------------------------------------------------------------
struct SpriteDrawData
{
    Texture* texture_;
    Vec4 uvBox_;
    Vec2 size_;
    Mat44 world_;
};

//------------------------------------------------------------------------------
class SpriteMaterial : public Material
{
public:
    ~SpriteMaterial();

    RESULT Init() override;
    void Draw() override;
    void DrawSprite(const SpriteDrawData& data);

private:
    Shader* vs_{};
    Shader* fs_{};
    uint    vertexLayout_{};
};

//------------------------------------------------------------------------------
class DebugShapeMaterial : public Material
{
public:
    ~DebugShapeMaterial();

    RESULT Init() override;
    void Draw() override;
    void DrawShape(Span<const Vec3> verts, const Color& color);

private:
    Shader* shapeVert_{};
    Shader* shapeFrag_{};
    uint    shapeVertexLayout_{};
};




//------------------------------------------------------------------------------
// Old materials
//------------------------------------------------------------------------------
class TexturedTriangleMaterial : public Material
{
public:
    RESULT Init() override;
    void Draw() override;

private:
    Shader*     triangleVert_{};
    Shader*     triangleFrag_{};
    Texture*    texture_{};
    Texture*    textureTree_{};
    Texture*    textureBox_{};
};


//------------------------------------------------------------------------------
class PhongMaterial : public Material
{
public:
    RESULT Init() override;
    void Draw() override;

private:
    Shader*         phongVert_{};
    Shader*         phongFrag_{};
};

//------------------------------------------------------------------------------
class SkyboxMaterial : public Material
{
public:
    RESULT Init() override;
    void Draw() override;

private:
    Shader*     skyboxVert_{};
    Shader*     skyboxFrag_{};

    Texture*    skyboxCubemap_{};
};

}
