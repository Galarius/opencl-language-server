//
//  diagnostics-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/25/23.
//

#pragma once

#include "diagnostics.hpp"

#include <gmock/gmock.h>

class DiagnosticsMock : public ocls::IDiagnostics
{
public:
    MOCK_METHOD(void, SetBuildOptions, (const nlohmann::json&), (override));

    MOCK_METHOD(void, SetBuildOptions, (const std::string&), (override));

    MOCK_METHOD(void, SetMaxProblemsCount, (uint64_t), (override));

    MOCK_METHOD(void, SetOpenCLDevice, (uint32_t), (override));

    MOCK_METHOD(std::optional<ocls::Device>, GetDevice, (), (const, override));

    MOCK_METHOD(std::string, GetBuildLog, (const ocls::Source&), (override));

    MOCK_METHOD(nlohmann::json, GetDiagnostics, (const ocls::Source&), (override));
};
