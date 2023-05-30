//
//  lsp.hpp
//  opencl-language-server
//
//  Created by Ilya Shoshin (Galarius) on 7/16/21.
//

#pragma once

#include <memory>

namespace vscode::opencl {

struct ILSPServer
{
    virtual int Run() = 0;
    virtual void Interrupt() = 0;
};

std::shared_ptr<ILSPServer> CreateLSPServer();

} // namespace vscode::opencl
