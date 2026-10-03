#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "generators.hpp"

TEST_SUITE("MEOW") 
{
    TEST_CASE("psps")
    {
        auto v = generators::generate<int>(100);
        CHECK(v.size() != 0);
    }
}
