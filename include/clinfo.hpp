//
//  clinfo.hpp
//  opencl-language-server
//
//  Created by is on 5.2.2023.
//

#pragma once

#include <memory>
#include <string>

#include "json.hpp"
#include "opencl.hpp"

namespace vscode::opencl {

struct ICLInfo
{
    virtual ~ICLInfo() = default;

    virtual boost::json::object json() = 0;

    virtual uint32_t GetDeviceID(const cl::Device& device) = 0;

    virtual std::string GetDeviceDescription(const cl::Device& device) = 0;
};

std::shared_ptr<ICLInfo> CreateCLInfo();

} // namespace vscode::opencl
