//
//  lsp-event-handler-tests.cpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/25/23.
//

#include "lsp.hpp"
#include "jsonrpc-mock.hpp"
#include "diagnostics-mock.hpp"

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
    std::shared_ptr<ILSPServerEventsHandler> handler;

    void SetUp() override
    {
        mockJsonRPC = std::make_shared<JsonRPCMock>();
        mockDiagnostics = std::make_shared<DiagnosticsMock>();
        handler = CreateLSPEventsHandler(mockJsonRPC, mockDiagnostics);
    }
};

} // namespace


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

    handler->OnInitialize(testData);
    auto response = handler->GetNextResponse();

    EXPECT_TRUE(response.has_value());
    EXPECT_EQ(*response, expectedResponse);
}
