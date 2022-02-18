#pragma once

#include <optional>
#include <string>

#include "include/structopt/structopt.hpp"

using namespace std;

namespace DcsEmbler {

enum class Format : unsigned short { binary, bin, hex, hexadecimal };

struct Options {
    static const char* outputFormatForBinary;
    static const char* outputFormatForHex;

    optional<string> inputFileName = "stdin";
    optional<string> outputFileName;

    optional<Format> format = Format::binary;

    /// In bytes.
    optional<int> startOfMemory = 0;

    optional<bool> verbose = false;

    static auto parseFrom(int argc, char **argv) -> Options;
    auto getOutputFileName() -> string;
};

}

STRUCTOPT(DcsEmbler::Options, inputFileName, outputFileName, format, verbose, startOfMemory);