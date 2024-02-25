cmake_minimum_required (VERSION 3.16)

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

if(MSVC)
    add_compile_options(/WX /W4 /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wimplicit-fallthrough -Werror)
    if(LINUX)
        # Prebuilt Clang and LLVM static libraries are position-dependent 
        add_link_options(-no-pie)
    endif()
endif()

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
find_package(OpenCLHeadersCpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CLI11 REQUIRED)
find_package(uriparser REQUIRED)

# libclang is not currently used, uncomment when needed (#17, #24)
# add_subdirectory(libclang)

if(ENABLE_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)
    enable_testing()
    add_subdirectory(tests)
endif()

set(headers
    clinfo.hpp
    device.hpp
    diagnostics.hpp
    jsonrpc.hpp
    log.hpp
    lsp.hpp
    utils.hpp
)
set(sources
    clinfo.cpp
    diagnostics.cpp
    jsonrpc.cpp
    log.cpp
    lsp.cpp
    main.cpp
    utils.cpp
)

if(APPLE)
    set(headers ${headers} oslogger.hpp)
endif()

list(TRANSFORM headers PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/include/")
list(TRANSFORM sources PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/src/")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp.in version.hpp)
source_group("include" FILES ${headers})
source_group("src" FILES ${sources})
set(conan_libs nlohmann_json::nlohmann_json spdlog::spdlog OpenCL::HeadersCpp CLI11::CLI11 uriparser::uriparser)
if(LINUX)
    set(conan_libs ${conan_libs} stdc++fs OpenCL::OpenCL)
elseif(APPLE)
    set(conan_libs ${conan_libs} ${OpenCL_LIBRARIES})
elseif(WIN32)
    set(conan_libs ${conan_libs} OpenCL::OpenCL)
endif()

add_executable (${PROJECT_NAME} ${sources} ${headers})
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    OPENCL_LS_VERSION="${PACKAGE_VERSION}"
    CL_HPP_ENABLE_EXCEPTIONS
    CL_HPP_CL_1_2_DEFAULT_BUILD
)
if(APPLE)
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    CL_HPP_MINIMUM_OPENCL_VERSION=120
    CL_HPP_TARGET_OPENCL_VERSION=120
)
else()
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    CL_HPP_MINIMUM_OPENCL_VERSION=110
    CL_HPP_TARGET_OPENCL_VERSION=300
)
endif()


target_link_libraries(${PROJECT_NAME} ${conan_libs} 
    # libclang-imported
)
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${PROJECT_SOURCE_DIR}/include"
    ${CMAKE_CURRENT_BINARY_DIR}
)
if(APPLE)
    target_include_directories(${PROJECT_NAME} PRIVATE "${OpenCL_INCLUDE_DIRS}")
endif()
