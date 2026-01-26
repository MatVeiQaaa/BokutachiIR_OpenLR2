cmake_minimum_required(VERSION 3.23)

project(BokutachiIR_OpenLR2)

find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_library(BokutachiIR SHARED)

set_target_properties(
    BokutachiIR
    PROPERTIES
        CXX_STANDARD 23
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

target_sources(BokutachiIR PUBLIC ./src/BokutachiIR.cpp)

target_include_directories(BokutachiIR PUBLIC ./include/)

target_link_libraries(BokutachiIR PRIVATE cpr::cpr)
target_link_libraries(BokutachiIR PRIVATE nlohmann_json::nlohmann_json)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX ".x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH_SUFFIX ".x86")
else()
    message(FATAL_ERROR "Unsupported CMAKE_SIZEOF_VOID_P")
endif()

set(CONF_SUFFIX $<$<CONFIG:Debug>:_D>)

set_target_properties(
    BokutachiIR
    PROPERTIES OUTPUT_NAME "BokutachiIR${CONF_SUFFIX}${ARCH_SUFFIX}"
)
