#pragma once

#include "array_ptr.h"

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <utility>
#include <iterator>
#include <stdexcept>
#include <type_traits>

struct ReserveProxyObj {
    explicit ReserveProxyObj(size_t capacity_to_reserve)
        : capacity(capacity_to_reserve) {}
    size_t capacity;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size)
        : items_(size), size_(size), capacity_(size)
    {
        std::uninitialized_value_construct_n(items_.Get(), size_);
    }

    SimpleVector(size_t size, const Type& value)
        : items_(size), size_(size), capacity_(size)
    {
        std::uninitialized_fill_n(items_.Get(), size_, value);
    }

    explicit SimpleVector(ReserveProxyObj proxy)
        : items_(proxy.capacity), size_(0), capacity_(proxy.capacity) {}

    SimpleVector(std::initializer_list<Type> init)
        : items_(init.size()), size_(init.size()), capacity_(init.size())
    {
        std::uninitialized_copy(init.begin(), init.end(), items_.Get());
    }

    SimpleVector(const SimpleVector& other)
        : items_(other.size_), size_(other.size_), capacity_(other.size_)
    {
        std::uninitialized_copy(other.begin(), other.end(), items_.Get());
    }

    SimpleVector(SimpleVector&& other) noexcept
        : items_(std::move(other.items_)), size_(other.size_), capacity_(other.capacity_)
    {
        other.size_ = 0;
        other.capacity_ = 0;
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector tmp(rhs);
            swap(tmp); // swap
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
    if (this != &rhs) {
        swap(rhs); // swap
    }
    return *this;
}

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Out of range");
        }
        return items_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Out of range");
        }
        return items_[index];
    }

    void Clear() noexcept {
        std::destroy_n(items_.Get(), size_);
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(items_.Get() + new_size, size_ - new_size);
        } else if (new_size > size_) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(items_.Get() + size_, new_size - size_);
        }
        size_ = new_size;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }
        ArrayPtr<Type> new_items(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<Type> || !std::is_copy_constructible_v<Type>) {
            std::uninitialized_move(items_.Get(), items_.Get() + size_, new_items.Get());
        } else {
            std::uninitialized_copy(items_.Get(), items_.Get() + size_, new_items.Get());
        }
        std::destroy_n(items_.Get(), size_);
        items_ = std::move(new_items);
        capacity_ = new_capacity;
    }

    void PushBack(const Type& item) {
        if (size_ == capacity_) {
            Reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        new (items_.Get() + size_) Type(item);
        ++size_;
    }

    void PushBack(Type&& item) {
        if (size_ == capacity_) {
            Reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        new (items_.Get() + size_) Type(std::move(item));
        ++size_;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        return InsertImpl(pos, value);
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        return InsertImpl(pos, std::move(value));
    }

    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        size_t idx = pos - begin();
        std::destroy_at(items_.Get() + idx);
        std::move(items_.Get() + idx + 1, items_.Get() + size_, items_.Get() + idx); // std::move
        --size_;
        return items_.Get() + idx;
    }

    void PopBack() noexcept {
        assert(size_ > 0);
        std::destroy_at(items_.Get() + size_ - 1);
        --size_;
    }

    Iterator begin() noexcept { return items_.Get(); }
    Iterator end() noexcept { return items_.Get() + size_; }
    ConstIterator begin() const noexcept { return items_.Get(); }
    ConstIterator end() const noexcept { return items_.Get() + size_; }
    ConstIterator cbegin() const noexcept { return items_.Get(); }
    ConstIterator cend() const noexcept { return items_.Get() + size_; }

    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    friend bool operator==(const SimpleVector& lhs, const SimpleVector& rhs) {
        return lhs.size_ == rhs.size_ && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    friend bool operator!=(const SimpleVector& lhs, const SimpleVector& rhs) {
        return !(lhs == rhs);
    }
    friend bool operator<(const SimpleVector& lhs, const SimpleVector& rhs) {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
    friend bool operator>(const SimpleVector& lhs, const SimpleVector& rhs) {
        return rhs < lhs;
    }
    friend bool operator<=(const SimpleVector& lhs, const SimpleVector& rhs) {
        return !(rhs < lhs);
    }
    friend bool operator>=(const SimpleVector& lhs, const SimpleVector& rhs) {
        return !(lhs < rhs);
    }

private:
    template <typename ValueType>
    Iterator InsertImpl(ConstIterator pos, ValueType&& value) {
        assert(pos >= begin() && pos <= end());
        size_t idx = pos - begin();
        if (size_ == capacity_) {
            size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> new_items(new_capacity);
            Iterator dest = new_items.Get();
            try {
                if constexpr (std::is_nothrow_move_constructible_v<Type> || !std::is_copy_constructible_v<Type>) {
                    std::uninitialized_move(items_.Get(), items_.Get() + idx, dest);
                } else {
                    std::uninitialized_copy(items_.Get(), items_.Get() + idx, dest);
                }
                new (dest + idx) Type(std::forward<ValueType>(value));
                if constexpr (std::is_nothrow_move_constructible_v<Type> || !std::is_copy_constructible_v<Type>) {
                    std::uninitialized_move(items_.Get() + idx, items_.Get() + size_, dest + idx + 1);
                } else {
                    std::uninitialized_copy(items_.Get() + idx, items_.Get() + size_, dest + idx + 1);
                }
            } catch (...) {
                std::destroy_n(dest, idx);
                throw;
            }
            std::destroy_n(items_.Get(), size_);
            items_ = std::move(new_items);
            capacity_ = new_capacity;
        } else {
            if (idx < size_) {
                new (items_.Get() + size_) Type(std::move(items_[size_ - 1]));
                std::move_backward(items_.Get() + idx, items_.Get() + size_ - 1, items_.Get() + size_);
                items_[idx] = std::forward<ValueType>(value);
            } else {
                new (items_.Get() + idx) Type(std::forward<ValueType>(value));
            }
        }
        ++size_;
        return items_.Get() + idx;
    }

    ArrayPtr<Type> items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};