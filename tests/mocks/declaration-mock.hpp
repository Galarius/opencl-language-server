//
//  declaration-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/04/26.
//

#pragma once

#include "declaration.hpp"

#include <gmock/gmock.h>

class DeclarationMock : public ocls::IDeclaration
{
public:
    MOCK_METHOD(
        std::vector<ocls::Location>, GetDeclarations, (const std::string &, unsigned, unsigned), (override));
};
