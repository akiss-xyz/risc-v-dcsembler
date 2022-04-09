#pragma once

#include <optional>
#include <string>

#include "structopt/structopt.hpp"

using namespace std;

namespace DcsEmbler {

enum class Format : unsigned short { binary, bin, hex, hexadecimal };

/// This struct represents the command line options for the program.
struct Options {
    static const char* outputFormatForBinary;
    static const char* outputFormatForHex;

    optional<string> inputFileName{"stdin"};
    optional<string> outputFileName{};

    optional<Format> format = Format::binary;

    /// In bytes.
    optional<int> startOfMemory = 0;

    /// FEATURE: Make this do something.
    optional<bool> verbose = false;

    static auto parseFrom(int argc, char **argv) -> Options;
    auto getOutputFileName() -> string;
};

}

STRUCTOPT(DcsEmbler::Options, inputFileName, outputFileName, format, verbose, startOfMemory);
