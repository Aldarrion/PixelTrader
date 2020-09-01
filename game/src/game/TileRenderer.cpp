#include "game/TileRenderer.h"

#include "common/Logging.h"

namespace hs
{

//------------------------------------------------------------------------------
RESULT TileRenderer::Init()
{
    tileMaterial_ = new TileMaterial();

    if (HS_FAILED(tileMaterial_->Init()))
    {
        Log(LogLevel::Error, "Failed to init material");
        return R_FAIL;
    }

    return R_OK;
}

//------------------------------------------------------------------------------
void TileRenderer::Draw()
{
    constexpr float uvSize = 16.0f / (3 * 16.0f);
    TileDrawData data{};
    data.uvBox_ = Vec4{ 0, 0, uvSize, uvSize };
    tileMaterial_->DrawTile(data);

    data.uvBox_ = Vec4{ uvSize, 0, uvSize, uvSize };
    data.pos_ = Vec2{ 16, 0 };
    tileMaterial_->DrawTile(data);
    data.pos_ = Vec2{ 32, 0 };
    tileMaterial_->DrawTile(data);
    data.pos_ = Vec2{ 48, 0 };
    tileMaterial_->DrawTile(data);
    data.pos_ = Vec2{ 64, 0 };
    tileMaterial_->DrawTile(data);
    data.pos_ = Vec2{ 80, 0 };
    tileMaterial_->DrawTile(data);

    data.uvBox_ = Vec4{ uvSize, uvSize, uvSize, uvSize };
    data.pos_ = Vec2{ 16, -16 };
    tileMaterial_->DrawTile(data);
    data.pos_ = Vec2{ 32, -16 };
    tileMaterial_->DrawTile(data);
    data.pos_ = Vec2{ 48, -16 };
    tileMaterial_->DrawTile(data);
    data.pos_ = Vec2{ 64, -16 };
    tileMaterial_->DrawTile(data);
    data.pos_ = Vec2{ 80, -16 };
    tileMaterial_->DrawTile(data);

    data.uvBox_ = Vec4{ 2 * uvSize, 0, uvSize, uvSize };
    data.pos_ = Vec2{ 96, 0 };
    tileMaterial_->DrawTile(data);
}

}
