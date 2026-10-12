#pragma once
#include "generators.hpp"

namespace test_params
{
    inline bool testParallel = true;

    enum class SizeConfig
    {
        SMALL_TESTS,
        LARGE_TESTS,
        ALL_TESTS
    };
    inline SizeConfig sizeConfig = SizeConfig::SMALL_TESTS;

    inline bool testAllShapes = false;
    inline generators::Shape initShape = generators::Shape::RANDOMIZED;

    inline bool loggingEnabled = true;
}
