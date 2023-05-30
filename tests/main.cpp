//
//  main.cpp
//  opencl-language-server-tests
//
//  Created by Ilya Shoshin (Galarius) on 7/16/21.
//

#include <catch2/catch_all.hpp>

#include "jsonrpc.hpp"
#define __glogger_implementation__ // define this only once
#include "diagnostics.hpp"
#include <glogger.hpp>
#include <nlohmann/json.hpp>

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

} // namespace

TEST_CASE("JSON-RPC TESTS", "<->")
{
    // EnableTracing();

    SECTION("Invalid request handling")
    {
        const std::string request = R"!({"jsonrpc: 2.0", "id":0, [method]: "initialize"})!";
        const std::string message = BuildRequest(request);
        JsonRPC jrpc;
        jrpc.RegisterOutputCallback([](const std::string& message) {
            auto response = json::parse(ParseResponse(message));
            const auto code = response["error"]["code"].get<int64_t>();
            REQUIRE(code == static_cast<int64_t>(JsonRPC::ErrorCode::ParseError));
        });
        Send(message, jrpc);
    }

    SECTION("Out of order request")
    {
        const std::string message = BuildRequest(
            json::object({{"jsonrpc", "2.0"}, {"id", 0}, {"method", "textDocument/didOpen"}, {"params", {}}}));

        JsonRPC jrpc;
        jrpc.RegisterOutputCallback([](const std::string& message) {
            auto response = json::parse(ParseResponse(message));
            const auto code = response["error"]["code"].get<int64_t>();
            REQUIRE(code == static_cast<int64_t>(JsonRPC::ErrorCode::NotInitialized));
        });
        Send(message, jrpc);
    }

    const std::string initRequest = BuildRequest(json::object(
        {{"jsonrpc", "2.0"},
         {"id", 0},
         {"method", "initialize"},
         {"params", {{"processId", 60650}, {"trace", "off"}}}}));

    SECTION("Method/initialize")
    {
        JsonRPC jrpc;
        jrpc.RegisterOutputCallback([](const std::string&) {});
        jrpc.RegisterMethodCallback("initialize", [](const json& request) {
            const auto& processId = request["params"]["processId"].get<int64_t>();
            REQUIRE(processId == 60650);
        });
        Send(initRequest, jrpc);
    }

    const auto InitializeJsonRPC = [&initRequest](JsonRPC& jrpc) {
        jrpc.RegisterOutputCallback([](const std::string&) {});

        jrpc.RegisterMethodCallback("initialize", [](const json&) {});
        Send(initRequest, jrpc);
        jrpc.Reset();
    };

    SECTION("Respond to unsupported method")
    {
        JsonRPC jrpc;
        InitializeJsonRPC(jrpc);
        // send unsupported request
        const std::string request = BuildRequest(
            json::object({{"jsonrpc", "2.0"}, {"id", 0}, {"method", "textDocument/didOpen"}, {"params", {}}}));
        jrpc.RegisterOutputCallback([](const std::string& message) {
            auto response = json::parse(ParseResponse(message));
            const auto code = response["error"]["code"].get<int64_t>();
            REQUIRE(code == static_cast<int64_t>(JsonRPC::ErrorCode::MethodNotFound));
        });
        Send(request, jrpc);
    }
}
