//
//  clinfo-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 14/8/23.
//

#include "clinfo.hpp"

#include <gmock/gmock.h>

class CLInfoMock : public ocls::ICLInfo
{
public:
    MOCK_METHOD(nlohmann::json, json, (), (override));

    MOCK_METHOD(std::vector<cl::Device>, GetDevices, (), (override));

    MOCK_METHOD(uint32_t, GetDeviceID, (const cl::Device&), (override));

    MOCK_METHOD(std::string, GetDeviceDescription, (const cl::Device&), (override));

    MOCK_METHOD(size_t, GetDevicePowerIndex, (const cl::Device&), (override));
};
