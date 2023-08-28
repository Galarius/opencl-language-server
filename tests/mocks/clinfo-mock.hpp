//
//  clinfo-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 14/8/23.
//

#pragma once

#include "clinfo.hpp"

#include <gmock/gmock.h>

class CLInfoMock : public ocls::ICLInfo
{
public:
    MOCK_METHOD(nlohmann::json, json, (), (override));

    MOCK_METHOD(std::vector<ocls::Device>, GetDevices, (), (override));
};
