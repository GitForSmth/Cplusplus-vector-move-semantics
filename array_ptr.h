#pragma once

#include <cassert>
#include <utility>
#include <algorithm> 
#include <utility>    

template <typename Type>
class ArrayPtr {
public:
    ArrayPtr() noexcept = default;

    explicit ArrayPtr(size_t size) 
        : raw_ptr_(size ? new Type[size] : nullptr) 
    {}

    ArrayPtr(const ArrayPtr&) = delete;
    ArrayPtr& operator=(const ArrayPtr&) = delete;

    ArrayPtr(ArrayPtr&& other) noexcept 
        : raw_ptr_(nullptr) 
    {
        swap(other); // swap
    }

    ArrayPtr& operator=(ArrayPtr&& other) noexcept {
        if (this != &other) {
            swap(other); // swap
        }
        return *this;
    }

    ~ArrayPtr() {
        delete[] raw_ptr_;
    }

    [[nodiscard]] Type* Release() noexcept {
        return std::exchange(raw_ptr_, nullptr);
    }

    void swap(ArrayPtr& other) noexcept {
        std::swap(raw_ptr_, other.raw_ptr_);
    }

    Type& operator[](size_t index) noexcept {
        assert(raw_ptr_ != nullptr);
        return raw_ptr_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(raw_ptr_ != nullptr);
        return raw_ptr_[index];
    }

    explicit operator bool() const {
        return raw_ptr_ != nullptr;
    }

    Type* Get() const noexcept {
        return raw_ptr_;
    }

private:
    Type* raw_ptr_ = nullptr;
};