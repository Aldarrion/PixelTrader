#pragma once

#include "Config.h"
#include "common/Enums.h"
#include "common/Types.h"
#include "containers/Array.h"
#include "math/hs_Math.h"

namespace hs
{

//------------------------------------------------------------------------------
enum class DrawMode
{
    Lines,
    CatmullRom
};

//------------------------------------------------------------------------------
class DrawCanvas
{
public:
    RESULT Init();
    void Draw();

private:
    VertexBuffer*   linesBuffer_{};
    Shader*         lineVert_{};
    Shader*         lineFrag_{};
    uint            lineVertType_{};

    Array<Vec2>     points_;

    DrawMode        drawMode_{};
};

}
