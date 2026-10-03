#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <format>
#include <iostream>
#include <locale>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "generators.hpp"
#include "radix_sort.hpp"

// ===================
// -----Constants-----
// ===================

namespace
{
    constexpr bool kLoggingEnabled = true;
    const std::locale kLocale = std::locale("en_US.UTF-8");

    constexpr std::array<std::string_view, 5> kShapeToStr = {
        "randomized", "sorted", "reverse sorted",
        "nearly sorted", "duplicates"
    };
}

// =========================
// -----Data Generators-----
// =========================

namespace
{
    auto shape2str(generators::Shape shape)
    {
        return kShapeToStr[static_cast<std::size_t>(shape)];
    }

    auto getGenParams()
    {
        using Shape = generators::Shape;
        auto shape = GENERATE(Shape::RANDOMIZED, Shape::SORTED, Shape::REVERSE_SORTED, 
                              Shape::NEARLY_SORTED, Shape::DUPLICATES);
        
        auto n = GENERATE(static_cast<std::size_t>(0), 1, 10, 100, 1'000, 10'000,
                          100'000, 1'000'000, 10'000'000, 100'000'000);
    
        return std::tuple{ n, shape };
    }

    template<typename T>
    auto generate(std::size_t n, generators::Shape shape)
    {
        auto vStd = generators::generate<T>(n, shape);
        auto vRad(vStd);
        return std::tuple{ std::move(vStd), std::move(vRad) };
    }
}

// ======================
// -----Test Helpers-----
// ======================

namespace 
{
    template<typename T>
    void test(std::string_view name, bool testParallel = false)
    {
        auto [n, shape] = getGenParams();
        auto [vStd, vRad] = ::generate<T>(n, shape);
        
        if constexpr (kLoggingEnabled)
            std::cout << std::format(kLocale, "{} ({}, {}, {:L})\n",
                name, (testParallel) ? "par" : "seq", shape2str(shape), n);

        std::sort(std::execution::par, vStd.begin(), vStd.end());
        if (testParallel)
            radix_sort::sort(vRad, {}, true);
        else
            radix_sort::sort(vRad);
        
        CHECK(std::ranges::equal(vRad, vStd));
    }
}

// ==========================
// -----Sequential Tests-----
// ==========================

TEST_SUITE("sequential_integrals_signed") 
{
    TEST_CASE("int8")
    {
        test<std::int8_t>("int8");
    }
    
    TEST_CASE("int16")
    {
        test<std::int16_t>("int16");
    }
    
    TEST_CASE("int32")
    {
        test<std::int32_t>("int32");
    }

    TEST_CASE("int64")
    {
        test<std::int64_t>("int64");
    }
}

TEST_SUITE("sequential_integrals_unsigned") 
{
    TEST_CASE("uint8")
    {
        test<std::uint8_t>("uint8");
    }
    
    TEST_CASE("uint16")
    {
        test<std::uint16_t>("uint16");
    }
    
    TEST_CASE("uint32")
    {
        test<std::uint32_t>("uint32");
    }

    TEST_CASE("uint64")
    {
        test<std::uint64_t>("uint64");
    }
}

// ========================
// -----Parallel Tests-----
// ========================

TEST_SUITE("parallel_integrals_signed") 
{
    TEST_CASE("int8")
    {
        test<std::int8_t>("int8", true);
    }
    
    TEST_CASE("int16")
    {
        test<std::int16_t>("int16", true);
    }
    
    TEST_CASE("int32")
    {
        test<std::int32_t>("int32", true);
    }

    TEST_CASE("int64")
    {
        test<std::int64_t>("int64", true);
    }
}

TEST_SUITE("parallel_integrals_unsigned") 
{
    TEST_CASE("uint8")
    {
        test<std::uint8_t>("uint8", true);
    }
    
    TEST_CASE("uint16")
    {
        test<std::uint16_t>("uint16", true);
    }
    
    TEST_CASE("uint32")
    {
        test<std::uint32_t>("uint32", true);
    }

    TEST_CASE("uint64")
    {
        test<std::uint64_t>("uint64", true);
    }
}
