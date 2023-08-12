//
//  clinfo.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 5.2.2023.
//

#pragma once

#include <CL/opencl.hpp>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace ocls {

struct ICLInfo
{
    virtual ~ICLInfo() = default;

    virtual nlohmann::json json() = 0;

    virtual std::vector<cl::Device> GetDevices() = 0;

    virtual uint32_t GetDeviceID(const cl::Device& device) = 0;

    virtual std::string GetDeviceDescription(const cl::Device& device) = 0;

    virtual size_t GetDevicePowerIndex(const cl::Device& device) = 0;
};

std::shared_ptr<ICLInfo> CreateCLInfo();

} // namespace ocls
