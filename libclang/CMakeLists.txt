# Usage: 
# add_subdirectory(libclang)
# target_link_libraries(${TARGET} libclang-imported)

if(WIN32)
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
endif()

# Define -DLLVM_ROOT_DIR=<path> if you have previously downloaded libclang manually
if(DEFINED LLVM_ROOT_DIR AND NOT LLVM_ROOT_DIR STREQUAL "")
  # LLVM_ROOT_DIR is already defined
else()

  # Download prebuilt static llvm & clang libraries

  include(FetchContent)
  set(FETCHCONTENT_QUIET OFF)

  if(APPLE)
    set(LIBCLANG_URL "https://download.qt.io/development_releases/prebuilt/libclang/qt/libclang-release_15.0.0-based-mac.7z")
    set(LIBCLANG_HASH "8a944b4b65d2dca883fe539af5e5eef9a67e2abca91b28258fb08c46e3bb05b6")
  elseif(WIN32)
    set(LIBCLANG_URL "https://download.qt.io/development_releases/prebuilt/libclang/qt/libclang-release_15.0.0-based-windows-vs2019_64.7z")
    set(LIBCLANG_HASH "3cd73b31be503ec7281f1f8907f95392d1c8bd0a8c2c92c39dc177f8c3d02baf")
  else()
    set(LIBCLANG_URL "https://download.qt.io/development_releases/prebuilt/libclang/qt/libclang-release_15.0.0-based-linux-Ubuntu20.04-gcc9.3-x86_64.7z")
    set(LIBCLANG_HASH "0cfdf3387b8917e99fbd81358a142daeb3bcb84c107c96f1508f257ee2082bda")
  endif()

  FetchContent_Declare(libclang
      SOURCE_DIR prebuild
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
      TLS_VERIFY TRUE
      URL ${LIBCLANG_URL}
      URL_HASH SHA256=${LIBCLANG_HASH}
    )
    FetchContent_MakeAvailable(libclang)
    set(LLVM_ROOT_DIR ${libclang_SOURCE_DIR})

endif()

message(STATUS "LLVM root directory: ${LLVM_ROOT_DIR}")

set(cmake_llvm "${LLVM_ROOT_DIR}/lib/cmake/llvm")
set(cmake_clang "${LLVM_ROOT_DIR}/lib/cmake/clang")
list(APPEND CMAKE_MODULE_PATH ${cmake_llvm} ${cmake_clang})
include(LLVMConfig)
include(ClangConfig)

if(WIN32)
  # Workaround to replace the hardcoded path to diaguids.lib
  
  if(EXISTS ${diasdk_lib})
    # Pass
  else()
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

set(llvm_components ${LLVM_NATIVE_ARCH} ${LLVM_NATIVE_ARCH}TargetMCA core)
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
