#pragma once

#include "Config.h"

#include "containers/Array.h"

#include "math/hs_Math.h"

#include "common/Pointers.h"
#include "common/Enums.h"
#include "common/Types.h"

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
    ~DrawCanvas();

    RESULT Init();
    void Draw();

private:
    UniquePtr<VertexBuffer> linesBuffer_{};

    Shader*         lineVert_{};
    Shader*         lineFrag_{};
    uint            lineVertType_{};

    Array<Vec2>     points_;

    DrawMode        drawMode_{};
};

}
