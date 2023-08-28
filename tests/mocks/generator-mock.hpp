//
//  generator-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/30/23.
//

#pragma once

#include "utils.hpp"

#include <gmock/gmock.h>

class GeneratorMock : public ocls::utils::IGenerator
{
public:
    MOCK_METHOD(std::string, GenerateID, (), (override));
};
