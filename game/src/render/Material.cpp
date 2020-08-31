#include "render/Material.h"

#include "render/Render.h"
#include "render/Texture.h"
#include "render/ShaderManager.h"
#include "render/VertexBuffer.h"
#include "render/DynamicUniformBuffer.h"
#include "render/VertexTypes.h"
#include "input/Input.h"

#include "render/hs_Image.h"
#include <string>

namespace hs
{

//------------------------------------------------------------------------------
RESULT TexturedTriangleMaterial::Init()
{
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/grass_tile.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        texture_ = new Texture(VK_FORMAT_R8G8B8A8_UNORM, VkExtent3D{ (uint)texWidth, (uint)texHeight, 1 }, Texture::Type::TEX_2D);

        auto texAllocRes = texture_->Allocate((void**)&pixels, "GrassTile");
        stbi_image_free(pixels);
    
        if (FAILED(texAllocRes))
            return R_FAIL; // TODO release resources
    }

    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/tree.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        textureTree_ = new Texture(VK_FORMAT_R8G8B8A8_UNORM, VkExtent3D{ (uint)texWidth, (uint)texHeight, 1 }, Texture::Type::TEX_2D);

        auto texAllocRes = textureTree_->Allocate((void**)&pixels, "Tree");
        stbi_image_free(pixels);
    
        if (FAILED(texAllocRes))
            return R_FAIL; // TODO release resources
    }

    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/box.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        textureBox_ = new Texture(VK_FORMAT_R8G8B8A8_UNORM, VkExtent3D{ (uint)texWidth, (uint)texHeight, 1 }, Texture::Type::TEX_2D);

        auto texAllocRes = textureBox_->Allocate((void**)&pixels, "Box");
        stbi_image_free(pixels);
    
        if (FAILED(texAllocRes))
            return R_FAIL; // TODO release resources
    }

    triangleVert_ = g_Render->GetShaderManager()->GetOrCreateShader("Triangle_vs.hlsl");
    triangleFrag_ = g_Render->GetShaderManager()->GetOrCreateShader("Triangle_fs.hlsl");

    if (!triangleVert_ || !triangleFrag_)
        return R_FAIL;

    return R_OK;
}

//------------------------------------------------------------------------------
void TexturedTriangleMaterial::Draw()
{
    g_Render->SetShader<PS_VERT>(triangleVert_);
    g_Render->SetShader<PS_FRAG>(triangleFrag_);
    g_Render->SetTexture(0, texture_);
    g_Render->SetTexture(1, textureBox_);
    g_Render->SetTexture(2, textureTree_);
    g_Render->Draw(3, 0);
}


//------------------------------------------------------------------------------
RESULT ShapeMaterial::Init()
{
    shapeVert_ = g_Render->GetShaderManager()->GetOrCreateShader("Shape_vs.hlsl");
    shapeFrag_ = g_Render->GetShaderManager()->GetOrCreateShader("Shape_fs.hlsl");

    if (!shapeVert_ || !shapeFrag_)
        return R_FAIL;

    vertexBuffer_ = new VertexBuffer(1024);
    if (FAILED(vertexBuffer_->Init()))
        return R_FAIL;

    auto mapped = (ShapeVertex*) vertexBuffer_->Map();

    mapped[0].position_.v[0] = 100;
    mapped[0].position_.v[1] = 100;

    mapped[1].position_.v[0] = 400;
    mapped[1].position_.v[1] = 100;

    mapped[2].position_.v[0] = 200;
    mapped[2].position_.v[1] = 400;

    mapped[0].color_ = 
    mapped[1].color_ = 
    mapped[2].color_ = 0xffff3333;

    vertexBuffer_->Unmap();

    vertexLayout_ = ShapeVertexLayout();

    return R_OK;
}

//------------------------------------------------------------------------------
void ShapeMaterial::Draw()
{
    g_Render->SetVertexLayout(0, vertexLayout_);

    g_Render->SetShader<PS_VERT>(shapeVert_);
    g_Render->SetShader<PS_FRAG>(shapeFrag_);

    g_Render->SetVertexBuffer(0, vertexBuffer_, 0);

    g_Render->Draw(3, 0);
}

