//
//  clinfo.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 5.2.2023.
//

#pragma once

#include "device.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace ocls {

struct ICLInfo
{
    virtual ~ICLInfo() = default;

    virtual nlohmann::json json() = 0;

    virtual std::vector<Device> GetDevices() = 0;
};

std::shared_ptr<ICLInfo> CreateCLInfo();

} // namespace ocls
