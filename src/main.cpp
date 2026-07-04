//
//  main.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/14/21.
//

#include "commands.hpp"
#include "clinfo.hpp"
#include "completion.hpp"
#include "definition.hpp"
#include "typedef.hpp"
#include "declaration.hpp"
#include "diagnostics.hpp"
#include "jsonrpc.hpp"
#include "log.hpp"
#include "lsp.hpp"
#include "version.hpp"

#include <CLI/CLI.hpp>
#include <algorithm>
#include <csignal>
#include <list>
#include <nlohmann/json.hpp>

#if defined(WIN32)
    #include <fcntl.h>
    #include <io.h>
    #include <stdio.h>
#endif

using namespace ocls;

namespace {

auto logger()
{
    return spdlog::get(ocls::LogName::main);
}

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
        ->check(
            CLI::IsMember(
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

    std::list<std::shared_ptr<SubCommand>> subCommands {
        std::make_shared<CLInfoSubCommand>(app),
        std::make_shared<DiagnosticsSubCommand>(app),
        std::make_shared<CompletionSubCommand>(app),
        MakeDefinitionSubCommand(app),
        MakeDeclarationSubCommand(app),
        MakeTypeDefinitionSubCommand(app)};

    CLI11_PARSE(app, argc, argv);

    if (flagLogTofile)
    {
        ConfigureFileLogging(optLogFile, optLogLevel);
    }
    else
    {
        ConfigureNullLogging();
    }

    logger()->info("OpenCL Language Server {}", ocls::version);
    logger()->info("{}", GetClangVersion());

    int result = 0;
    do
    {
        auto it = std::find_if(subCommands.begin(), subCommands.end(), [](const std::shared_ptr<SubCommand>& command) {
            return command->IsParsed();
        });

        if (it != subCommands.end())
        {
            result = it->get()->Execute();
            break;
        }

        SetupBinaryStreamMode();
        std::signal(SIGINT, SignalHandler);

        auto jrpc = CreateJsonRPC();
        auto clinfo = CreateCLInfo();
        auto diagnostics = CreateDiagnostics(clinfo);
        auto device = diagnostics->GetDevice();
        auto clStandard = device ? device->GetCLStandard() : "CL";
        auto options = BuildDefaultTranslationOptions(clStandard);
        auto store = CreateTranslationUnitStore();
        auto completion = CreateCompletion(store);
        auto definition = CreateDefinition(store);
        auto typeDefinition = CreateTypeDefinition(store);
        auto declaration = CreateDeclaration(store);
        store->SaveHeaders();
        store->SetTranslationOptions(options);
        server = CreateLSPServer(jrpc, store, diagnostics, completion, definition, typeDefinition, declaration);
        result = server->Run();
    } while (false);

    logger()->info("Shutting down...\n\n");
    spdlog::shutdown();
    return result;
}
