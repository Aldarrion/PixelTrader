#pragma once

#include "Common/hs_Assert.h"

#include <cstddef>

namespace hs
{

//------------------------------------------------------------------------------
template<class T>
class UniquePtr
{
public:
    using Pointer_t = T*;

    //------------------------------------------------------------------------------
    UniquePtr() : ptr_(nullptr) {}

    //------------------------------------------------------------------------------
    explicit UniquePtr(T* p)
        : ptr_(p)
    {}

    //------------------------------------------------------------------------------
    UniquePtr(const UniquePtr<T>&) = delete;
    UniquePtr<T>& operator=(const UniquePtr<T>&) = delete;

    //------------------------------------------------------------------------------
    UniquePtr(UniquePtr<T>&& other)
    {
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
    }

    //------------------------------------------------------------------------------
    UniquePtr<T>& operator=(UniquePtr<T>&& other)
    {
        // First save to temp to achieve branchless self-assignment protection
        auto otherPtr = other.ptr_;
        other.ptr_ = nullptr;

        delete ptr_;
        ptr_ = otherPtr;

        return *this;
    }

    //------------------------------------------------------------------------------
    ~UniquePtr()
    {
        delete ptr_;
    }

    //------------------------------------------------------------------------------
    T* Get() const
    {
        return ptr_;
    }

    //------------------------------------------------------------------------------
    T* Release()
    {
        auto p = ptr_;
        ptr_ = nullptr;
        return p;
    }

    //------------------------------------------------------------------------------
    void Reset(T* newPtr = nullptr)
    {
        hs_assert((!ptr_ || newPtr != ptr_) && "UniquePtr Reset self-assignment");

        delete ptr_;
        ptr_ = newPtr;
    }

    //------------------------------------------------------------------------------
    T& operator*() const
    {
        hs_assert(ptr_);
        return *ptr_;
    }

    //------------------------------------------------------------------------------
    T* operator->() const
    {
        hs_assert(ptr_);
        return ptr_;
    }

    //------------------------------------------------------------------------------
    explicit operator bool() const
    {
        return ptr_ != nullptr;
    }

private:
    T* ptr_;
};

//------------------------------------------------------------------------------
template<class T, class... TArgs>
UniquePtr<T> MakeUnique(TArgs... args)
{
    return UniquePtr<T>(new T(args...));
}


// nullptr comparison
//------------------------------------------------------------------------------
template<class T>
bool operator==(const UniquePtr<T>& p, std::nullptr_t)
{
    return p.Get() == nullptr;
}
//------------------------------------------------------------------------------
template<class T>
bool operator!=(const UniquePtr<T>& p, std::nullptr_t)
{
    return !(p == nullptr);
}
//------------------------------------------------------------------------------
template<class T>
bool operator==(std::nullptr_t, const UniquePtr<T>& p)
{
    return p == nullptr;
}
//------------------------------------------------------------------------------
template<class T>
bool operator!=(std::nullptr_t, const UniquePtr<T>& p)
{
    return !(p == nullptr);
}

// Unique to unique comparison
//------------------------------------------------------------------------------
template<class T1, class T2>
bool operator==(const UniquePtr<T1>& a, const UniquePtr<T2>& b)
{
    return a.Get() == b.Get();
}
//------------------------------------------------------------------------------
template<class T1, class T2>
bool operator!=(const UniquePtr<T1>& a, const UniquePtr<T2>& b)
{
    return a.Get() != b.Get();
}

// Unique to unique address ordering
//------------------------------------------------------------------------------
template<class T1, class T2>
bool operator<(const UniquePtr<T1>& a, const UniquePtr<T2>& b)
{
    return static_cast<void*>(a.Get()) < static_cast<void*>(b.Get());
}
//------------------------------------------------------------------------------
template<class T1, class T2>
bool operator>(const UniquePtr<T1>& a, const UniquePtr<T2>& b)
{
    return b < a;
}
//------------------------------------------------------------------------------
template<class T1, class T2>
bool operator>=(const UniquePtr<T1>& a, const UniquePtr<T2>& b)
{
    return !(a < b);
}
//------------------------------------------------------------------------------
template<class T1, class T2>
bool operator<=(const UniquePtr<T1>& a, const UniquePtr<T2>& b)
{
    return !(b > a);
}

}
