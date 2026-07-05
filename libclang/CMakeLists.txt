# Usage: 
# add_subdirectory(libclang)
# target_link_libraries(${TARGET} libclang-imported)

if(DEFINED CMAKE_SYSTEM_PROCESSOR)
  set(_proc ${CMAKE_SYSTEM_PROCESSOR})
elseif(DEFINED CMAKE_HOST_SYSTEM_PROCESSOR)
  set(_proc ${CMAKE_HOST_SYSTEM_PROCESSOR})
else()
  set(_proc "")
endif()

string(TOLOWER "${_proc}" _proc_l)
set(IS_ARM64 OFF)
if(_proc_l MATCHES "aarch64|arm64|armv8")
  set(IS_ARM64 ON)
endif()

message(STATUS "ARCH: ${_proc}")

# Download prebuilt static llvm & clang libraries
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/libclang-urls.txt libclang_entries)
message(STATUS "Parsing ${CMAKE_CURRENT_SOURCE_DIR}/libclang-urls.txt...")
foreach(entry ${libclang_entries})
  # Parse platform, URL, and hash
  list(GET entry 0 platform)
  list(GET entry 1 arch)
  list(GET entry 2 url_value)
  list(GET entry 3 hash_value)

  if(platform STREQUAL "APPLE" AND APPLE)
    set(libclang_url ${url_value})
    set(libclang_hash ${hash_value})
    break()
  elseif(platform STREQUAL "WIN32" AND WIN32)

    # Visual Studio generator may set CMAKE_VS_PLATFORM_NAME to "ARM64"
    if(IS_ARM64 STREQUAL "OFF" AND DEFINED CMAKE_VS_PLATFORM_NAME)
      string(TOLOWER "${CMAKE_VS_PLATFORM_NAME}" _vsplat_l)
      if(_vsplat_l STREQUAL "arm64")
        set(IS_ARM64 ON)
      endif()
    endif()

    if(NOT IS_ARM64 AND arch MATCHES "x86_64|x64|amd64")
      set(libclang_url ${url_value})
      set(libclang_hash ${hash_value})
      break()
    elseif(arch STREQUAL _proc_l)
      set(libclang_url ${url_value})
      set(libclang_hash ${hash_value})
      break()
    endif()

  elseif(platform STREQUAL "LINUX" AND LINUX)
  
    if(IS_ARM64 AND arch MATCHES "aarch64|arm64|armv8")
      set(libclang_url ${url_value})
      set(libclang_hash ${hash_value})
      break()
    elseif(arch STREQUAL _proc_l)
      set(libclang_url ${url_value})
      set(libclang_hash ${hash_value})
      break()
    endif()

  endif()
endforeach()

message(STATUS "Detected ARM64: ${IS_ARM64}")
message(STATUS "LIBCLANG URL: ${libclang_url}")
message(STATUS "LIBCLANG HASH: ${libclang_hash}")

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

if(NOT DEFINED LIBCLANG_CACHE_DIR)
  set(LIBCLANG_CACHE_DIR "${CMAKE_SOURCE_DIR}/.libclang-cache")
endif()

FetchContent_Declare(libclang
    SOURCE_DIR "${LIBCLANG_CACHE_DIR}/prebuild"
    DOWNLOAD_DIR "${LIBCLANG_CACHE_DIR}/download"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    TLS_VERIFY TRUE
    URL ${libclang_url}
    URL_HASH SHA256=${libclang_hash}
)
FetchContent_MakeAvailable(libclang)
set(LLVM_ROOT_DIR "${LIBCLANG_CACHE_DIR}/prebuild")
set(LLVM_ROOT_DIR "${LLVM_ROOT_DIR}" PARENT_SCOPE)

set(cmake_llvm "${LLVM_ROOT_DIR}/lib/cmake/llvm")
set(cmake_clang "${LLVM_ROOT_DIR}/lib/cmake/clang")

message(STATUS "LLVM root directory: ${LLVM_ROOT_DIR}")

list(APPEND CMAKE_MODULE_PATH ${cmake_llvm} ${cmake_clang})
include(LLVMConfig)
include(ClangConfig)

