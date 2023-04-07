cmake_minimum_required(VERSION 3.16.0)

project(TPX3 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(TPX3 tpx3processer.cpp)

target_sources( TPX3 PRIVATE
    tpx3reader.cpp
    )

target_include_directories( TPX3 PUBLIC
    
    )
