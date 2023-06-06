//
//  main.cpp
//  opencl-language-server
//
//  Created by Ilya Shoshin (Galarius) on 7/14/21.
//

#include <iostream>

#include "clinfo.hpp"
#include "lsp.hpp"
#include "version.hpp"

#include <CLI/CLI.hpp>
#include <csignal>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>

#if defined(WIN32)
    #include <fcntl.h>
    #include <io.h>
    #include <stdio.h>
#endif

using namespace vscode::opencl;

namespace {

std::shared_ptr<ILSPServer> server;

static void SignalHandler(int)
{
    std::cout << "Interrupt signal received. Press any key to exit." << std::endl;
    if (server)
    {
        server->Interrupt();
    }
}

void ConfigureLogging(bool fileLogging, std::string filename, spdlog::level::level_enum level)
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
        spdlog::set_default_logger(std::make_shared<spdlog::logger>("opencl-ls", sink));
        spdlog::set_level(level);
        std::vector<std::shared_ptr<spdlog::logger>> subLoggers = {
            std::make_shared<spdlog::logger>("clinfo", sink),
            std::make_shared<spdlog::logger>("diagnostics", sink),
            std::make_shared<spdlog::logger>("jrpc", sink),
            std::make_shared<spdlog::logger>("lsp", sink)};
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

inline void SetupBinaryStreamMode()
{
#if defined(WIN32)
    // to handle CRLF
    if (_setmode(_fileno(stdin), _O_BINARY) == -1)
        GLogError("Cannot set stdin mode to _O_BINARY");
    if (_setmode(_fileno(stdout), _O_BINARY) == -1)
        GLogError("Cannot set stdout mode to _O_BINARY");
#endif
}

} // namespace

int main(int argc, char* argv[])
{
    bool flagLogTofile = false;
    bool flagCLInfo = false;
    std::string optLogFile = "opencl-language-server.log";
    spdlog::level::level_enum optLogLevel = spdlog::level::trace;

    CLI::App app {"OpenCL Language Server"};
    app.add_flag("-i,--clinfo", flagCLInfo, "Show information about available OpenCL devices");
    app.add_flag("-e,--enable-file-logging", flagLogTofile, "Enable file logging");
    app.add_option("-f,--log-file", optLogFile, "Path to log file")->required(false)->capture_default_str();
    app.add_option("-l,--log-level", optLogLevel, "Log level")
        ->check(CLI::IsMember(
            {spdlog::level::trace,
             spdlog::level::debug,
             spdlog::level::info,
             spdlog::level::warn,
             spdlog::level::err,
             spdlog::level::critical}))
        ->required(false)
        ->capture_default_str();
    app.add_flag_callback(
        "-v,--version",
        []() {
            std::cout << vscode::opencl::version << std::endl;
            exit(0);
        },
        "Show version");

    CLI11_PARSE(app, argc, argv);

    ConfigureLogging(flagLogTofile, optLogFile, optLogLevel);

    if (flagCLInfo)
    {
        const auto clinfo = CreateCLInfo();
        const auto jsonBody = clinfo->json();
        std::cout << jsonBody.dump() << std::endl;
        exit(0);
    }

    SetupBinaryStreamMode();

    std::signal(SIGINT, SignalHandler);

    server = CreateLSPServer();
    return server->Run();
}
