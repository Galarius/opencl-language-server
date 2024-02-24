//
//  completion-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 10/8/23.
//

#pragma once

#include "completion.hpp"

#include <gmock/gmock.h>

class CompletionMock : public ocls::ICompletion
{
public:
    MOCK_METHOD(std::vector<ocls::CompletionResult>, GetCompletions, (const std::string &, unsigned, unsigned), (override));
};
