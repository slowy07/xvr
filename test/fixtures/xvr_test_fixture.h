#pragma once

#include <catch2/catch_test_macros.hpp>

class XvrTestFixture {
protected:
    XvrTestFixture() = default;
    virtual ~XvrTestFixture() = default;

    static void setUp() {}

    static void tearDown() {}
};
