cmake_minimum_required(VERSION 3.7)
project(unicode-text-utils VERSION 0.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(ICU 60 REQUIRED COMPONENTS uc io i18n)

add_executable(recolumn recolumn.cpp formatter.cpp util.cpp)
target_include_directories(recolumn PRIVATE ${ICU_INCLUDE_DIR})
target_link_libraries(recolumn PRIVATE ICU::uc ICU::io ICU::i18n)

add_executable(unorm unorm.cpp)
target_include_directories(unorm PRIVATE ${ICU_INCLUDE_DIR})
target_link_libraries(unorm PRIVATE ICU::uc)

add_executable(ufmt ufmt.cpp util.cpp)
target_include_directories(ufmt PRIVATE ${ICU_INCLUDE_DIR})
target_link_libraries(ufmt PRIVATE ICU::uc ICU::io)

add_executable(uwc uwc.cpp util.cpp)
target_include_directories(uwc PRIVATE ${ICU_INCLUDE_DIR})
target_link_libraries(uwc PRIVATE ICU::uc ICU::io)
