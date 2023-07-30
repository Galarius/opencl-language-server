//
//  lsp-event-handler-tests.cpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/25/23.
//

#include "lsp.hpp"
#include "utils.hpp"
#include "jsonrpc-mock.hpp"
#include "diagnostics-mock.hpp"
#include "generator-mock.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace ocls;
using namespace nlohmann;

namespace {

class LSPTest : public ::testing::Test
{
protected:
    std::shared_ptr<JsonRPCMock> mockJsonRPC;
    std::shared_ptr<DiagnosticsMock> mockDiagnostics;
    std::shared_ptr<GeneratorMock> mockGenerator;
    std::shared_ptr<ILSPServerEventsHandler> handler;

    void SetUp() override
    {
        mockJsonRPC = std::make_shared<JsonRPCMock>();
        mockDiagnostics = std::make_shared<DiagnosticsMock>();
        mockGenerator = std::make_shared<GeneratorMock>();

        ON_CALL(*mockGenerator, GenerateID()).WillByDefault(::testing::Return("12345678"));

        handler = CreateLSPEventsHandler(mockJsonRPC, mockDiagnostics, mockGenerator);
    }
};

} // namespace


// OnInitialize

TEST_F(LSPTest, OnInitialize_shouldBuildResponse_andCallDiagnosticsSetters)
{
    nlohmann::json testData = R"({
        "params": {
            "capabilities": {
                "workspace": {
                    "configuration": true,
                    "didChangeConfiguration": {"dynamicRegistration": true}
                }
            },
            "initializationOptions": {
                "configuration": {
                    "buildOptions": { "option": "value" },
                    "maxNumberOfProblems": 10,
                    "deviceID": 1
                }
            }
        },
        "id": 1
    })"_json;

    nlohmann::json expectedResponse = R"({
        "id": 1,
        "result": {
            "capabilities": {
                "textDocumentSync": {
                    "openClose": true,
                    "change": 1,
                    "willSave": false,
                    "willSaveWaitUntil": false,
                    "save": false
                }
            }
        }
    })"_json;

    EXPECT_CALL(
        *mockDiagnostics, SetBuildOptions(testData["params"]["initializationOptions"]["configuration"]["buildOptions"]))
        .Times(1);
    EXPECT_CALL(*mockDiagnostics, SetMaxProblemsCount(10)).Times(1);
    EXPECT_CALL(*mockDiagnostics, SetOpenCLDevice(1)).Times(1);

    handler->OnInitialize(testData);
    auto response = handler->GetNextResponse();

    EXPECT_TRUE(response.has_value());
    EXPECT_EQ(*response, expectedResponse);
}

TEST_F(LSPTest, OnInitialize_withMissingConfigurationFields_shouldBuildResponse_andNotCallDiagnosticsSetters)
{
    nlohmann::json testData = R"({
        "params": {
            "capabilities": {
                "workspace": {
                    "configuration": true,
                    "didChangeConfiguration": {"dynamicRegistration": true}
                }
            },
            "initializationOptions": {
                "configuration": {

                }
            }
        },
        "id": "1"
    })"_json;

    nlohmann::json expectedResponse = R"({
        "id": "1",
        "result": {
            "capabilities": {
                "textDocumentSync": {
                    "openClose": true,
                    "change": 1,
                    "willSave": false,
                    "willSaveWaitUntil": false,
                    "save": false
                }
            }
        }
    })"_json;

    handler->OnInitialize(testData);
    auto response = handler->GetNextResponse();

    EXPECT_TRUE(response.has_value());
    EXPECT_EQ(*response, expectedResponse);
}

// OnInitialized

TEST_F(LSPTest, OnInitialized_withDidChangeConfigurationSupport_shouldBuildResponse)
{
    nlohmann::json initData = R"({
        "params": {
            "capabilities": {
                "workspace": {
                    "didChangeConfiguration": {"dynamicRegistration": true}
                }
            },
            "initializationOptions": {
                "configuration": {
                }
            }
        },
        "id": "1"
    })"_json;

    nlohmann::json expectedResponse = R"({
        "id": "1",
        "method": "client/registerCapability",
        "params": {
            "registrations": [{
                "id": "12345678",
                "method": "workspace/didChangeConfiguration"
            }]
        }
    })"_json;

    handler->OnInitialize(initData);
    handler->GetNextResponse();

    EXPECT_CALL(*mockGenerator, GenerateID()).Times(1);

    handler->OnInitialized(R"({"id": "1"})"_json);
    auto response = handler->GetNextResponse();

    EXPECT_TRUE(response.has_value());
    EXPECT_EQ(*response, expectedResponse);
}

TEST_F(LSPTest, OnInitialized_withoutDidChangeConfigurationSupport_shouldNotBuildResponse)
{
    nlohmann::json initData = R"({
        "params": {
            "capabilities": {
                "workspace": {
                    "didChangeConfiguration": {"dynamicRegistration": false}
                }
            },
            "initializationOptions": {
                "configuration": {
                }
            }
        },
        "id": "1"
    })"_json;

    handler->OnInitialize(initData);
    handler->GetNextResponse();

    handler->OnInitialized(R"({"id": "1"})"_json);
    auto response = handler->GetNextResponse();

    EXPECT_FALSE(response.has_value());
}
