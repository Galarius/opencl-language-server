//
//  main.cpp
//  opencl-language-server
//
//  Created by Ilya Shoshin (Galarius) on 7/14/21.
//

#include <iostream>

#include "clinfo.hpp"
#include "diagnostics.hpp"
#include "jsonrpc.hpp"
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

using namespace ocls;

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
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
        spdlog::error("Cannot set stdin mode to _O_BINARY");
    }
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
        spdlog::error("Cannot set stdout mode to _O_BINARY");
    }
#endif
}

} // namespace

int main(int argc, char* argv[])
{
    bool flagLogTofile = false;
    std::string optLogFile = "opencl-language-server.log";
    spdlog::level::level_enum optLogLevel = spdlog::level::trace;

    CLI::App app {
        "OpenCL Language Server\n"
        "The language server communicates with a client using JSON-RPC protocol.\n"
        "You can stop the server by sending an interrupt signal followed by any character sent to standard input.\n"
    };
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
            std::cout << ocls::version << std::endl;
            exit(0);
        },
        "Show version");
    
    bool flagPrettyPrint = false;
    auto clinfoCmd = app.add_subcommand("clinfo", "Show information about available OpenCL devices");
    clinfoCmd->add_flag("-p,--pretty-print", flagPrettyPrint, "Enable pretty-printing");

    CLI11_PARSE(app, argc, argv);

    ConfigureLogging(flagLogTofile, optLogFile, optLogLevel);

    auto clinfo = CreateCLInfo();
    if (*clinfoCmd)
    {
        const auto jsonBody = clinfo->json();
        const int indentation = flagPrettyPrint ? 4 : -1;
        std::cout << jsonBody.dump(indentation) << std::endl;
        return 0;
    }

    SetupBinaryStreamMode();

    std::signal(SIGINT, SignalHandler);

    auto jrpc = CreateJsonRPC();
    auto diagnostics = CreateDiagnostics(clinfo);
    server = CreateLSPServer(jrpc, diagnostics);
    return server->Run();
}