//------------------------------------------------------------------------------
RESULT PhongMaterial::Init()
{
    phongVert_ = g_Render->GetShaderManager()->GetOrCreateShader("Phong_vs.hlsl");
    phongFrag_ = g_Render->GetShaderManager()->GetOrCreateShader("Phong_fs.hlsl");

    if (!phongVert_ || !phongFrag_)
        return R_FAIL;

    return R_OK;
}

//------------------------------------------------------------------------------
void PhongMaterial::Draw()
{
    // TODO move scene CB elsewhere
    struct SceneData
    {
        Mat44   Projection;
        Vec4    ViewPos;
    };

    void* mapped;
    DynamicUBOEntry constBuffer = g_Render->GetUBOCache()->BeginAlloc(sizeof(SceneData), &mapped);
    auto ubo = (SceneData*)mapped;

    ubo->Projection = g_Render->GetCamera().ToCamera() * g_Render->GetCamera().ToProjection();
    ubo->ViewPos    = g_Render->GetCamera().Position().ToVec4();
    
    g_Render->GetUBOCache()->EndAlloc();

    g_Render->SetDynamicUbo(1, &constBuffer);


    // This material setup
    g_Render->SetShader<PS_VERT>(phongVert_);
    g_Render->SetShader<PS_FRAG>(phongFrag_);

    g_Render->Draw(3 * 12, 0);
}

//------------------------------------------------------------------------------
RESULT SkyboxMaterial::Init()
{
    {
        stbi_uc* pixels[6]{};

        int texWidth, texHeight, texChannels;
        pixels[0] = stbi_load("textures/skybox/right.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        pixels[1] = stbi_load("textures/skybox/left.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        pixels[2] = stbi_load("textures/skybox/top.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        pixels[3] = stbi_load("textures/skybox/bottom.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        pixels[4] = stbi_load("textures/skybox/front.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        pixels[5] = stbi_load("textures/skybox/back.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        skyboxCubemap_ = new Texture(VK_FORMAT_R8G8B8A8_UNORM, VkExtent3D{ (uint)texWidth, (uint)texHeight, 1 }, Texture::Type::TEX_CUBE);

        auto texAllocRes = skyboxCubemap_->Allocate((void**)pixels, "Skybox");
        for (uint i = 0; i < hs_arr_len(pixels); ++i)
            stbi_image_free(pixels[i]);
    
        if (FAILED(texAllocRes))
            return R_FAIL; // TODO release resources
    }

    skyboxVert_ = g_Render->GetShaderManager()->GetOrCreateShader("Skybox_vs.hlsl");
    if (!skyboxVert_)
        return R_FAIL;

    skyboxFrag_ = g_Render->GetShaderManager()->GetOrCreateShader("Skybox_fs.hlsl");
    if (!skyboxFrag_)
        return R_FAIL;

    return R_OK;
}

//------------------------------------------------------------------------------
void SkyboxMaterial::Draw()
{
    // TODO move scene CB elsewhere
    struct SceneData
    {
        Mat44   Projection;
        Vec4    ViewPos;
    };

    void* mapped;
    DynamicUBOEntry constBuffer = g_Render->GetUBOCache()->BeginAlloc(sizeof(SceneData), &mapped);
    auto ubo = (SceneData*)mapped;

    Mat44 camMat = g_Render->GetCamera().ToCamera();
    camMat.SetPosition(Vec3{});

    Mat44 projMat = camMat * g_Render->GetCamera().ToProjection();
    ubo->Projection = projMat;

    g_Render->GetUBOCache()->EndAlloc();

    g_Render->SetDynamicUbo(1, &constBuffer);


    // This material setup
    g_Render->SetShader<PS_VERT>(skyboxVert_);
    g_Render->SetShader<PS_FRAG>(skyboxFrag_);

    // Bind skybox texture
    g_Render->SetTexture(0, skyboxCubemap_);

    g_Render->SetDepthState(false);

    g_Render->Draw(3 * 12, 0);
}

}
