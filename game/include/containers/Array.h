#pragma once

#include "containers/Span.h"

#include "common/Types.h"
#include "common/hs_Assert.h"

#include <cstdlib>
#include <cstring>

namespace hs
{

//------------------------------------------------------------------------------
template<class T>
class Array
{
public:
    //------------------------------------------------------------------------------
    Array() = default;

    //------------------------------------------------------------------------------
    ~Array()
    {
        for (int i = 0; i < count_; ++i)
            items_[i].~T();
        count_ = 0;

        capacity_ = 0;
        free(items_);
    }

    //------------------------------------------------------------------------------
    Array(const Array<T>& other)
    {
        capacity_ = other.capacity_;
        count_ = other.count_;

        items_ = (T*)malloc(sizeof(T) * capacity_);
        for (int i = 0; i < count_; ++i)
        {
            items_[i] = other.items_[i];
        }
    }

    //------------------------------------------------------------------------------
    Array<T>& operator=(const Array<T>& other)
    {
        for (int i = 0; i < count_; ++i)
            items_[i].~T();
        free(items_);

        capacity_ = other.capacity_;
        count_ = other.count_;

        items_ = (T*)malloc(sizeof(T) * capacity_);
        for (int i = 0; i < count_; ++i)
        {
            items_[i] = other.items_[i];
        }

        return *this;
    }

    //------------------------------------------------------------------------------
    Array(Array<T>&& other)
    {
        capacity_ = other.capacity_;
        count_ = other.count_;
        items_ = other.items_;
        
        other.items_ = nullptr;
        other.capacity_ = 0;
        other.count_ = 0;
    }

    //------------------------------------------------------------------------------
    Array<T>& operator=(Array<T>&& other)
    {
        for (int i = 0; i < count_; ++i)
            items_[i].~T();
        free(items_);

        capacity_ = other.capacity_;
        count_ = other.count_;
        items_ = other.items_;
        
        other.items_ = nullptr;
        other.capacity_ = 0;
        other.count_ = 0;

        return *this;
    }

    //------------------------------------------------------------------------------
    uint64 Count() const
    {
        return count_;
    }

    //------------------------------------------------------------------------------
    bool IsEmpty() const
    {
        return count_ == 0;
    }

    //------------------------------------------------------------------------------
    const T& operator[](uint64 index) const
    {
        hs_assert(index < count_);
        return items_[index];
    }

    //------------------------------------------------------------------------------
    T& operator[](uint64 index)
    {
        hs_assert(index < count_);
        return items_[index];
    }

    //------------------------------------------------------------------------------
    void Add(const T& item)
    {
        // Check for aliasing
        hs_assert((&item < items_ || &item >= items_ + capacity_) && "Inserting item from array to itself is not handled");

        if (count_ >= capacity_)
        {
            auto oldCapacity = capacity_;
            capacity_ = ArrMax(capacity_ << 1, MIN_CAPACITY);
            
            T* newItems = (T*)malloc(sizeof(T) * capacity_);
            memcpy(newItems, items_, sizeof(T) * oldCapacity);
            free(items_);
            items_ = newItems;
        }

        new(items_ + count_) T(item);
        ++count_;
    }

    //------------------------------------------------------------------------------
    void Insert(uint64 index, const T& item)
    {
        hs_assert(index <= count_);
        // Check for aliasing
        hs_assert((&item < items_ || &item >= items_ + capacity_) && "Inserting item from array to itself is not handled");

        if (count_ >= capacity_)
        {
            auto oldCapacity = capacity_;
            capacity_ = ArrMax(capacity_ << 1, MIN_CAPACITY);
            
            auto newItems = (T*)malloc(sizeof(T) * capacity_);
            memcpy(newItems, items_, sizeof(T) * index);
            memcpy(&newItems[index + 1], &items_[index], (oldCapacity - index) * sizeof(T));

            free(items_);
            items_ = newItems;
        }
        else
        {
            // Move items by one to the right
            memmove(&items_[index + 1], &items_[index], (count_ - index) * sizeof(T));
        }

        new(items_ + index) T(item);
        ++count_;
    }

    //------------------------------------------------------------------------------
    void Remove(uint64 index)
    {
        hs_assert(index < count_);
        items_[index].~T();
        
        --count_;
        memmove(&items_[index], &items_[index + 1], (count_ - index) * sizeof(T));
    }

    //------------------------------------------------------------------------------
    void RemoveLast()
    {
        Remove(count_ - 1);
    }

    //------------------------------------------------------------------------------
    void Clear()
    {
        for (uint64 i = 0; i < count_; ++i)
        {
            items_[i].~T();
        }
        count_ = 0;
    }

    //------------------------------------------------------------------------------
    void Reserve(uint64 capacity)
    {
        if (capacity <= capacity_)
            return;

        auto oldCapacity = capacity_;
        capacity_ = ArrMax(capacity, MIN_CAPACITY);
            
        T* newItems = (T*)malloc(sizeof(T) * capacity_);
        memcpy(newItems, items_, sizeof(T) * oldCapacity);
        free(items_);
        items_ = newItems;
    }

    //------------------------------------------------------------------------------
    const T& First() const
    {
        hs_assert(count_);
        return items_[0];
    }

    //------------------------------------------------------------------------------
    T& First()
    {
        hs_assert(count_);
        return items_[0];
    }

    //------------------------------------------------------------------------------
    const T& Last() const
    {
        hs_assert(count_);
        return items_[count_ - 1];
    }

    //------------------------------------------------------------------------------
    T& Last()
    {
        hs_assert(count_);
        return items_[count_ - 1];
    }

    //------------------------------------------------------------------------------
    T* Data() const
    {
        return items_;
    }

private:
    static constexpr uint64 MIN_CAPACITY = 8;

    uint64 capacity_{};
    uint64 count_{};
    T* items_{};

    //------------------------------------------------------------------------------
    uint64 ArrMax(uint64 a, uint64 b)
    {
        return a > b ? a : b;
    }
};

//------------------------------------------------------------------------------
template<class T>
Span<T> MakeSpan(const Array<T>& array)
{
    return Span<T>(array.Data(), array.Count());
}

}
