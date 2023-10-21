#include "Options.hpp"
#include "catch2.hpp"
#include <cstring>
#include <iterator>

using namespace std;
using namespace DcsEmbler;

TEST_CASE("Default format is binary", "[Options]")
{
  REQUIRE(Options{ "inp", "outp" }.format == Format::binary);
}

TEST_CASE("Automatic output file naming works", "[Options]")
{
  string inputFileName{ "input_file_name" };
  string exp{ inputFileName + Options::outputFormatForBinary };
  REQUIRE(Options{ "input_file_name" }.getOutputFileName() == exp);
}

TEST_CASE("Output file naming for hex works as expected", "[Options]")
{
  string inputFileName{ "input_file_name" };
  string exp{ inputFileName + Options::outputFormatForHex };
  Options o{ .inputFileName = "input_file_name", .format = Format::hex };
  REQUIRE(o.getOutputFileName() == exp);
}

TEST_CASE("Automatic output file naming doesn't modify input file name",
          "[Options]")
{
  string inputFileName{ "input_file_name" };
  string exp{ inputFileName + Options::outputFormatForBinary };
  Options o{ "input_file_name" };
  string output_file_name = o.getOutputFileName();
  REQUIRE(o.getOutputFileName() == exp);
  REQUIRE(o.inputFileName == inputFileName);
}

TEST_CASE("x", "[Options]")
{
  array<char*, 5> args = {
    strdup("appname"),
    strdup("--outputFileName=asd-out.txt"),
    strdup("--inputFileName=asd.txt"),
    strdup("--format=bin"),
    strdup("--startOfMemory=0x0"),
  };
  Options o = Options::parseFrom(args.size(), args.data());
  REQUIRE(*o.inputFileName == "asd.txt");
  REQUIRE(o.getOutputFileName() == "asd-out.txt");
}

TEST_CASE("Usage is printed when invalid options are passed.", "[Options]")
{
    char* argv[2] = {
        strdup("appname"),
        strdup("--what")
    };
    Options o = Options::parseFrom(1, argv);
}
