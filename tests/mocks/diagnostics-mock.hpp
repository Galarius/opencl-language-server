//
//  diagnostics-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/25/23.
//

#include "diagnostics.hpp"

#include <gmock/gmock.h>

class DiagnosticsMock : public ocls::IDiagnostics
{
public:
    MOCK_METHOD(void, SetBuildOptions, (const nlohmann::json&), (override));

    MOCK_METHOD(void, SetMaxProblemsCount, (int), (override));

    MOCK_METHOD(void, SetOpenCLDevice, (uint32_t), (override));

    MOCK_METHOD(nlohmann::json, Get, (const ocls::Source&), (override));
};
