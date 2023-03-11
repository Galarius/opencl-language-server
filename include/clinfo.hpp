//
//  clinfo.hpp
//  opencl-language-server
//
//  Created by is on 5.2.2023.
//

#pragma once

#include <memory>
#include <string>

#pragma warning(push, 0)
#include <boost/json.hpp>
#pragma warning(pop)

namespace vscode::opencl {

struct ICLInfo
{
    virtual ~ICLInfo() = default;
    virtual boost::json::object json() = 0;
};

std::shared_ptr<ICLInfo> CreateCLInfo();

} // namespace vscode::opencl
