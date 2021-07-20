cmake_minimum_required(VERSION 3.7)
project(recolumn VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(ICU REQUIRED COMPONENTS uc)

add_executable(recolumn recolumn.cpp)
target_include_directories(recolumn PRIVATE ICU::uc)
target_link_libraries(recolumn PRIVATE ICU::uc)
