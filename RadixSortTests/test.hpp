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
    inline ParConfig parConfig = ParConfig::PAR;

    enum class SizeConfig
    {
        SMALL,
        LARGE,
        BOTH
    };
    inline SizeConfig sizeConfig = SizeConfig::SMALL;

    inline bool testAllShapes = false;
    inline generators::Shape initShape = generators::Shape::RANDOMIZED;

    inline bool loggingEnabled = true;
}
