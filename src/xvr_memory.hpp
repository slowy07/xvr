#ifndef XVR_MEMORY_HPP
#define XVR_MEMORY_HPP

#include <cstddef>
#include <memory>
#include <new>

namespace xvr {

using ReallocateFn = void* (*)(void* pointer, size_t oldSize, size_t newSize);

void setMemoryAllocator(ReallocateFn allocator);
ReallocateFn getMemoryAllocator();

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

template<typename T>
T* allocate(size_t count = 1) {
    return static_cast<T*>(reallocate(nullptr, 0, sizeof(T) * count));
}

template<typename T>
void free(T* pointer, size_t count = 1) {
    reallocate(pointer, sizeof(T) * count, 0);
}

template<typename T>
T* grow(T* oldPtr, size_t oldCount, size_t newCount) {
    return static_cast<T*>(reallocate(oldPtr, sizeof(T) * oldCount, sizeof(T) * newCount));
}

template<typename T>
T* shrink(T* oldPtr, size_t oldCount, size_t newCount) {
    return static_cast<T*>(reallocate(oldPtr, sizeof(T) * oldCount, sizeof(T) * newCount));
}

constexpr size_t growCapacity(size_t capacity) {
    return capacity < 8 ? 8 : capacity * 2;
}

constexpr size_t growCapacityFast(size_t capacity) {
    return capacity < 32 ? 32 : capacity * 2;
}

#if defined(XVR_DEBUG) || defined(DEBUG)
void debugPrintMemoryStats();
size_t debugGetAllocCount();
size_t debugGetFreeCount();
size_t debugGetCurrentMemory();
void debugResetMemoryStats();
#endif

template<typename T>
class UniquePtr {
    T* ptr_;
public:
    explicit UniquePtr(T* p = nullptr) : ptr_(p) {}
    ~UniquePtr() { free<T>(ptr_); }
    
    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;
    
    UniquePtr(UniquePtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            free<T>(ptr_);
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }
    
    void reset(T* p = nullptr) {
        free<T>(ptr_);
        ptr_ = p;
    }
    
    T* release() {
        T* tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }
};

template<typename T, typename Deleter>
class UniquePtrWithDeleter {
    T* ptr_;
    Deleter deleter_;
public:
    explicit UniquePtrWithDeleter(T* p = nullptr, Deleter d = Deleter{}) 
        : ptr_(p), deleter_(d) {}
    ~UniquePtrWithDeleter() { if (ptr_) deleter_(ptr_); }
    
    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    void reset(T* p = nullptr) { 
        if (ptr_) deleter_(ptr_); 
        ptr_ = p; 
    }
};

}  // namespace xvr

#endif  // XVR_MEMORY_HPP