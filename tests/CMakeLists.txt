set(TESTS_PROJECT_NAME ${PROJECT_NAME}-tests)
set(headers
    "${PROJECT_SOURCE_DIR}/include/jsonrpc.hpp"
)
set(sources
    "${PROJECT_SOURCE_DIR}/src/jsonrpc.cpp"
    main.cpp
)
set(libs GTest::gtest nlohmann_json::nlohmann_json spdlog::spdlog OpenCL::HeadersCpp)
if(LINUX)
    set(libs ${libs} stdc++fs OpenCL::OpenCL)
elseif(APPLE)
    set(libs ${libs} ${OpenCL_LIBRARIES})
elseif(WIN32)
    set(libs ${libs} OpenCL::OpenCL)
endif()

add_executable (${TESTS_PROJECT_NAME} ${headers} ${sources})
target_include_directories(${TESTS_PROJECT_NAME} PRIVATE 
    "${PROJECT_SOURCE_DIR}/include"
)
if(APPLE)
    target_include_directories(${TESTS_PROJECT_NAME} PRIVATE "${OpenCL_INCLUDE_DIRS}")
endif()
target_link_libraries (${TESTS_PROJECT_NAME} ${libs})
target_compile_definitions(${TESTS_PROJECT_NAME} PRIVATE 
    CL_HPP_CL_1_2_DEFAULT_BUILD
    CL_HPP_TARGET_OPENCL_VERSION=120
    CL_HPP_MINIMUM_OPENCL_VERSION=120
)
gtest_discover_tests(${TESTS_PROJECT_NAME})