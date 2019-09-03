cmake_minimum_required(VERSION 3.13)
project(slang)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE sources "src/*.cpp")

add_executable(slang ${sources})
target_compile_options(slang PUBLIC -Wall -fno-limit-debug-info)
target_include_directories(slang PUBLIC "include" "lib/cxxopts/include")