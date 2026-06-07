if(NOT DEFINED LLVM_ROOT_DIR OR "${LLVM_ROOT_DIR}" STREQUAL "")
    message(FATAL_ERROR "LLVM_ROOT_DIR is not defined!")
endif()

string(STRIP "${LLVM_ROOT_DIR}" LLVM_ROOT_DIR_CLEAN)

file(GLOB CLANG_VERSION_DIR RELATIVE "${LLVM_ROOT_DIR_CLEAN}" "${LLVM_ROOT_DIR_CLEAN}/lib/clang/*")

list(LENGTH CLANG_VERSION_DIR DIR_COUNT)
if(DIR_COUNT EQUAL 0)
    message(FATAL_ERROR "Could not detect Clang resource directory")
elseif(DIR_COUNT GREATER 1)
    list(SORT CLANG_VERSION_DIR)
    list(GET CLANG_VERSION_DIR -1 CLANG_VERSION_DIR)
endif()

set(OPENCL_C_H "${LLVM_ROOT_DIR_CLEAN}/${CLANG_VERSION_DIR}/include/opencl-c.h")
set(OPENCL_C_BASE_H "${LLVM_ROOT_DIR_CLEAN}/${CLANG_VERSION_DIR}/include/opencl-c-base.h")

# Read file contents as hexadecimal strings natively
file(READ "${OPENCL_C_H}" OPENCL_C_HEX HEX)
file(READ "${OPENCL_C_BASE_H}" OPENCL_C_BASE_HEX HEX)

# Format hex data into a comma-separated list of raw numbers (e.g., 0x7B, 0x22...)
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1," OPENCL_C_BYTES "${OPENCL_C_HEX}")
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1," OPENCL_C_BASE_BYTES "${OPENCL_C_BASE_HEX}")

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_HEADER "${GENERATED_DIR}/embedded_headers.h")

file(WRITE "${GENERATED_HEADER}" [=[
// Auto-generated file. Do not edit.
#pragma once
#include <unordered_map>
#include <string>

namespace ocls::resources {

// Raw byte array segments allocated in read-only program storage
]=])

file(APPEND "${GENERATED_HEADER}" "static const unsigned char opencl_c_data[] = { ${OPENCL_C_BYTES} };\n")
file(APPEND "${GENERATED_HEADER}" "static const unsigned char opencl_c_base_data[] = { ${OPENCL_C_BASE_BYTES} };\n\n")

file(APPEND "${GENERATED_HEADER}" [=[
inline const std::unordered_map<std::string, std::string>& get_headers() {
    static const std::unordered_map<std::string, std::string> headers = {
]=])

file(APPEND "${GENERATED_HEADER}" "        {\"opencl-c.h\", std::string(reinterpret_cast<const char*>(opencl_c_data), sizeof(opencl_c_data))},\n")
file(APPEND "${GENERATED_HEADER}" "        {\"opencl-c-base.h\", std::string(reinterpret_cast<const char*>(opencl_c_base_data), sizeof(opencl_c_base_data))},\n")

file(APPEND "${GENERATED_HEADER}" [=[
    };
    return headers;
}
}
]=])

add_library(embedded_resources INTERFACE)
target_include_directories(embedded_resources INTERFACE "${GENERATED_DIR}")
