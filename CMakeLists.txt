cmake_minimum_required (VERSION 3.17)

file(READ "version" VERSION_OUTPUT)
string(STRIP ${VERSION_OUTPUT} PACKAGE_VERSION)

project(opencl-language-server
    DESCRIPTION "OpenCL Language Server"
    VERSION ${PACKAGE_VERSION}
    LANGUAGES CXX
)

if(NOT CMAKE_CXX_STANDARD) 
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(LINUX FALSE)
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

set(ENABLE_TESTING_DEFAULT OFF)
option(ENABLE_TESTING "Include unittest related targtes" 
    ${ENABLE_TESTING_DEFAULT}
)

message(STATUS "Build Configuration")
message(STATUS "Enable testing:" ${ENABLE_TESTING})
message(STATUS "CMake Generator:" ${CMAKE_GENERATOR})
message(STATUS "C++ Flags:" ${CMAKE_CXX_FLAGS})
message(STATUS "List of compile features:" ${CMAKE_CXX_COMPILE_FEATURES})
message(STATUS "Toolchain file:" ${CMAKE_TOOLCHAIN_FILE})

if(APPLE)
    find_package(OpenCL REQUIRED)
else()
    find_package(OpenCLICDLoader REQUIRED)
endif()
find_package(OpenclHeadersCpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

if(ENABLE_TESTING)
    find_package(catch2 REQUIRED)
    add_subdirectory(tests)
endif()

set(headers
    clinfo.hpp
    diagnostics.hpp
    jsonrpc.hpp
    lsp.hpp
    utils.hpp
)
set(sources
    clinfo.cpp
    diagnostics.cpp
    jsonrpc.cpp
    lsp.cpp
    main.cpp
    utils.cpp
)
list(TRANSFORM headers PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/include/")
list(TRANSFORM sources PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/src/")
source_group("include" FILES ${headers})
source_group("src" FILES ${sources})
set(libs nlohmann_json::nlohmann_json spdlog::spdlog OpenCL::HeadersCpp)
if(LINUX)
    set(libs ${libs} stdc++fs OpenCL::OpenCL)
elseif(APPLE)
    set(libs ${libs} ${OpenCL_LIBRARIES})
elseif(WIN32)
    set(libs ${libs} OpenCL::OpenCL)
endif()

add_executable (${PROJECT_NAME} ${sources} ${headers})
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    VERSION="${PACKAGE_VERSION}"
    CL_HPP_ENABLE_EXCEPTIONS
    CL_HPP_CL_1_2_DEFAULT_BUILD
    CL_HPP_TARGET_OPENCL_VERSION=120
    CL_HPP_MINIMUM_OPENCL_VERSION=120
)
target_link_libraries(${PROJECT_NAME} ${libs})
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/lib/filesystem/include"
)
if(APPLE)
    target_include_directories(${PROJECT_NAME} PRIVATE "${OpenCL_INCLUDE_DIRS}")
endif()
