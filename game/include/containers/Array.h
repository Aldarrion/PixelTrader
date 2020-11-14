#pragma once

#include "containers/Span.h"

#include "common/Types.h"
#include "common/hs_Assert.h"

#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <initializer_list>

namespace hs
{

//------------------------------------------------------------------------------
template<class T>
class Array
{
public:
    using Iter_t = T*;
    using ConstIter_t = const Iter_t;

    //------------------------------------------------------------------------------
    static constexpr uint64 IndexBad()
    {
        return (uint64)-1;
    }

    //------------------------------------------------------------------------------
    Array() = default;

    //------------------------------------------------------------------------------
    Array(std::initializer_list<T> elements)
    {
        for (auto&& e : elements)
        {
            Add(std::forward<decltype(e)>(e));
        }
    }

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
    [[nodiscard]] uint64 Count() const
    {
        return count_;
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] bool IsEmpty() const
    {
        return count_ == 0;
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] const T& operator[](uint64 index) const
    {
        hs_assert(index < count_);
        return items_[index];
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] T& operator[](uint64 index)
    {
        hs_assert(index < count_);
        return items_[index];
    }

    //------------------------------------------------------------------------------
    template<class ...ArgsT>
    void EmplaceBack(ArgsT ...args)
    {
        // Check for aliasing
        if (count_ == capacity_)
        {
            capacity_ = ArrMax(capacity_ << 1, MIN_CAPACITY);

            T* newItems = (T*)malloc(sizeof(T) * capacity_);

            if (std::is_trivial_v<T>)
            {
                memcpy(newItems, items_, sizeof(T) * count_);
            }
            else
            {
                for (int i = 0; i < count_; ++i)
                {
                    new(newItems + i) T(std::move(items_[i]));
                    items_[i].~T();
                }
            }
            free(items_);
            items_ = newItems;
        }

        hs_assert(count_ < capacity_);

        new(items_ + count_) T(std::forward<ArgsT>(args)...);
        ++count_;
    }

    //------------------------------------------------------------------------------
    template<class ...ArgsT>
    void Emplace(uint64 index, ArgsT ...args)
    {
        hs_assert(index <= count_);
        if (index == count_)
        {
            EmplaceBack(std::forward<ArgsT>(args)...);
            return;
        }

        if (count_ == capacity_)
        {
            capacity_ = ArrMax(capacity_ << 1, MIN_CAPACITY);

            auto newItems = (T*)malloc(sizeof(T) * capacity_);
            if (std::is_trivial_v<T>)
            {
                memcpy(newItems, items_, sizeof(T) * index);
                memcpy(&newItems[index + 1], &items_[index], (count_ - index) * sizeof(T));
            }
            else
            {
                for (int i = 0; i < index; ++i)
                {
                    new(newItems + i) T(std::move(items_[i]));
                    items_[i].~T();
                }
                for (int i = index; i < count_; ++i)
                {
                    new(newItems + i + 1) T(std::move(items_[i]));
                    items_[i].~T();
                }
            }

            free(items_);
            items_ = newItems;
            new(items_ + index) T(std::forward<ArgsT>(args)...);
        }
        else
        {
            // Move items by one to the right
            if (std::is_trivial_v<T>)
            {
                memmove(&items_[index + 1], &items_[index], (count_ - index) * sizeof(T));
            }
            else
            {
                // count_ is at least 1, otherwise there is early exit
                // New last place is not initialized item, move construct there
                new(items_ + count_) T(std::move(items_[count_ - 1]));
                // Other items can be move assigned
                for (T* item = items_ + count_- 1; item != items_ + index; --item)
                    *item = std::move(*(item - 1));
            }

            items_[index] = T(std::forward<ArgsT>(args)...);
        }

        hs_assert(count_ < capacity_);
        ++count_;
    }

    //------------------------------------------------------------------------------
    void Add(const T& item)
    {
        // Check for aliasing
        hs_assert((&item < items_ || &item >= items_ + capacity_) && "Inserting item from array to itself is not handled");
        EmplaceBack(item);
    }

    //------------------------------------------------------------------------------
    void Add(T&& item)
    {
        // Check for aliasing
        hs_assert((&item < items_ || &item >= items_ + capacity_) && "Inserting item from array to itself is not handled");
        EmplaceBack(std::move(item));
    }

    //------------------------------------------------------------------------------
    void Insert(uint64 index, const T& item)
    {
        // Check for aliasing
        hs_assert((&item < items_ || &item >= items_ + capacity_) && "Inserting item from array to itself is not handled");
        Emplace(index, item);
    }

    //------------------------------------------------------------------------------
    void Insert(uint64 index, T&& item)
    {
        // Check for aliasing
        hs_assert((&item < items_ || &item >= items_ + capacity_) && "Inserting item from array to itself is not handled");
        Emplace(index, std::move(item));
    }

    //------------------------------------------------------------------------------
    void Remove(uint64 index)
    {
        hs_assert(index < count_);

        if (std::is_trivial_v<T>)
        {
            memmove(&items_[index], &items_[index + 1], (count_ - index) * sizeof(T));
        }
        else
        {
            for (T* item = items_ + index; item != items_ + count_ - 1; ++item)
                *item = std::move(item[1]);
            items_[count_ - 1].~T();
        }

        --count_;
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
        if (std::is_trivial_v<T>)
        {
            memcpy(newItems, items_, sizeof(T) * oldCapacity);
        }
        else
        {
            for (int i = 0; i < count_; ++i)
            {
                new(newItems + i) T(std::move(items_[i]));
                items_[i].~T();
            }
        }

        free(items_);
        items_ = newItems;
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] const T& First() const
    {
        hs_assert(count_);
        return items_[0];
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] T& First()
    {
        hs_assert(count_);
        return items_[0];
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] const T& Last() const
    {
        hs_assert(count_);
        return items_[count_ - 1];
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] T& Last()
    {
        hs_assert(count_);
        return items_[count_ - 1];
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] T* Data() const
    {
        return items_;
    }

    //------------------------------------------------------------------------------
    uint64 IndexOf(const T& item) const
    {
        for (int i = 0; i < count_; ++i)
        {
            if (items_[i] == item)
                return i;
        }

        return IndexBad();
    }

    #pragma region Iterators
    //------------------------------------------------------------------------------
    // Iterators
    //------------------------------------------------------------------------------
    [[nodiscard]] ConstIter_t cbegin() const
    {
        return items_;
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] ConstIter_t begin() const
    {
        return cbegin();
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] Iter_t begin()
    {
        return items_;
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] ConstIter_t cend() const
    {
        return items_ + count_;
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] ConstIter_t end() const
    {
        return cend();
    }

    //------------------------------------------------------------------------------
    [[nodiscard]] Iter_t end()
    {
        return items_ + count_;
    }
    #pragma endregion

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
