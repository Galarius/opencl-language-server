set(TESTS_PROJECT_NAME ${PROJECT_NAME}-tests)
set(sources
    diagnostics.cpp
    jsonrpc.cpp
    lsp.cpp
    utils.cpp
)
list(TRANSFORM sources PREPEND "${PROJECT_SOURCE_DIR}/src/")
set(test_sources
    jsonrpc-tests.cpp
    diagnostics-parser-tests.cpp
    lsp-event-handler-tests.cpp
    main.cpp
)
set(libs GTest::gmock nlohmann_json::nlohmann_json spdlog::spdlog OpenCL::HeadersCpp uriparser::uriparser)
if(LINUX)
    set(libs ${libs} stdc++fs OpenCL::OpenCL)
elseif(APPLE)
    set(libs ${libs} ${OpenCL_LIBRARIES})
elseif(WIN32)
    set(libs ${libs} OpenCL::OpenCL)
endif()

add_executable (${TESTS_PROJECT_NAME} ${sources} ${test_sources})
target_compile_definitions(${TESTS_PROJECT_NAME} PRIVATE 
    CL_HPP_ENABLE_EXCEPTIONS
    CL_HPP_CL_1_2_DEFAULT_BUILD
)
if(APPLE)
target_compile_definitions(${TESTS_PROJECT_NAME} PRIVATE 
    CL_HPP_MINIMUM_OPENCL_VERSION=120
    CL_HPP_TARGET_OPENCL_VERSION=120
)
else()
target_compile_definitions(${TESTS_PROJECT_NAME} PRIVATE 
    CL_HPP_MINIMUM_OPENCL_VERSION=110
    CL_HPP_TARGET_OPENCL_VERSION=300
)
endif()

target_link_libraries (${TESTS_PROJECT_NAME} ${libs})
target_include_directories(${TESTS_PROJECT_NAME} PRIVATE 
    "${PROJECT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/mocks"
)
if(APPLE)
    target_include_directories(${TESTS_PROJECT_NAME} PRIVATE "${OpenCL_INCLUDE_DIRS}")
endif()

gtest_discover_tests(${TESTS_PROJECT_NAME})