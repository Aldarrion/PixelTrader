#pragma once

#include "common/Types.h"
#include "common/hs_Assert.h"

namespace hs
{

//------------------------------------------------------------------------------
template<class T>
class Span
{
public:
    //------------------------------------------------------------------------------
    constexpr Span() = default;

    //------------------------------------------------------------------------------
    constexpr Span(T* items, uint64 count)
        : items_(items)
        , count_(count)
    {}

    //------------------------------------------------------------------------------
    template<uint64 N>
    explicit constexpr Span(T (&array)[N])
        : items_(array)
        , count_(N)
    {}

    //------------------------------------------------------------------------------
    constexpr T* Data() const
    {
        return items_;
    }

    //------------------------------------------------------------------------------
    constexpr uint64 Count() const
    {
        return count_;
    }

    //------------------------------------------------------------------------------
    constexpr bool IsEmpty() const
    {
        return count_ == 0;
    }

    //------------------------------------------------------------------------------
    constexpr T& operator[](uint64 index) const
    {
        hs_assert(index < count_);
        return items_[index];
    }

    //------------------------------------------------------------------------------
    operator Span<const T>() const
    {
        return Span<const T>(items_, count_);
    }

private:
    T* items_{};
    uint64 count_{};
};

//------------------------------------------------------------------------------
template<class T>
constexpr Span<T> MakeSpan(T* items, uint64 count)
{
    return Span<T>(items, count);
}

//------------------------------------------------------------------------------
template<class T, uint64 N>
constexpr Span<T> MakeSpan(T (&array)[N])
{
    return Span<T>(array);
}

}
