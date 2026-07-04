//
//  typedef-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/04/26.
//

#pragma once

#include "typedef.hpp"

#include <gmock/gmock.h>

class TypeDefinitionMock : public ocls::ITypeDefinition
{
public:
    MOCK_METHOD(
        std::vector<ocls::Location>, GetTypeDefinitions, (const std::string &, unsigned, unsigned), (override));
};
