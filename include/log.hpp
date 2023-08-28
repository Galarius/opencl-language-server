//
//  logs.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 20/08/23.
//

#pragma once

#include <string>
#include <spdlog/spdlog.h>

namespace ocls {

struct LogName
{
    static std::string main;
    static std::string clinfo;
    static std::string diagnostics;
    static std::string jrpc;
    static std::string lsp;
};

extern void ConfigureFileLogging(const std::string& filename, spdlog::level::level_enum level);
extern void ConfigureNullLogging();

} // namespace ocls
