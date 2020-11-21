#pragma once
#include "Config.h"

#include "Common/hs_Assert.h"

#include <cstdint>

using uint8     = uint8_t;
using uint16    = uint16_t;
using uint      = uint32_t;
using uint64    = uint64_t;

using byte      = uint8;

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

#define hs_arr_len(arr) ::internal::ArrSizeInternal(arr)
//#define VKR_ARR_COUNT(arr) sizeof(arr)/sizeof(arr[0])

#define HS_FAILED(res) (((int)(res)) < 0)
#define HS_SUCCEEDED(res) (((int)(res)) >= 0)
#if HS_DEBUG
    #define HS_CHECK(res) hs_assert(HS_SUCCEEDED(res))
#else
    #define HS_CHECK(res) (void)res
#endif

