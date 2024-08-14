# Copyright(c) 2019 spdlog authors Distributed under the MIT License (http://opensource.org/licenses/MIT)

cmake_minimum_required(VERSION 3.12)
project(spdlog_examples CXX)

set(CMAKE_CXX_STANDARD 20)

# if(NOT TARGET spdlog)
#     # Stand-alone build
#     find_package(spdlog REQUIRED)
# endif()


add_executable(example_header_only log.h example.cpp)
#target_link_libraries(example_header_only PRIVATE spdlog::spdlog_header_only)

target_include_directories(example_header_only PUBLIC ../include)
