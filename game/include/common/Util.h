#pragma once

#include "Config.h"
#include <malloc.h>

#define HS_ALLOCA(Type, count) (Type*)_alloca(count * sizeof(Type))

namespace hs
{

//------------------------------------------------------------------------------
template<class T>
using RemoveCvRef_t = std::remove_cv_t<std::remove_reference_t<T>>;

//------------------------------------------------------------------------------
template<class T>
void Swap(T& a, T& b)
{
    auto tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}

}
