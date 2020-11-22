#include "Game/DrawCanvas.h"

#include "Render/Render.h"
#include "Render/ShaderManager.h"
#include "Render/VertexTypes.h"
#include "Render/VertexBuffer.h"
#include "Input/Input.h"

#include "Common/Logging.h"

namespace hs
{

//------------------------------------------------------------------------------
static constexpr uint MAX_LINE_VERTS = 1024;

//------------------------------------------------------------------------------
DrawCanvas::~DrawCanvas()
{
    linesBuffer_->Free();
}

//------------------------------------------------------------------------------
RESULT DrawCanvas::Init()
{
    lineVert_ = g_Render->GetShaderManager()->GetOrCreateShader("Shape_vs.hlsl");
    lineFrag_ = g_Render->GetShaderManager()->GetOrCreateShader("Shape_fs.hlsl");

    if (!lineVert_ || !lineVert_)
        return R_FAIL;

    lineVertType_ = ShapeVertexLayout();

    linesBuffer_ = MakeUnique<VertexBuffer>(MAX_LINE_VERTS * sizeof(ShapeVertex));
    if (FAILED(linesBuffer_->Init()))
        return R_FAIL;

    drawMode_ = DrawMode::CatmullRom;

    return R_OK;
}

//------------------------------------------------------------------------------
void DrawCanvas::Draw()
{
    // Update
    if (g_Input->IsButtonDown(BTN_LEFT))
    {
        if (points_.Count() == MAX_LINE_VERTS)
            points_.Clear();

        Vec2 mousePos = g_Input->GetMousePos();
        points_.Add(mousePos);
    }

    if (points_.Count() < 2)
        return;

    // Copy to VB
    uint vertsToDraw = 0;

    auto verts = (ShapeVertex*)linesBuffer_->Map();
    if (drawMode_ == DrawMode::Lines)
    {
        for (int i = 0; i < points_.Count(); ++i)
        {
            verts[i].position_ = Vec4{ points_[i].x, points_[i].y, 0, 0 };
            verts[i].color_ = 0xff2222dd;
        }
        vertsToDraw = points_.Count();
    }
    else
    {
        hs_assert(drawMode_ == DrawMode::CatmullRom);

        const uint tesselLevel = 5;

        for (int i = 0; i < points_.Count() - 1; ++i)
        {
            Vec2 p0 = points_[i];
            Vec2 p1 = points_[i + 1];
            Vec2 m0 = i == 0 ? Vec2{} : p1 - points_[i - 1];
            Vec2 m1 = i == points_.Count() - 2 ? Vec2{} : points_[i + 2] - p0;

            for (int j = 0; j <= tesselLevel; ++j)
            {
                float t = (float)j / tesselLevel;
                float t2 = t * t;
                float t3 = t2 * t;
                Vec2 pos =
                    (2 * t3 - 3 * t2 + 1) * p0
                    + (t3 - 2 * t2 + t) * m0
                    + (-2 * t3 + 3 * t2) * p1
                    + (t3 - t2) * m1;

                verts[i * tesselLevel + j].position_ = Vec4{ pos.x, pos.y, 0, 0 };
                verts[i * tesselLevel + j].color_ = 0xffff22dd;
            }
        }

        vertsToDraw = (points_.Count() - 1) * tesselLevel + 1;
    }
    linesBuffer_->Unmap();

    // Render
    g_Render->SetVertexBuffer(0, VertexBufferEntry{ linesBuffer_->GetBuffer(), 0 });
    g_Render->SetShader<PS_VERT>(lineVert_);
    g_Render->SetShader<PS_FRAG>(lineFrag_);
    g_Render->SetPrimitiveTopology(VkrPrimitiveTopology::LINE_STRIP);

    g_Render->SetVertexLayout(0, lineVertType_);

    g_Render->Draw(vertsToDraw, 0);
}

}
