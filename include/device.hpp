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
    Device(uint32_t identifier, std::string description, size_t powerIndex)
        : m_identifier {identifier}
        , m_description {std::move(description)}
        , m_powerIndex {powerIndex}
    {}
#endif

    Device(cl::Device device, uint32_t identifier, std::string description, size_t powerIndex)
        : m_device {std::move(device)}
        , m_identifier {identifier}
        , m_description {std::move(description)}
        , m_powerIndex {powerIndex}
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

private:
    cl::Device m_device;
    uint32_t m_identifier;
    std::string m_description;
    size_t m_powerIndex;
};

} // namespace ocls
