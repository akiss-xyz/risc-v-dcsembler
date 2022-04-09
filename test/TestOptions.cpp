#include "catch2.hpp"
#include "Options.hpp"
#include <iterator>

using namespace std;
using namespace DcsEmbler;

TEST_CASE("Default format is binary", "[Options]")
{
    REQUIRE(Options{"inp", "outp"}.format == Format::binary);
}

TEST_CASE("Automatic output file naming works", "[Options]")
{
    string inputFileName{"input_file_name"};
    string exp{inputFileName + Options::outputFormatForBinary};
    REQUIRE(Options{"input_file_name"}.getOutputFileName() == exp);
}

TEST_CASE("Output file naming for hex works as expected", "[Options]")
{
    string inputFileName{"input_file_name"};
    string exp{inputFileName + Options::outputFormatForHex};
    Options o{.inputFileName = "input_file_name", .format = Format::hex};
    REQUIRE(o.getOutputFileName() == exp);
}

TEST_CASE("Automatic output file naming doesn't modify input file name", "[Options]")
{
    string inputFileName{"input_file_name"};
    string exp{inputFileName + Options::outputFormatForBinary};
    Options o{"input_file_name"};
    string output_file_name = o.getOutputFileName();
    REQUIRE(o.getOutputFileName() == exp);
    REQUIRE(o.inputFileName == inputFileName);
}
