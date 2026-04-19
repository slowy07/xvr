#ifndef XVR_REFSTRING_HPP
#define XVR_REFSTRING_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "xvr_memory.h"
#include "xvr_memory.hpp"

namespace xvr {

class RefString {
public:
    RefString() = default;
    explicit RefString(const char* str);
    explicit RefString(const char* str, size_t len);
    explicit RefString(std::string_view view);
    
    RefString(const RefString& other);
    RefString& operator=(const RefString& other);
    
    RefString(RefString&& other) noexcept;
    RefString& operator=(RefString&& other) noexcept;
    
    ~RefString();
    
    const char* data() const { return data_; }
    size_t length() const { return length_; }
    size_t refCount() const { return refCount_; }
    size_t capacity() const { return capacity_; }
    
    std::string_view view() const { return std::string_view(data_, length_); }
    std::string toString() const { return std::string(data_, length_); }
    
    void incrementRef();
    void decrementRef();
    
    static RefString* create(const char* str);
    static RefString* create(const char* str, size_t len);
    static RefString* create(std::string_view view);
    static void destroy(RefString* rs);
    
private:
    char* data_{nullptr};
    size_t length_{0};
    size_t refCount_{1};
    size_t capacity_{0};
    
    void allocateAndCopy(const char* str, size_t len);
};

using RefStringPtr = std::shared_ptr<RefString>;
using RefStringWeakPtr = std::weak_ptr<RefString>;

inline bool operator==(const RefString& lhs, const RefString& rhs) {
    return lhs.view() == rhs.view();
}

inline bool operator!=(const RefString& lhs, const RefString& rhs) {
    return !(lhs == rhs);
}

}  // namespace xvr

#endif  // XVR_REFSTRING_HPP