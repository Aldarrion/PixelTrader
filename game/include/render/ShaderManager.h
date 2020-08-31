#pragma once

#include "Config.h"
#include "Enums.h"

#include "Array.h"

#include "VkTypes.h"
#include <unordered_map>

struct shaderc_compiler;
struct shaderc_compile_options;


namespace hs
{

class Shader;

//------------------------------------------------------------------------------
class ShaderManager
{
public:
    RESULT Init();

    Shader* GetOrCreateShader(const char* name);
    RESULT ReloadShaders();

private:
    std::unordered_map<const char*, Shader*, StrHash<const char*>, StrCmpEq<const char*>> cache_;
    
    Array<VkShaderModule>       toDestroy_;
    shaderc_compiler*           shadercCompiler_{};
    shaderc_compile_options*    opts_{};

    uint16 shaderId_[PS_COUNT]{};

    RESULT CompileShader(const char* file, PipelineStage type, Shader& shader) const;
    RESULT CreateShader(const char* name, Shader* shader);
};

}
