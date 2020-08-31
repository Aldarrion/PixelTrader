#pragma once

#include "Config.h"

#include "Enums.h"
#include "Types.h"

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
