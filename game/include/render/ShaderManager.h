#pragma once

#include "Config.h"
#include "common/Enums.h"

#include "containers/Array.h"
#include "containers/Hash.h"

#include "render/VkTypes.h"
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
    ~ShaderManager();

    Shader* GetOrCreateShader(const char* name);
    RESULT ReloadShaders();

private:
    // TODO use custom hashmap
    std::unordered_map<const char*, Shader*, StrHash<const char*>, StrCmpEq<const char*>> cache_;
    
    Array<VkShaderModule>       toDestroy_;
    shaderc_compiler*           shadercCompiler_{};
    shaderc_compile_options*    opts_{};

    uint16 shaderId_[PS_COUNT]{};

    RESULT CompileShader(const char* file, PipelineStage type, Shader& shader) const;
    RESULT CreateShader(const char* name, Shader* shader);
};

}
