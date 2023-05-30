//
//  main.cpp
//  opencl-language-server
//
//  Created by Ilya Shoshin (Galarius) on 7/14/21.
//

#include <iostream>

#include "clinfo.hpp"
#include "lsp.hpp"

#include <csignal>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>

#ifndef VERSION
    #error version is required
#endif

#if defined(WIN32)
    #include <fcntl.h>
    #include <io.h>
    #include <stdio.h>
#endif

using namespace vscode::opencl;

bool isArgOption(char** begin, char** end, const char* option);
char* getArgOption(char** begin, char** end, const char* option);

std::shared_ptr<ILSPServer> server;

static void SignalHandler(int)
{
    std::cout << "Interrupt signal received. Press any key to exit." << std::endl;
    if(server) {
        server->Interrupt();
    }
}

void ConfigureLogging(bool fileLogging, char *filename, char *levelStr) 
{
    auto level = spdlog::level::off;
    try
    {
        spdlog::sink_ptr sink;
        if (fileLogging)
        {   
            auto file = std::string(filename ?: "opencl-language-server.log");
            sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(file);
        }
        else 
        {
            sink = std::make_shared<spdlog::sinks::null_sink_st>();
        }

        auto mainLogger = std::make_shared<spdlog::logger>("opencl-language-server", sink);
        auto clinfoLogger = std::make_shared<spdlog::logger>("clinfo", sink);
        auto diagnosticsLogger = std::make_shared<spdlog::logger>("diagnostics", sink);
        auto jsonrpcLogger = std::make_shared<spdlog::logger>("jrpc", sink);
        auto lspLogger = std::make_shared<spdlog::logger>("lsp", sink);

        mainLogger->set_level(level);
        clinfoLogger->set_level(level);
        diagnosticsLogger->set_level(level);
        jsonrpcLogger->set_level(level);
        lspLogger->set_level(level);

        spdlog::set_default_logger(mainLogger);
        spdlog::register_logger(clinfoLogger);
        spdlog::register_logger(diagnosticsLogger);
        spdlog::register_logger(jsonrpcLogger);
        spdlog::register_logger(lspLogger);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cerr << "Log init failed: " << ex.what() << std::endl;
    }
    if (levelStr)
    {
        int lvl = std::atoi(levelStr);
        if (lvl < spdlog::level::n_levels)
        {
            level = static_cast<spdlog::level::level_enum>(lvl);
        }
    }
    else
    {
        level = spdlog::level::trace;
    }
    spdlog::set_level(level);
}

int main(int argc, char* argv[])
{
    std::signal(SIGINT, SignalHandler);

    if (isArgOption(argv, argv + argc, "--version"))
    {
        std::cout << VERSION << std::endl;
        exit(0);
    }

    const bool shouldLogTofile = isArgOption(argv, argv + argc, "--enable-file-tracing");
    char* filename = getArgOption(argv, argv + argc, "--filename");
    char* levelStr = getArgOption(argv, argv + argc, "--level");

    ConfigureLogging(shouldLogTofile, filename, levelStr);

    if (isArgOption(argv, argv + argc, "--clinfo"))
    {
        auto clinfo = CreateCLInfo();
        auto jsonBody = clinfo->json();
        std::cout << jsonBody.dump() << std::endl;
        exit(0);
    }

#if defined(WIN32)
    // to handle CRLF
    if (_setmode(_fileno(stdin), _O_BINARY) == -1)
        GLogError("Cannot set stdin mode to _O_BINARY");
    if (_setmode(_fileno(stdout), _O_BINARY) == -1)
        GLogError("Cannot set stdout mode to _O_BINARY");
#endif

    server = CreateLSPServer();
    return server->Run();
}

bool isArgOption(char** begin, char** end, const char* option)
{
    const char* optr = 0;
    char* bptr = 0;

    for (; begin != end; ++begin)
    {
        optr = option;
        bptr = *begin;

        for (; *bptr == *optr; ++bptr, ++optr)
        {
            if (*bptr == '\0')
            {
                return true;
            }
        }
    }

    return false;
}

char* getArgOption(char** begin, char** end, const char* option)
{
    const char* optr = 0;
    char* bptr = 0;

    for (; begin != end; ++begin)
    {
        optr = option;
        bptr = *begin;

        for (; *bptr == *optr; ++bptr, ++optr)
        {
            if (*bptr == '\0')
            {
                if (bptr != *end && ++bptr != *end)
                {
                    return bptr;
                }
            }
        }
    }

    return 0;
}
