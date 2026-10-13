#include "test.hpp"

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

#include "doctest.h"

#include "generators.hpp"
#include "radix_sort.hpp"

// ===================
// -----Constants-----
// ===================

namespace
{
    using namespace test_params;

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

    bool getParallelism()
    {
        switch (parConfig)
        {
            case ParConfig::SEQ:
            {
                return false;
            }
            case ParConfig::PAR:
            {
                return true;
            }
            default:
            {
                return GENERATE(false, true);
            }
        }
    }

    generators::Shape getGenShape()
    {
        using Shape = generators::Shape;
        
        if (testAllShapes)
        {
            return GENERATE(Shape::RANDOMIZED, Shape::SORTED, Shape::REVERSE_SORTED,
                Shape::NEARLY_SORTED, Shape::DUPLICATES);
        }

        return initShape;
    }

    template <typename T>
    std::size_t getGenSize()
    {
        switch (sizeConfig)
        {
            case SizeConfig::SPECIFIC:
            {
                return specificSize;
            }
            case SizeConfig::SMALL:
            {
                return GENERATE(static_cast<std::size_t>(0), 1, 10, 100, 1'000, 10'000,
                    100'000, 1'000'000);
            }
            case SizeConfig::LARGE:
            {
                if constexpr (std::same_as<T, Employee>)
                    return GENERATE(static_cast<std::size_t>(10'000'000));
                else if constexpr (std::same_as<T, std::string>)
                    return GENERATE(static_cast<std::size_t>(10'000'000), 50'000'000);
                else
                    return GENERATE(static_cast<std::size_t>(10'000'000), 100'000'000);
            }
            default:
            {
                if constexpr (std::same_as<T, Employee>)
                {
                    return GENERATE(static_cast<std::size_t>(0), 1, 10, 100, 1'000, 10'000,
                        100'000, 1'000'000, 10'000'000);
                }
                else if constexpr (std::same_as<T, std::string>)
                {
                    return GENERATE(static_cast<std::size_t>(0), 1, 10, 100, 1'000, 10'000,
                        100'000, 1'000'000, 10'000'000, 50'000'000);
                }
                else
                {
                    return GENERATE(static_cast<std::size_t>(0), 1, 10, 100, 1'000, 10'000,
                        100'000, 1'000'000, 10'000'000, 100'000'000);
                }
            }
        }
    }

    template <typename T>
    auto getGenParams()
    {
        bool isParallel = getParallelism();
        generators::Shape shape = getGenShape();
        std::size_t n = getGenSize<T>();

        return std::tuple{ isParallel, n, shape };
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
    void sortRad(std::vector<T>& v, bool isParallel)
    {
        constexpr auto kProj = getRadProj<T, U>();
        radix_sort::sort(v, kProj, isParallel);
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

        if (loggingEnabled)
        {
            if (!isEqual)
                printDiff(vRad, vStd);
        }
    }

    template <typename T, typename U = T>
    void test(std::string_view name)
    {
        auto [isParallel, n, shape] = getGenParams<T>();
        auto [vStd, vRad] = ::generate<T>(n, shape);
        
        if (loggingEnabled)
        {
            std::cout << std::format(kLocale, "{} ({}, {}, {:L})\n",
                name, (isParallel) ? "par" : "seq", shape2str(shape), n);
        }

        sortStd<T, U>(vStd);
        sortRad<T, U>(vRad, isParallel);
        
        checkEquality(vRad, vStd);
    }
}

// ===============
// -----Tests-----
// ===============

TEST_SUITE("integrals_signed") 
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

TEST_SUITE("integrals_unsigned") 
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

TEST_SUITE("floating_point") 
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

TEST_SUITE("strings")
{
    TEST_CASE("string")
    {
        test<std::string>("string");
    }
}

TEST_SUITE("complex")
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
