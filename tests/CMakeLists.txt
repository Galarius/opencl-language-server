set(TESTS_PROJECT_NAME ${PROJECT_NAME}-tests)
set(headers
    "${PROJECT_SOURCE_DIR}/include/jsonrpc.hpp"
)
set(sources
    "${PROJECT_SOURCE_DIR}/src/jsonrpc.cpp"
    main.cpp
)
set(libs ${OpenCL_LIBRARIES} Catch2::Catch2WithMain nlohmann_json::nlohmann_json spdlog::spdlog)
if(LINUX)
    set(libs ${libs} stdc++fs)
endif()

add_executable (${TESTS_PROJECT_NAME} ${headers} ${sources})
target_include_directories(${TESTS_PROJECT_NAME} PRIVATE 
    "${OpenCL_INCLUDE_DIRS}"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/lib/opencl/include"
)
target_link_libraries (${TESTS_PROJECT_NAME} ${libs})
target_compile_definitions(${TESTS_PROJECT_NAME} PRIVATE 
    CL_SILENCE_DEPRECATION
)
