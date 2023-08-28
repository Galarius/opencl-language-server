//
//  jsonrpc-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/25/23.
//

#pragma once

#include "jsonrpc.hpp"

#include <gmock/gmock.h>

class JsonRPCMock final : public ocls::IJsonRPC
{
public:
    MOCK_METHOD(void, RegisterMethodCallback, (const std::string&, ocls::InputCallbackFunc&&), (override));

    MOCK_METHOD(void, RegisterInputCallback, (ocls::InputCallbackFunc &&), (override));

    MOCK_METHOD(void, RegisterOutputCallback, (ocls::OutputCallbackFunc &&), (override));

    MOCK_METHOD(void, Consume, (char), (override));

    MOCK_METHOD(bool, IsReady, (), (const, override));

    MOCK_METHOD(void, Write, (const nlohmann::json&), (const, override));

    MOCK_METHOD(void, Reset, (), (override));

    MOCK_METHOD(void, WriteTrace, (const std::string&, const std::string&), (override));

    MOCK_METHOD(void, WriteError, (ocls::JRPCErrorCode, const std::string&), (const, override));
};