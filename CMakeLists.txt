cmake_minimum_required(VERSION 3.3)
project(gosat CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fno-rtti")

set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0 -fno-inline -fno-omit-frame-pointer" CACHE STRING
        "Flags used by the C++ compiler for Debug builds."
        FORCE)

set(CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE STRING
        "Flags used by the C++ compiler for Release builds."
        FORCE)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Finds and configures LLVM build directories
find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

if(CMAKE_PREFIX_PATH)
    include_directories(${CMAKE_PREFIX_PATH}/include)
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    message(STATUS "Possible build types are Debug and Release.")
    set(CMAKE_BUILD_TYPE Debug)
else()
    message(STATUS "Current build type is ${CMAKE_BUILD_TYPE}")
endif()

include_directories(src)

add_library(libz3 SHARED IMPORTED)
add_library(libnlopt SHARED IMPORTED)

find_library(libz3-path NAMES "libz3.so")
if(NOT libz3-path)
    message(FATAL_ERROR "libz3.so was not found! please check your installation and/or cmake configurations. ")
endif()

find_library(libnlopt-path NAMES "libnlopt.so")
if(NOT libnlopt-path)
    message(FATAL_ERROR "libnlopt.so was not found! please check your installation and/or cmake configurations. ")
endif()

set_property(TARGET libz3 PROPERTY IMPORTED_LOCATION ${libz3-path})
set_property(TARGET libnlopt PROPERTY IMPORTED_LOCATION ${libnlopt-path})

llvm_map_components_to_libnames(llvm_libs_required
        Core
        ExecutionEngine
        MCJIT
        native
)

set(SOURCE_FILES
        src/main.cpp
        src/Utils/fpa_util.cpp
        src/ExprAnalyzer/FPExprAnalyzer.cpp
        src/IRGen/FPIRGenerator.cpp
        src/CodeGen/FPExprCodeGenerator.cpp
        src/CodeGen/FPExprLibGenerator.cpp
        src/Optimizer/NLoptOptimizer.cpp
        src/CodeGen/CodeGen.cpp
        src/Optimizer/ModelValidator.cpp
)

add_subdirectory(tools/nl_solver)
add_executable(gosat ${SOURCE_FILES})

target_link_libraries(gosat libz3 libnlopt ${llvm_libs_required} )

