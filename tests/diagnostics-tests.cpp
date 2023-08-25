//
//  diagnostics-tests.cpp
//  opencl-language-server-tests
//
//  Created by Ilia Shoshin on 14/8/23.
//

#include "diagnostics.hpp"
#include "clinfo-mock.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <vector>


using namespace ocls;
using namespace testing;

namespace {

const uint32_t deviceID1 = 12345678;
const uint32_t deviceID2 = 23456789;

std::vector<Device> GetTestDevices()
{
    return {Device(deviceID1, "Test Device 1", 10), Device(deviceID2, "Test Device 2", 20)};
}

class DiagnosticsTest : public ::testing::Test
{
protected:
    std::shared_ptr<CLInfoMock> mockCLInfo;
    
    void SetUp() override
    {
        mockCLInfo = std::make_shared<CLInfoMock>();
    }
};

} // namespace

TEST_F(DiagnosticsTest, SelectDeviceBasedOnPowerIndexDuringTheCreation)
{
    EXPECT_CALL(*mockCLInfo, GetDevices()).WillOnce(Return(GetTestDevices()));
    
    auto diagnostics = CreateDiagnostics(mockCLInfo);

    ASSERT_TRUE(diagnostics->GetDevice().has_value());
    EXPECT_EQ(diagnostics->GetDevice().value().GetID(), deviceID2);
}

TEST_F(DiagnosticsTest, SelectDeviceBasedOnPowerIndex)
{
    EXPECT_CALL(*mockCLInfo, GetDevices()).Times(2).WillRepeatedly(Return(GetTestDevices()));
    
    auto diagnostics = CreateDiagnostics(mockCLInfo);
    diagnostics->SetOpenCLDevice(0);

    ASSERT_TRUE(diagnostics->GetDevice().has_value());
    EXPECT_EQ(diagnostics->GetDevice().value().GetID(), deviceID2);
}

TEST_F(DiagnosticsTest, SelectDeviceBasedOnExistingIndex)
{
    EXPECT_CALL(*mockCLInfo, GetDevices()).Times(2).WillRepeatedly(Return(GetTestDevices()));

    auto diagnostics = CreateDiagnostics(mockCLInfo);
    diagnostics->SetOpenCLDevice(12345678);

    ASSERT_TRUE(diagnostics->GetDevice().has_value());
    EXPECT_EQ(diagnostics->GetDevice().value().GetID(), deviceID1);
}

TEST_F(DiagnosticsTest, SelectDeviceBasedOnNonExistingIndex)
{
    EXPECT_CALL(*mockCLInfo, GetDevices()).Times(2).WillRepeatedly(Return(GetTestDevices()));

    auto diagnostics = CreateDiagnostics(mockCLInfo);
    diagnostics->SetOpenCLDevice(10000000);

    ASSERT_TRUE(diagnostics->GetDevice().has_value());
    EXPECT_EQ(diagnostics->GetDevice().value().GetID(), deviceID2);
}
