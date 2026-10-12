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
        SMALL,
        LARGE,
        BOTH
    };
    inline SizeConfig sizeConfig = SizeConfig::BOTH;

    inline bool testAllShapes = true;
    inline generators::Shape initShape = generators::Shape::RANDOMIZED;

    inline bool loggingEnabled = false;
}
