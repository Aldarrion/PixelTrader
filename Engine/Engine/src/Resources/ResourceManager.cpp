#include "Resources/ResourceManager.h"

#include "Render/Texture.h"

#include "Common/Types.h"

namespace hs
{

//------------------------------------------------------------------------------
ResourceManager* g_ResourceManager{};

//------------------------------------------------------------------------------
RESULT CreateResourceManager()
{
    g_ResourceManager = new ResourceManager();

    return R_OK;
}

//------------------------------------------------------------------------------
void DestroyResourceManager()
{
    if (!g_ResourceManager)
        return;

    g_ResourceManager->Free();
    delete g_ResourceManager;
}

//------------------------------------------------------------------------------
RESULT ResourceManager::InitWin32()
{
    return R_OK;
}

//------------------------------------------------------------------------------
void ResourceManager::Free()
{
    for (auto it : textures_)
    {
        it.second->Free();
        delete it.second;
    }

    textures_.clear();
}

//------------------------------------------------------------------------------
RESULT ResourceManager::LoadTexture2D(const char* path, Texture** texture)
{
    Texture*& it = textures_[path];
    if (it)
    {
        *texture = it;
        return R_OK;
    }

    if (HS_FAILED(Texture::CreateTex2D(path, path, &it)))
        return R_FAIL;

    *texture = it;
    return R_OK;
}

}
