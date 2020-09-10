#include "game/DebugShapeRenderer.h"

#include <utility>

namespace hs
{

//------------------------------------------------------------------------------
RESULT DebugShapeRenderer::Init()
{
    if (HS_FAILED(debugShapeMat_.Init()))
        return R_FAIL;

    return R_OK;
}

//------------------------------------------------------------------------------
void DebugShapeRenderer::Draw()
{
    for (int i = 0; i < shapes_.Count(); ++i)
    {
        debugShapeMat_.DrawShape(shapes_[i].vertices_.Data(), shapes_[i].vertices_.Count(), shapes_[i].color_);
    }
}

//------------------------------------------------------------------------------
void DebugShapeRenderer::ClearShapes()
{
    shapes_.Clear();
}

//------------------------------------------------------------------------------
void DebugShapeRenderer::AddShape(Vec3* vertices, uint vertexCount, Color color)
{
    Array<Vec3> verts;
    verts.Reserve(vertexCount);

    for (uint i = 0; i < vertexCount; ++i)
        verts.Add(vertices[i]);
    shapes_.Add(DebugShape{ std::move(verts), color });
}

}
