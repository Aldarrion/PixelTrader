#pragma once
#include "Config.h"

#include <cstdint>

using uint8     = uint8_t;
using uint16    = uint16_t;
using uint      = uint32_t;
using uint64    = uint64_t;

namespace hs
{
class Texture;
class Shader;
class VertexBuffer;
class DynamicUniformBuffer;
}

namespace internal
{

template<class T, uint N>
constexpr uint ArrSizeInternal(T(&)[N])
{
    return N;
}

}

#define hs_arr_len(arr) internal::ArrSizeInternal(arr)
//#define VKR_ARR_COUNT(arr) sizeof(arr)/sizeof(arr[0])

#define HS_FAILED(res) (((int)(res)) < 0)
#define HS_SUCCEEDED(res) (((int)(res)) >= 0)

//------------------------------------------------------------------------------
// Hashing
template<typename>
struct StrCmpEq
{
    constexpr bool operator()(const char* lhs, const char* rhs) const
    {
        return strcmp(lhs, rhs) == 0;
    }
};

template<typename>
struct StrHash
{
    uint64 operator()(const char* key) const
    {
        uint64 hash = 9909453657034508789;
        uint len = strlen(key);
        for (uint i = 0; i < len; ++i)
        {
            hash = hash * 1525334644490293591 + key[i];
        }
        return hash;
    }
};

