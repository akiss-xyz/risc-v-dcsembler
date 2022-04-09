cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

project(dcsembler
        VERSION 0.0.1
        DESCRIPTION "A RISC-V (RV32I) assembler built for the DCS RISC-V project."
        HOMEPAGE_URL "https://github.com/DCS-RISC-V"
        LANGUAGES CXX C)

# A lot of this cmake file is adapted from Jason Turner's best practices example project.
# See: https://github.com/lefticus/cpp_starter_project

include(cmake/Sanitizers.cmake)

include(cmake/AssureOutOfSourceBuilds.cmake)
assureoutofsourcebuilds()

include(cmake/DefaultBuildTypeDebug.cmake)

include(cmake/Doxygen.cmake)
enable_doxygen()

# Enable IPO, link time optimization
include(cmake/IPO.cmake)

if (MSVC)
	add_compile_options(/W4)
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Generates compile_commands.json for text editors, clang-* tools, and sourcetrail and stuff.
# For example, my vim uses these for autocompletion.
# See: https://github.com/ycm-core/YouCompleteMe#option-1-use-a-compilation-database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(cmake/CopyCompileCommands.cmake)

FILE(GLOB_RECURSE cppsources src/*.cpp)
FILE(GLOB_RECURSE csources src/*.c)
add_executable(${PROJECT_NAME} ${cppsources} ${csources})

target_include_directories(${PROJECT_NAME} PUBLIC src/)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PUBLIC /)

# Testing
option(ENABLE_TESTING "Enable tests" ON)
if(ENABLE_TESTING)
    enable_testing()
    message("Enabled tests.")
    add_subdirectory(test)
endif()
