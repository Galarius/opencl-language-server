//
//  device.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 24.8.2023.
//

#pragma once

#include <CL/opencl.hpp>

namespace ocls {

class Device
{
public:
    Device(const Device&) = default;
    Device& operator=(const Device&) = default;
    Device(Device&&) noexcept = default;
    Device& operator=(Device&&) noexcept = default;

#ifdef ENABLE_TESTING
    Device(uint32_t identifier, std::string description, size_t powerIndex, std::string cl_standard)
        : m_identifier {identifier}
        , m_description {std::move(description)}
        , m_powerIndex {powerIndex}
        , m_cl_standard {std::move(cl_standard)}
    {}
#endif

    Device(cl::Device device, uint32_t identifier, std::string description, size_t powerIndex, std::string cl_standard)
        : m_device {std::move(device)}
        , m_identifier {identifier}
        , m_description {std::move(description)}
        , m_powerIndex {powerIndex}
        , m_cl_standard {std::move(cl_standard)}
    {}

    const cl::Device& getUnderlyingDevice() const noexcept
    {
        return m_device;
    }

    uint32_t GetID() const noexcept
    {
        return m_identifier;
    }

    std::string GetDescription() const noexcept
    {
        return m_description;
    }

    size_t GetPowerIndex() const noexcept
    {
        return m_powerIndex;
    }

    std::string GetCLStandard() const noexcept
    {
        return m_cl_standard;
    }


private:
    cl::Device m_device;
    uint32_t m_identifier;
    std::string m_description;
    size_t m_powerIndex;
    /// CL, CL1.0, CL1.1, CL1.2, CL2.0, CL3.0, CLC++, CLC++2021
    std::string m_cl_standard;
};

} // namespace ocls
