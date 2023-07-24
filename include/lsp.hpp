//
//  lsp.hpp
//  opencl-language-server
//
//  Created by Ilya Shoshin (Galarius) on 7/16/21.
//

#pragma once

#include <memory>

#include "diagnostics.hpp"
#include "jsonrpc.hpp"

namespace ocls {

struct ILSPServer
{
    virtual int Run() = 0;
    virtual void Interrupt() = 0;
};

std::shared_ptr<ILSPServer> CreateLSPServer(std::shared_ptr<IJsonRPC> jrpc, std::shared_ptr<IDiagnostics> diagnostics);

} // namespace ocls
