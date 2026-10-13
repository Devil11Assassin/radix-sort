#pragma once
#include "generators.hpp"

namespace test_params
{
    enum class ParConfig
    {
        SEQ,
        PAR,
        BOTH
    };
    inline ParConfig parConfig = ParConfig::BOTH;

    enum class SizeConfig
    {
        SPECIFIC,
        SMALL,
        LARGE,
        BOTH
    };
    inline SizeConfig sizeConfig = SizeConfig::BOTH;
    inline std::size_t specificSize = 0;

    inline bool testAllShapes = true;
    inline generators::Shape initShape = generators::Shape::RANDOMIZED;

    inline bool loggingEnabled = false;
}
