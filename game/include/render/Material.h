#pragma once

#include "Config.h"

#include "common/Enums.h"
#include "common/Types.h"
#include "math/hs_Math.h"

namespace hs
{

//------------------------------------------------------------------------------
class Material
{
public:
    virtual RESULT Init() = 0;
    virtual void Draw() = 0;
};

//------------------------------------------------------------------------------
struct TileDrawData
{
    Texture* texture_;
    Vec4 uvBox_;
    Vec2 pos_;
};

//------------------------------------------------------------------------------
class TileMaterial : public Material
{
public:
    RESULT Init() override;
    void Draw() override;
    void DrawTile(const TileDrawData& data);

private:
    Shader*         tileVert_{};
    Shader*         tileFrag_{};
    Texture*        tileTex_{};
    VertexBuffer*   tilesBuffer_{};
    uint            tileVertexLayout_{};
    uint            vertsDrawn_{};
};

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
class ShapeMaterial : public Material
{
public:
    RESULT Init() override;
    void Draw() override;

private:
    Shader*         shapeVert_{};
    Shader*         shapeFrag_{};
    VertexBuffer*   vertexBuffer_{};
    uint            vertexLayout_{};
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
