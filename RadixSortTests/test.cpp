#include <algorithm>
#include <array>
#include <bit>
#include <compare>
#include <concepts>
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
    constexpr bool kTestAllShapes = true;
    constexpr generators::Shape kShape = generators::Shape::RANDOMIZED;

    enum class SizeConfig
    {
        SMALL_TESTS,
        LARGE_TESTS,
        ALL_TESTS
    };
    constexpr SizeConfig kSizeConfig = SizeConfig::ALL_TESTS;

    constexpr bool kLoggingEnabled = true;
    const std::locale kLocale = std::locale("en_US.UTF-8");
    constexpr std::size_t kMaxErrorPrints = 10;

    constexpr std::array<std::string_view, 5> kShapeToStr = {
        "randomized", "sorted", "reverse sorted",
        "nearly sorted", "duplicates"
    };

    constexpr auto kFpEqual = [](const auto a, const auto b) 
    { 
        return std::strong_order(a, b) == 0; 
    };

    template <std::size_t S> struct t2u_impl;
    template <> struct t2u_impl<2> { using type = std::uint16_t; };
    template <> struct t2u_impl<4> { using type = std::uint32_t; };
    template <> struct t2u_impl<8> { using type = std::uint64_t; };

    template <typename T>
    using t2u = t2u_impl<sizeof(T)>::type;
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

    auto getGenShape()
    {
        using Shape = generators::Shape;
        
        Shape shape = kShape;
        if constexpr (kTestAllShapes)
        {
            shape = GENERATE(Shape::RANDOMIZED, Shape::SORTED, Shape::REVERSE_SORTED,
                Shape::NEARLY_SORTED, Shape::DUPLICATES);
        }

        return shape;
    }

    template <typename T>
    auto getGenSize()
    {
        std::size_t n = 0;
        if constexpr (kSizeConfig == SizeConfig::SMALL_TESTS)
        {
            n = GENERATE(static_cast<std::size_t>(0), 1, 10, 100, 1'000, 10'000,
                100'000, 1'000'000);
        }
        else if constexpr (kSizeConfig == SizeConfig::LARGE_TESTS)
        {
            if constexpr (std::same_as<T, Employee>)
                n = GENERATE(static_cast<std::size_t>(10'000'000));
            else if constexpr (std::same_as<T, std::string>)
                n = GENERATE(static_cast<std::size_t>(10'000'000), 50'000'000);
            else
                n = GENERATE(static_cast<std::size_t>(10'000'000), 100'000'000);
        }
        else
        {
            if constexpr (std::same_as<T, Employee>)
            {
                n = GENERATE(static_cast<std::size_t>(0), 1, 10, 100, 1'000, 10'000,
                    100'000, 1'000'000, 10'000'000);
            }
            else if constexpr (std::same_as<T, std::string>)
            {
                n = GENERATE(static_cast<std::size_t>(0), 1, 10, 100, 1'000, 10'000,
                    100'000, 1'000'000, 10'000'000, 50'000'000);
            }
            else
            {
                n = GENERATE(static_cast<std::size_t>(0), 1, 10, 100, 1'000, 10'000, 
                    100'000, 1'000'000, 10'000'000, 100'000'000);
            }
        }

        return n;
    }

    template <typename T>
    auto getGenParams()
    {
        generators::Shape shape = getGenShape();
        std::size_t n = getGenSize<T>();

        return std::tuple{ n, shape };
    }

    template <typename T>
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
    template <typename T, typename U = T>
    constexpr auto getStdComp()
    {
        if constexpr (std::floating_point<T>)
            return [](const auto a, const auto b) { return std::strong_order(a, b) < 0; };
        else if constexpr (!std::same_as<T, Employee>)
            return std::less<>();
        else
        {
            if constexpr (std::same_as<U, decltype(Employee::age)>)
                return [](const Employee& a, const Employee& b) { return a.age < b.age; };
            else if constexpr (std::same_as<U, decltype(Employee::id)>)
                return [](const Employee& a, const Employee& b) { return a.id < b.id; };
            else if constexpr (std::same_as<U, decltype(Employee::salary_f)>)
                return [](const Employee& a, const Employee& b) { return std::strong_order(a.salary_f, b.salary_f) < 0; };
            else if constexpr (std::same_as<U, decltype(Employee::salary)>)
                return [](const Employee& a, const Employee& b) { return std::strong_order(a.salary, b.salary) < 0; };
            else if constexpr (std::same_as<U, decltype(Employee::name)>)
                return [](const Employee& a, const Employee& b) { return a.name < b.name; };
        }
    }

    template <typename T, typename U = T>
    constexpr auto getRadProj()
    {
        if constexpr (!std::same_as<T, Employee>)
            return std::identity{};
        else
        {
            if constexpr (std::same_as<U, decltype(Employee::age)>)
                return &Employee::age;
            else if constexpr (std::same_as<U, decltype(Employee::id)>)
                return &Employee::id;
            else if constexpr (std::same_as<U, decltype(Employee::salary_f)>)
                return &Employee::salary_f;
            else if constexpr (std::same_as<U, decltype(Employee::salary)>)
                return &Employee::salary;
            else if constexpr (std::same_as<U, decltype(Employee::name)>)
                return &Employee::name;
        }
    }

    template <typename T, typename U = T>
    void sortStd(std::vector<T>& v)
    {
        constexpr auto kComp = getStdComp<T, U>();

        if constexpr (std::same_as<T, Employee>)
            std::stable_sort(std::execution::par, v.begin(), v.end(), kComp);
        else
            std::sort(std::execution::par, v.begin(), v.end(), kComp);
    }

    template <typename T, typename U = T>
    void sortRad(std::vector<T>& v, bool testParallel)
    {
        constexpr auto kProj = getRadProj<T, U>();
        radix_sort::sort(v, kProj, testParallel);
    }

    template <typename T>
    void printDiff(const std::vector<T>& vRad, const std::vector<T>& vStd)
    {
        if constexpr (std::floating_point<T>)
        {
            std::cout << "index: (radix value, expected value) hex(radix value, expected value)\n";

            using U = t2u<T>;
            for (std::size_t i = 0, size = vRad.size(); i < size && i < kMaxErrorPrints; i++)
            {
                if (!kFpEqual(vRad[i], vStd[i]))
                    std::cout << std::format("{}:\t({}, {})\thex({}, {})\n", i, vRad[i], vStd[i],
                        std::bit_cast<U>(vRad[i]), std::bit_cast<U>(vStd[i]));
            }
        }
        else if constexpr (!std::same_as<T, Employee>)
        {
            std::cout << "\nindex: (radix value, expected value)\n";

            for (std::size_t i = 0, size = vRad.size(); i < size && i < kMaxErrorPrints; i++)
            {
                if (vRad[i] != vStd[i])
                    std::cout << std::format("{}:\t({}, {})\n", i, vRad[i], vStd[i]);
            }
        }

        std::cout << '\n';
    }

    template <typename T>
    void checkEquality(const std::vector<T>& vRad, const std::vector<T>& vStd)
    {
        bool isEqual = false;
        if constexpr (std::floating_point<T>)
            isEqual = std::ranges::equal(vRad, vStd, kFpEqual);
        else
            isEqual = std::ranges::equal(vRad, vStd);

        CHECK(isEqual);

        if constexpr (kLoggingEnabled)
        {
            if (!isEqual)
                printDiff(vRad, vStd);
        }
    }

    template <typename T, typename U = T>
    void test(std::string_view name, bool testParallel = false)
    {
        auto [n, shape] = getGenParams<T>();
        auto [vStd, vRad] = ::generate<T>(n, shape);
        
        if constexpr (kLoggingEnabled)
        {
            std::cout << std::format(kLocale, "{} ({}, {}, {:L})\n",
                name, (testParallel) ? "par" : "seq", shape2str(shape), n);
        }

        sortStd<T, U>(vStd);
        sortRad<T, U>(vRad, testParallel);
        
        checkEquality(vRad, vStd);
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

TEST_SUITE("sequential_floating_point") 
{
    TEST_CASE("float")
    {
        test<float>("float");
    }
    TEST_CASE("double")
    {
        test<double>("double");
    }
}

TEST_SUITE("sequential_strings")
{
    TEST_CASE("string")
    {
        test<std::string>("string");
    }
}

TEST_SUITE("sequential_complex")
{
    TEST_CASE("employee::int32")
    {
        test<Employee, decltype(Employee::age)>("employee::int32");
    }
    TEST_CASE("employee::int64")
    {
        test<Employee, decltype(Employee::id)>("employee::int64");
    }
    TEST_CASE("employee::float")
    {
        test<Employee, decltype(Employee::salary_f)>("employee::float");
    }
    TEST_CASE("employee::double")
    {
        test<Employee, decltype(Employee::salary)>("employee::double");
    }
    TEST_CASE("employee::string")
    {
        test<Employee, decltype(Employee::name)>("employee::string");
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

TEST_SUITE("parallel_floating_point")
{
    TEST_CASE("float")
    {
        test<float>("float", true);
    }
    TEST_CASE("double")
    {
        test<double>("double", true);
    }
}

TEST_SUITE("parallel_strings")
{
    TEST_CASE("string")
    {
        test<std::string>("string", true);
    }
}

TEST_SUITE("parallel_complex")
{
    TEST_CASE("employee::int32")
    {
        test<Employee, decltype(Employee::age)>("employee::int32", true);
    }
    TEST_CASE("employee::int64")
    {
        test<Employee, decltype(Employee::id)>("employee::int64", true);
    }
    TEST_CASE("employee::float")
    {
        test<Employee, decltype(Employee::salary_f)>("employee::float", true);
    }
    TEST_CASE("employee::double")
    {
        test<Employee, decltype(Employee::salary)>("employee::double", true);
    }
    TEST_CASE("employee::string")
    {
        test<Employee, decltype(Employee::name)>("employee::string", true);
    }
}
