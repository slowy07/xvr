#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdlib>

#include "xvr_console_colors.h"
#include "xvr_memory.h"

static int callCount = 0;

void* testAllocator(void* pointer, size_t oldSize, size_t newSize) {
    callCount++;

    if (newSize == 0 && oldSize == 0) {
        return NULL;
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* mem = realloc(pointer, newSize);
    if (mem == NULL) {
        exit(-1);
    }

    return mem;
}

TEST_CASE("Memory allocation single", "[memory][unit]") {
    int* integer = XVR_ALLOCATE(int, 1);
    REQUIRE(integer != nullptr);
    XVR_FREE(int, integer);
}

TEST_CASE("Memory allocation array", "[memory][unit]") {
    int* array = XVR_ALLOCATE(int, 10);
    REQUIRE(array != nullptr);
    array[1] = 30;
    REQUIRE(array[1] == 30);
    XVR_FREE_ARRAY(int, array, 10);
}

TEST_CASE("Memory allocation multiple", "[memory][unit]") {
    int* array1 = XVR_ALLOCATE(int, 10);
    int* array2 = XVR_ALLOCATE(int, 10);

    REQUIRE(array1 != nullptr);
    REQUIRE(array2 != nullptr);

    array1[1] = 42;
    array2[1] = 42;

    REQUIRE(array1[1] == 42);
    REQUIRE(array2[1] == 42);

    XVR_FREE_ARRAY(int, array1, 10);
    XVR_FREE_ARRAY(int, array2, 10);
}

TEST_CASE("Custom allocator", "[memory][unit]") {
    callCount = 0;
    Xvr_setMemoryAllocator(testAllocator);

    int* integer = XVR_ALLOCATE(int, 1);
    XVR_FREE(int, integer);

    REQUIRE(callCount == 2);
}