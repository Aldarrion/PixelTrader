#pragma once

#include "common/Types.h"

namespace hs
{

//------------------------------------------------------------------------------
template<class KeyT>
struct FibonacciHash
{
    constexpr uint64 operator()(const KeyT key) const
    {
        return (key * 11400714819323198485llu) >> 61;
    }
};

//------------------------------------------------------------------------------
template<typename>
struct StrCmpEq
{
    constexpr bool operator()(const char* lhs, const char* rhs) const
    {
        return strcmp(lhs, rhs) == 0;
    }
};

//------------------------------------------------------------------------------
template<typename>
struct StrHash
{
    uint64 operator()(const char* key) const
    {
        uint64 hash = 9909453657034508789u;
        uint len = strlen(key);
        for (uint i = 0; i < len; ++i)
        {
            hash = hash * 1525334644490293591u + key[i];
        }
        return hash;
    }
};

}
