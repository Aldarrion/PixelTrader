#pragma once

#include "Config.h"

#include "Render/Material.h"

#include "Math/hs_Math.h"

#include "Containers/Array.h"

#include "Common/Enums.h"

namespace hs
{

//------------------------------------------------------------------------------
struct DebugShape
{
    Array<Vec3> vertices_;
    Color color_;
};

//------------------------------------------------------------------------------
class DebugShapeRenderer
{
public:
    RESULT Init();
    void Draw();

    void ClearShapes();
    void AddShape(Span<const Vec3> vertices, Color color);

private:
    DebugShapeMaterial debugShapeMat_;
    Array<DebugShape> shapes_;
};

}
