#define CATCH_AMALGAMATED_CUSTOM_MAIN
#include "catch2/catch_amalgamated.hpp"

int main(int argc, char* argv[])
{
    return Catch::Session().run(argc, argv);
}

// Placeholder test to ensure the test framework compiles
TEST_CASE("Sanity check", "[sanity]")
{
    REQUIRE(1 + 1 == 2);
}