if(WIN32)
  # Workaround to replace the hardcoded path to diaguids.lib
  if(CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    set(DIA_PLATFORM_NAME "amd64")
  else()
    set(DIA_PLATFORM_NAME "")
  endif()

  find_library(
    diasdk_lib
    NAMES diaguids.lib
    PATH_SUFFIXES "lib/${DIA_PLATFORM_NAME}"
    HINTS "$ENV{VSINSTALLDIR}/DIA SDK" "${CMAKE_GENERATOR_INSTANCE}/DIA SDK")

  message(STATUS "diasdk_lib: ${diasdk_lib}")

  if(NOT EXISTS ${diasdk_lib})
    message(FATAL_ERROR "The file ${diasdk_lib} does not exist")
  endif()
  
  get_target_property(link_libraries LLVMDebugInfoPDB INTERFACE_LINK_LIBRARIES)
  list(FIND link_libraries "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/DIA SDK/lib/amd64/diaguids.lib" index)
  if(index GREATER -1)
    list(REMOVE_AT link_libraries ${index})
    list(INSERT link_libraries ${index} "${diasdk_lib}")
  endif()
  set_target_properties(LLVMDebugInfoPDB PROPERTIES
    INTERFACE_LINK_LIBRARIES "${link_libraries}"
  )
  message(STATUS "LLVMDebugInfoPDB: ${link_libraries}")
endif()

message(STATUS "LLVM targets to build: ${LLVM_TARGETS_TO_BUILD}")
message(STATUS "LLVM native arch: ${LLVM_NATIVE_ARCH}")
message(STATUS "LLVM include directories: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")
message(STATUS "LLVM assertions: ${LLVM_ENABLE_ASSERTIONS}")
message(STATUS "LLVM eh: ${LLVM_ENABLE_EH}")
message(STATUS "LLVM rtti: ${LLVM_ENABLE_RTTI}")

# Prepare the list of definitions before passing them to the INTERFACE_COMPILE_DEFINITIONS property
separate_arguments(llvm_definitions_list NATIVE_COMMAND ${LLVM_DEFINITIONS})
foreach(definition ${llvm_definitions_list})
  # Remove the prefix -D from the current definition
  string(REGEX REPLACE "^-D" "" new_definition ${definition})
  # Replace the current definition with the new definition (without the "-D' prefix)
  list(APPEND new_llvm_definitions ${new_definition})
endforeach()
set(llvm_definitions_list ${new_llvm_definitions})

if(WIN32)
  # Import functions from static library, not from DLL
  list(APPEND llvm_definitions_list "CINDEX_NO_EXPORTS")
endif()

message(STATUS "LLVM parsed definitions: ${llvm_definitions_list}")

set(llvm_components ${LLVM_NATIVE_ARCH} core)
llvm_map_components_to_libnames(llvm_libs ${llvm_components})
set(clang_libraries clangAnalysis clangAnalysisFlowSensitive clangAnalysisFlowSensitiveModels clangAPINotes clangARCMigrate clangAST clangASTMatchers clangBasic clangCodeGen clangCrossTU clangDependencyScanning clangDirectoryWatcher clangDriver clangDynamicASTMatchers clangEdit clangExtractAPI clangFormat clangFrontend clangFrontendTool clangHandleCXX clangHandleLLVM clangIndex clangIndexSerialization clangInterpreter clangLex clangParse clangRewrite clangRewriteFrontend clangSema clangSerialization clangStaticAnalyzerCheckers clangStaticAnalyzerCore clangStaticAnalyzerFrontend clangSupport clangTooling clangToolingASTDiff clangToolingCore clangToolingInclusions clangToolingRefactoring clangToolingSyntax clangTransformer)

message(STATUS "LLVM libraries: ${llvm_libs}")
message(STATUS "CLANG libraries: ${clang_libraries}")
set(deps_libs ${llvm_libs})
list(APPEND deps_libs ${clang_libraries})

add_library(libclang-imported STATIC IMPORTED GLOBAL)
set_target_properties(libclang-imported 
  PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${LLVM_INCLUDE_DIRS}"
  INTERFACE_COMPILE_DEFINITIONS "${llvm_definitions_list}"
  IMPORTED_LOCATION "${LLVM_ROOT_DIR}/lib/libclang${CMAKE_STATIC_LIBRARY_SUFFIX}"
  IMPORTED_LINK_INTERFACE_LIBRARIES "${deps_libs}"
)
