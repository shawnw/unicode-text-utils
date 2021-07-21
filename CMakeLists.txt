cmake_minimum_required(VERSION 3.7)
project(recolumn VERSION 0.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(ICU REQUIRED COMPONENTS uc io i18n)

add_executable(recolumn recolumn.cpp formatter.cpp)
target_include_directories(recolumn PRIVATE ${ICU_INCLUDE_DIR})
target_link_libraries(recolumn PRIVATE ICU::uc ICU::io ICU::i18n)
