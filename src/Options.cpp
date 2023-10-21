#include "Options.hpp"

namespace DcsEmbler {

const char* Options::outputFormatForBinary = ".bin.riscv5i";
const char* Options::outputFormatForHex = ".hex.riscv5i";

auto Options::parseFrom(int argc, char **argv) -> Options {
  auto app = structopt::app("DCSembler", "0.0.1");
  try {
    return app.parse<Options>(argc,
                              const_cast<char**>(argv));
  } catch (structopt::exception &e) {
    cout << e.what();
    cout << " Usage: ";
    cout << app.help();
    exit(EXIT_FAILURE);
  }
}

auto DcsEmbler::Options::getOutputFileName() -> string {
  if (outputFileName.has_value()) {
    return *outputFileName;
  } else {
    switch (*format) {
      case Format::binary:
      case Format::bin:
        return *inputFileName + outputFormatForBinary;
        break;
      case Format::hexadecimal:
      case Format::hex:
        return *inputFileName + outputFormatForHex;
        break;
      default:
        cerr << " Unknown value for Format\n";
        exit(EXIT_FAILURE);
        break;
    }
  }
}

}
