//
//  logs.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 20/08/23.
//

#include "log.hpp"
#include "utils.hpp"

#if defined(__APPLE__)
#include "oslogger.hpp"
#endif

#include <iostream>

#include "spdlog/pattern_formatter.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>

namespace ocls {

std::string LogName::main = "opencl-ls";
std::string LogName::clinfo = "clinfo";
std::string LogName::diagnostics = "diagnostics";
std::string LogName::jrpc = "jrpc";
std::string LogName::lsp = "lsp";

const int flushIntervalSec = 5;

void ConfigureLogging(bool fileLogging, const std::string& filename, spdlog::level::level_enum level)
{
    try
    {
        std::vector<spdlog::sink_ptr> sinks;
        if (fileLogging)
        {
            sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename));
        }
        else
        {
            sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
        }
#if defined(__APPLE__)
        sinks.push_back(std::make_shared<ocls::oslogger_sink_mt>());
#endif

        spdlog::set_default_logger(std::make_shared<spdlog::logger>(LogName::main, sinks.begin(), sinks.end()));
        spdlog::set_level(level);
        std::vector<std::shared_ptr<spdlog::logger>> subLoggers = {
            std::make_shared<spdlog::logger>(LogName::clinfo, sinks.begin(), sinks.end()),
            std::make_shared<spdlog::logger>(LogName::diagnostics, sinks.begin(), sinks.end()),
            std::make_shared<spdlog::logger>(LogName::jrpc, sinks.begin(), sinks.end()),
            std::make_shared<spdlog::logger>(LogName::lsp, sinks.begin(), sinks.end())
        };

        for (const auto& logger : subLoggers)
        {
            logger->set_level(level);
            spdlog::register_logger(logger);
        }

        if (fileLogging)
        {
            spdlog::flush_on(level);
            spdlog::flush_every(std::chrono::seconds(flushIntervalSec));
        }

        spdlog::get(LogName::main)->info("Starting new session at {}...\n", utils::GetCurrentDateTime());
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
