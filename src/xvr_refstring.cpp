#include "xvr_refstring.hpp"
#include "xvr_refstring.h"
#include "xvr_memory.h"
#include <cstring>

namespace xvr {

void RefString::allocateAndCopy(const char* str, size_t len) {
    capacity_ = len + 1;
    data_ = static_cast<char*>(Xvr_reallocate(nullptr, 0, capacity_));
    length_ = len;
    refCount_ = 1;
    if (str) {
        std::memcpy(data_, str, len);
        data_[len] = '\0';
    }
}

RefString::RefString(const char* str) {
    if (str) {
        allocateAndCopy(str, std::strlen(str));
    }
}

RefString::RefString(const char* str, size_t len) {
    if (str && len > 0) {
        allocateAndCopy(str, len);
    }
}

RefString::RefString(std::string_view view) {
    if (!view.empty()) {
        allocateAndCopy(view.data(), view.size());
    }
}

RefString::RefString(const RefString& other) 
    : data_(other.data_), length_(other.length_), 
      refCount_(other.refCount_), capacity_(other.capacity_) {
    incrementRef();
}

RefString& RefString::operator=(const RefString& other) {
    if (this != &other) {
        decrementRef();
        data_ = other.data_;
        length_ = other.length_;
        refCount_ = other.refCount_;
        capacity_ = other.capacity_;
        incrementRef();
    }
    return *this;
}

RefString::RefString(RefString&& other) noexcept 
    : data_(other.data_), length_(other.length_),
      refCount_(other.refCount_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.length_ = 0;
    other.refCount_ = 0;
    other.capacity_ = 0;
}

RefString& RefString::operator=(RefString&& other) noexcept {
    if (this != &other) {
        decrementRef();
        data_ = other.data_;
        length_ = other.length_;
        refCount_ = other.refCount_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.length_ = 0;
        other.refCount_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

RefString::~RefString() {
    decrementRef();
}

void RefString::incrementRef() {
    if (data_) {
        ++refCount_;
    }
}

void RefString::decrementRef() {
    if (data_ && --refCount_ == 0) {
        XVR_FREE(char, data_);
        data_ = nullptr;
    }
}

RefString* RefString::create(const char* str) {
    return new RefString(str);
}

RefString* RefString::create(const char* str, size_t len) {
    return new RefString(str, len);
}

RefString* RefString::create(std::string_view view) {
    return new RefString(view);
}

void RefString::destroy(RefString* rs) {
    delete rs;
}

}  // namespace xvr