#pragma once

#include "Config.h"

#include "Containers/Array.h"

#include "Math/hs_Math.h"

#include "Common/Pointers.h"
#include "Common/Enums.h"
#include "Common/Types.h"

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
