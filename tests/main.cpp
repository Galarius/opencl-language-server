//
//  main.cpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 7/16/21.
//

#include "log.hpp"

#include <gtest/gtest.h>
#include <memory>

int main(int argc, char** argv)
{
    ocls::ConfigureNullLogging();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}