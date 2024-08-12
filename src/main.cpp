//
//  main.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/14/21.
//

#include <iostream>

#include "clinfo.hpp"
#include "diagnostics.hpp"
#include "jsonrpc.hpp"
#include "log.hpp"
#include "lsp.hpp"
#include "version.hpp"

#include <CLI/CLI.hpp>
#include <csignal>
#include <nlohmann/json.hpp>

#if defined(WIN32)
    #include <fcntl.h>
    #include <io.h>
    #include <stdio.h>
#endif

using namespace ocls;

namespace {

struct SubCommand
{
    SubCommand(CLI::App& app, std::string name, std::string description) : cmd {app.add_subcommand(name, description)}
    {}

    ~SubCommand() = default;

    bool IsParsed() const
    {
        return cmd->parsed();
    }

protected:
    CLI::App* cmd;
};

struct CLInfoSubCommand final : public SubCommand
{
    CLInfoSubCommand(CLI::App& app) : SubCommand(app, "clinfo", "Show information about available OpenCL devices")
    {
        cmd->add_flag("-p,--pretty-print", prettyPrint, "Enable pretty-printing");
    }

    int Execute(const std::shared_ptr<ICLInfo>& clinfo)
    {
        const auto jsonBody = clinfo->json();
        const int indentation = prettyPrint ? 4 : -1;
        std::cout << jsonBody.dump(indentation) << std::endl;
        return EXIT_SUCCESS;
    }

private:
    bool prettyPrint = false;
};

struct DiagnosticsSubCommand final : public SubCommand
{
    DiagnosticsSubCommand(CLI::App& app) : SubCommand(app, "diagnostics", "Provides an OpenCL kernel diagnostics")
    {
        cmd->add_flag("-j,--json", json, "Print diagnostics in JSON format");
        cmd->add_option("-k,--kernel", kernel, "Path to a kernel file")->required(true);
        cmd->add_option("-b,--build-options", buildOptions, "Options to be utilized when building the program.")
            ->capture_default_str();
        cmd->add_option(
               "-d,--device-id",
               deviceID,
               "Device ID or 0 (automatic selection) of the OpenCL device to be used for diagnostics.")
            ->capture_default_str();
        cmd->add_option("--error-limit", maxNumberOfProblems, "The maximum number of errors parsed by the compiler.")
            ->capture_default_str();
    }

    int Execute(const std::shared_ptr<IDiagnostics>& diagnostics)
    {
        auto content = utils::ReadFileContent(kernel);
        if (!content.has_value())
        {
            return EXIT_FAILURE;
        }

        try
        {
            if (!buildOptions.empty())
            {
                diagnostics->SetBuildOptions(buildOptions);
            }

            if (deviceID > 0)
            {
                diagnostics->SetOpenCLDevice(deviceID);
            }

            if (maxNumberOfProblems != INT8_MAX)
            {
                diagnostics->SetMaxProblemsCount(maxNumberOfProblems);
            }

            Source source {kernel, *content};
            if (json)
            {
                auto output = diagnostics->GetDiagnostics(source);
                std::cout << output.dump(4) << std::endl;
            }
            else
            {
                auto output = diagnostics->GetBuildLog(source);
                std::cout << output << std::endl;
            }
        }
        catch (std::exception& err)
        {
            std::cerr << "Failed to get diagnostics: " << err.what() << std::endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

private:
    std::string kernel;
    std::string buildOptions;
    uint32_t deviceID = 0;
    uint64_t maxNumberOfProblems = INT8_MAX;
    bool json = false;
};

std::shared_ptr<ILSPServer> server;

static void SignalHandler(int)
{
    std::cout << "Interrupt signal received. Press any key to exit." << std::endl;
    if (server)
    {
        server->Interrupt();
    }
}

inline void SetupBinaryStreamMode()
{
#if defined(WIN32)
    // to handle CRLF
    if (_setmode(_fileno(stdin), _O_BINARY) == -1)
    {
        spdlog::error("Cannot set stdin mode to _O_BINARY");
    }
    if (_setmode(_fileno(stdout), _O_BINARY) == -1)
    {
        spdlog::error("Cannot set stdout mode to _O_BINARY");
    }
#endif
}

} // namespace

int main(int argc, char* argv[])
{
    bool flagLogTofile = false;
    bool flagStdioMode = true;
    std::string optLogFile = "opencl-language-server.log";
    spdlog::level::level_enum optLogLevel = spdlog::level::trace;

    CLI::App app {"OpenCL Language Server\n"
                  "The language server communicates with a client using JSON-RPC protocol.\n"
                  "Stop the server by sending an interrupt signal followed by any character sent to standard input.\n"
                  "Optionally, you can use subcommands to access functionality without starting the server.\n"};
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
    app.add_flag("--stdio", flagStdioMode, "Use stdio transport channel for the language server");
    app.add_flag_callback(
        "-v,--version",
        []() {
            std::cout << ocls::version << std::endl;
            exit(EXIT_SUCCESS);
        },
        "Show version");

    CLInfoSubCommand clInfoCmd(app);
    DiagnosticsSubCommand diagnosticsCmd(app);
    CLI11_PARSE(app, argc, argv);
    if(flagLogTofile)
    {
        ConfigureFileLogging(optLogFile, optLogLevel);
    }
    else
    {
        ConfigureNullLogging();
    }

    auto clinfo = CreateCLInfo();

    int result = 0;
    do {
        if (clInfoCmd.IsParsed())
        {
            result = clInfoCmd.Execute(clinfo);
            break;
        }

        auto diagnostics = CreateDiagnostics(clinfo);
        if (diagnosticsCmd.IsParsed())
        {
            result = diagnosticsCmd.Execute(diagnostics);
            break;
        }

        SetupBinaryStreamMode();
        std::signal(SIGINT, SignalHandler);

        auto jrpc = CreateJsonRPC();
        server = CreateLSPServer(jrpc, diagnostics);
        result = server->Run();

    } while (false);

    spdlog::get(LogName::main)->info("Shutting down...\n\n");
    spdlog::shutdown();
    return result;
}
