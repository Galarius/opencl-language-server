//
//  definition-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/04/26.
//

#pragma once

#include "definition.hpp"

#include <gmock/gmock.h>

class DefinitionMock : public ocls::IDefinition
{
public:
    MOCK_METHOD(
        std::vector<ocls::DefinitionLocation>, GetDefinitions, (const std::string &, unsigned, unsigned), (override));
};
