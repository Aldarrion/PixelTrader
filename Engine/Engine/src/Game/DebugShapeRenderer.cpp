#include "Game/DebugShapeRenderer.h"

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
        debugShapeMat_.DrawShape(MakeSpan(shapes_[i].vertices_), shapes_[i].color_);
    }
}

//------------------------------------------------------------------------------
void DebugShapeRenderer::ClearShapes()
{
    shapes_.Clear();
}

//------------------------------------------------------------------------------
void DebugShapeRenderer::AddShape(Span<const Vec3> vertices, Color color)
{
    Array<Vec3> verts;
    verts.Reserve(vertices.Count());

    for (uint i = 0; i < vertices.Count(); ++i)
        verts.Add(vertices[i]);
    shapes_.Add(DebugShape{ std::move(verts), color });
}

}
