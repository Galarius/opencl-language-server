//
//  exit-handler-mock.hpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 8/03/23.
//


#include "utils.hpp"

#include <gmock/gmock.h>

class ExitHandlerMock : public ocls::utils::IExitHandler
{
public:
    MOCK_METHOD(void, OnExit, (int), (override));
};
