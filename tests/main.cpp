//
//  main.cpp
//  opencl-language-server-tests
//
//  Created by Ilya Shoshin (Galarius) on 7/16/21.
//

#include <gtest/gtest.h>

#include "diagnostics.hpp"
#include "jsonrpc.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>

using namespace vscode::opencl;
using namespace nlohmann;

namespace {

std::string BuildRequest(const std::string& content)
{
    std::string request;
    request.append("Content-Length: " + std::to_string(content.size()) + "\r\n");
    request.append("Content-Type: application/vscode-jsonrpc;charset=utf-8\r\n");
    request.append("\r\n");
    request.append(content);
    return request;
}

std::string BuildRequest(const json& obj)
{
    return BuildRequest(obj.dump());
}

void Send(const std::string& request, JsonRPC& jrpc)
{
    for (auto c : request)
        jrpc.Consume(c);
}

std::string ParseResponse(std::string str)
{
    std::string delimiter = "\r\n";
    size_t pos = 0;
    while ((pos = str.find(delimiter)) != std::string::npos)
    {
        str.erase(0, pos + delimiter.length());
    }
    return str;
}

const std::string initRequest = BuildRequest(json::object(
        {{"jsonrpc", "2.0"},
         {"id", 0},
         {"method", "initialize"},
         {"params", {{"processId", 60650}, {"trace", "off"}}}}));

const auto InitializeJsonRPC = [](JsonRPC& jrpc) {
    jrpc.RegisterOutputCallback([](const std::string&) {});
    jrpc.RegisterMethodCallback("initialize", [](const json&) {});
    Send(initRequest, jrpc);
    jrpc.Reset();
};

} // namespace

TEST(JsonRPCTest, InvalidRequestHandling)
{
    const std::string request = R"!({"jsonrpc: 2.0", "id":0, [method]: "initialize"})!";
    const std::string message = BuildRequest(request);
    JsonRPC jrpc;
    jrpc.RegisterOutputCallback([](const std::string& message) {
        auto response = json::parse(ParseResponse(message));
        const auto code = response["error"]["code"].get<int64_t>();
        EXPECT_EQ(code, static_cast<int64_t>(JsonRPC::ErrorCode::ParseError));
    });
    Send(message, jrpc);
}

TEST(JsonRPCTest, OutOfOrderRequest)
{
    const std::string message = BuildRequest(
        json::object({{"jsonrpc", "2.0"}, {"id", 0}, {"method", "textDocument/didOpen"}, {"params", {}}}));

    JsonRPC jrpc;
    jrpc.RegisterOutputCallback([](const std::string& message) {
        auto response = json::parse(ParseResponse(message));
        const auto code = response["error"]["code"].get<int64_t>();
        EXPECT_EQ(code, static_cast<int64_t>(JsonRPC::ErrorCode::NotInitialized));
    });
    Send(message, jrpc);
}

TEST(JsonRPCTest, MethodInitialize)
{
    JsonRPC jrpc;
    jrpc.RegisterOutputCallback([](const std::string&) {});
    jrpc.RegisterMethodCallback("initialize", [](const json& request) {
        const auto& processId = request["params"]["processId"].get<int64_t>();
        EXPECT_EQ(processId, 60650);
    });
    Send(initRequest, jrpc);
}

TEST(JsonRPCTest, RespondToUnsupportedMethod)
{
    JsonRPC jrpc;
    InitializeJsonRPC(jrpc);
    // send unsupported request
    const std::string request = BuildRequest(
        json::object({{"jsonrpc", "2.0"}, {"id", 0}, {"method", "textDocument/didOpen"}, {"params", {}}}));
    jrpc.RegisterOutputCallback([](const std::string& message) {
        auto response = json::parse(ParseResponse(message));
        const auto code = response["error"]["code"].get<int64_t>();
        EXPECT_EQ(code, static_cast<int64_t>(JsonRPC::ErrorCode::MethodNotFound));
    });
    Send(request, jrpc);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    auto sink = std::make_shared<spdlog::sinks::null_sink_st>();
    auto mainLogger = std::make_shared<spdlog::logger>("opencl-language-server", sink);
    auto clinfoLogger = std::make_shared<spdlog::logger>("clinfo", sink);
    auto diagnosticsLogger = std::make_shared<spdlog::logger>("diagnostics", sink);
    auto jsonrpcLogger = std::make_shared<spdlog::logger>("jrpc", sink);
    auto lspLogger = std::make_shared<spdlog::logger>("lsp", sink);
    spdlog::set_default_logger(mainLogger);
    spdlog::register_logger(clinfoLogger);
    spdlog::register_logger(diagnosticsLogger);
    spdlog::register_logger(jsonrpcLogger);
    spdlog::register_logger(lspLogger);
    return RUN_ALL_TESTS();
}