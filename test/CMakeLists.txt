find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

add_library(catch_main OBJECT catch_main.cpp)
target_link_libraries(catch_main PUBLIC Catch2::Catch2)
target_compile_definitions(catch_main PRIVATE CATCH_CONFIG_FAST_COMPILE CATCH_CONFIG_DISABLE_MATCHES)

FILE(GLOB_RECURSE cpptestsources test*.cpp)
FILE(GLOB_RECURSE cppTestsources Test*.cpp)
FILE(GLOB_RECURSE cppsources ../src/*.cpp)
list(FILTER cppsources EXCLUDE REGEX "main.cpp")

add_executable(tests ${cpptestsources} ${cppTestsources} ${cppsources})
target_include_directories(tests PUBLIC ../include/)
target_include_directories(tests PUBLIC ../src/)
target_link_libraries(tests PRIVATE catch_main)
target_compile_definitions(tests PRIVATE CATCH_CONFIG_FAST_COMPILE CATCH_CONFIG_DISABLE_MATCHES)

target_precompile_headers(tests PRIVATE catch2.hpp)

catch_discover_tests(tests)
