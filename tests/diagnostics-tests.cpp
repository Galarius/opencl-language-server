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


TEST(DiagnosticsTest, SelectDeviceBasedOnPowerIndexDuringTheCreation)
{
    auto mockCLInfo = std::make_shared<CLInfoMock>();
    std::vector<cl::Device> devices = {cl::Device(), cl::Device()};

    EXPECT_CALL(*mockCLInfo, GetDevices()).WillOnce(Return(devices));
    EXPECT_CALL(*mockCLInfo, GetDevicePowerIndex(_)).WillOnce(Return(10)).WillOnce(Return(10));
    EXPECT_CALL(*mockCLInfo, GetDeviceDescription(_)).WillOnce(Return(""));

    CreateDiagnostics(mockCLInfo);
}

TEST(DiagnosticsTest, SelectDeviceBasedOnPowerIndex)
{
    auto mockCLInfo = std::make_shared<CLInfoMock>();
    std::vector<cl::Device> devices = {cl::Device(), cl::Device()};

    EXPECT_CALL(*mockCLInfo, GetDevices()).Times(2).WillRepeatedly(Return(devices));
    EXPECT_CALL(*mockCLInfo, GetDevicePowerIndex(_))
        .WillOnce(Return(10))
        .WillOnce(Return(20))
        .WillOnce(Return(10))
        .WillOnce(Return(20))
        .WillOnce(Return(20))
        .WillOnce(Return(20));
    EXPECT_CALL(*mockCLInfo, GetDeviceDescription(_)).WillOnce(Return(""));
    auto diagnostics = CreateDiagnostics(mockCLInfo);
    diagnostics->SetOpenCLDevice(0);
}

TEST(DiagnosticsTest, SelectDeviceBasedOnExistingIndex)
{
    auto mockCLInfo = std::make_shared<CLInfoMock>();
    std::vector<cl::Device> devices = {cl::Device(), cl::Device()};

    EXPECT_CALL(*mockCLInfo, GetDevices()).Times(2).WillRepeatedly(Return(devices));
    EXPECT_CALL(*mockCLInfo, GetDevicePowerIndex(_)).WillOnce(Return(10)).WillOnce(Return(20));
    EXPECT_CALL(*mockCLInfo, GetDeviceID(_)).WillOnce(Return(3138399603)).WillOnce(Return(2027288592));
    EXPECT_CALL(*mockCLInfo, GetDeviceDescription(_)).Times(2).WillRepeatedly(Return(""));
    auto diagnostics = CreateDiagnostics(mockCLInfo);
    diagnostics->SetOpenCLDevice(2027288592);
}

TEST(DiagnosticsTest, SelectDeviceBasedOnNonExistingIndex)
{
    auto mockCLInfo = std::make_shared<CLInfoMock>();
    std::vector<cl::Device> devices = {cl::Device(), cl::Device()};

    EXPECT_CALL(*mockCLInfo, GetDevices()).Times(2).WillRepeatedly(Return(devices));
    EXPECT_CALL(*mockCLInfo, GetDeviceID(_)).WillOnce(Return(3138399603)).WillOnce(Return(2027288592));
    EXPECT_CALL(*mockCLInfo, GetDevicePowerIndex(_))
        .WillOnce(Return(10))
        .WillOnce(Return(20))
        .WillOnce(Return(10))
        .WillOnce(Return(20))
        .WillOnce(Return(20))
        .WillOnce(Return(20));
    EXPECT_CALL(*mockCLInfo, GetDeviceDescription(_)).WillOnce(Return(""));

    auto diagnostics = CreateDiagnostics(mockCLInfo);
    diagnostics->SetOpenCLDevice(static_cast<uint32_t>(4527288514));
}
