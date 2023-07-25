//
//  main.cpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/16/21.
//

#include <gtest/gtest.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>

int main(int argc, char** argv)
{
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