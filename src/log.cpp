//
//  logs.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 20/08/23.
//

#include "log.hpp"

#include <iostream>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>

namespace ocls {

std::string LogName::main = "opencl-ls";
std::string LogName::clinfo = "clinfo";
std::string LogName::diagnostics = "diagnostics";
std::string LogName::jrpc = "jrpc";
std::string LogName::lsp = "lsp";

void ConfigureLogging(bool fileLogging, const std::string& filename, spdlog::level::level_enum level)
{
    try
    {
        spdlog::sink_ptr sink;
        if (fileLogging)
        {
            sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(filename);
        }
        else
        {
            sink = std::make_shared<spdlog::sinks::null_sink_st>();
        }

        spdlog::set_default_logger(std::make_shared<spdlog::logger>(LogName::main, sink));
        spdlog::set_level(level);
        std::vector<std::shared_ptr<spdlog::logger>> subLoggers = {
            std::make_shared<spdlog::logger>(LogName::clinfo, sink),
            std::make_shared<spdlog::logger>(LogName::diagnostics, sink),
            std::make_shared<spdlog::logger>(LogName::jrpc, sink),
            std::make_shared<spdlog::logger>(LogName::lsp, sink)};

        for (const auto& logger : subLoggers)
        {
            logger->set_level(level);
            spdlog::register_logger(logger);
        }
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cerr << "Log init failed: " << ex.what() << std::endl;
    }
}

void ConfigureFileLogging(const std::string& filename, spdlog::level::level_enum level)
{
    ConfigureLogging(true, filename, level);
}

void ConfigureNullLogging()
{
    ConfigureLogging(false, "", spdlog::level::trace);
}

} // namespace ocls
