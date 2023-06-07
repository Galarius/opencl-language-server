//
//  clinfo.cpp
//  opencl-language-server
//
//  Created by is on 5.2.2023.
//

#include "clinfo.hpp"
#include "utils.hpp"

#include <array>
#include <spdlog/spdlog.h>
#include <unordered_map>

using namespace nlohmann;
using namespace vscode::opencl::utils;
using vscode::opencl::ICLInfo;

namespace {

constexpr const char logger[] = "clinfo";

const std::unordered_map<cl_bool, std::string> booleanChoices {
    {CL_TRUE, "CL_TRUE"},
    {CL_FALSE, "CL_FALSE"},
};
const std::unordered_map<cl_bitfield, std::string> commandQueuePropertiesChoices {
    {CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, "CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE"},
    {CL_QUEUE_PROFILING_ENABLE, "CL_QUEUE_PROFILING_ENABLE"},
};
const std::unordered_map<cl_bitfield, std::string> deviceFPConfigChoices {
    {CL_FP_DENORM, "CL_FP_DENORM"},
    {CL_FP_INF_NAN, "CL_FP_INF_NAN"},
    {CL_FP_ROUND_TO_NEAREST, "CL_FP_ROUND_TO_NEAREST"},
    {CL_FP_ROUND_TO_ZERO, "CL_FP_ROUND_TO_ZERO"},
    {CL_FP_ROUND_TO_INF, "CL_FP_ROUND_TO_INF"},
    {CL_FP_FMA, "CL_FP_FMA"},
};
const std::unordered_map<cl_bitfield, std::string> deviceExecCapabilitiesChoices {
    {CL_EXEC_KERNEL, "CL_EXEC_KERNEL"},
    {CL_EXEC_NATIVE_KERNEL, "CL_EXEC_NATIVE_KERNEL"},
};
const std::unordered_map<cl_bitfield, std::string> deviceMemCacheTypeChoices {
    {CL_NONE, "CL_NONE"}, {CL_READ_ONLY_CACHE, "CL_READ_ONLY_CACHE"}, {CL_READ_WRITE_CACHE, "CL_READ_WRITE_CACHE"}};
const std::unordered_map<cl_bitfield, std::string> deviceLocalMemTypeChoices {
    {CL_LOCAL, "CL_LOCAL"},
    {CL_GLOBAL, "CL_GLOBAL"},
};
const std::unordered_map<cl_bitfield, std::string> deviceTypeChoices {
    {CL_DEVICE_TYPE_CPU, "CL_DEVICE_TYPE_CPU"},
    {CL_DEVICE_TYPE_GPU, "CL_DEVICE_TYPE_GPU"},
    {CL_DEVICE_TYPE_ACCELERATOR, "CL_DEVICE_TYPE_ACCELERATOR"},
    {CL_DEVICE_TYPE_DEFAULT, "CL_DEVICE_TYPE_DEFAULT"},
};
const std::unordered_map<intptr_t, std::string> devicePartitionPropertyChoices {
    {CL_DEVICE_PARTITION_EQUALLY, "CL_DEVICE_PARTITION_EQUALLY"},
    {CL_DEVICE_PARTITION_BY_COUNTS, "CL_DEVICE_PARTITION_BY_COUNTS"},
    {CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN, "CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN"},
};
const std::unordered_map<cl_device_affinity_domain, std::string> deviceAffinityDomainChoices {
    {CL_DEVICE_AFFINITY_DOMAIN_NUMA, "CL_DEVICE_AFFINITY_DOMAIN_NUMA"},
    {CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE, "CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE"},
    {CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE, "CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE"},
    {CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE, "CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE"},
    {CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE, "CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE"},
    {CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE, "CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE"},
};

enum class PropertyType
{
    Boolean,
    CommandQueueProperties,
    DeviceAffinityDomain,
    DeviceExecCapabilities,
    DeviceFPConfig,
    DeviceLocalMemType,
    DeviceMemCacheType,
    DeviceType,
    Size,
    String,
    UnsignedInt,
    UnsignedLong,
    VectorOfSizes,
    VectorOfDevicePartitionProperties
};

struct CLPlatformInfo
{
    cl_platform_info field;
    std::string name;
};

struct CLDeviceInfo
{
    cl_device_info field;
    std::string name;
    PropertyType type;
};

std::array<CLPlatformInfo, 4> platformProperties {{
    {CL_PLATFORM_NAME, "CL_PLATFORM_NAME"},
    {CL_PLATFORM_VERSION, "CL_PLATFORM_VERSION"},
    {CL_PLATFORM_VENDOR, "CL_PLATFORM_VENDOR"},
    {CL_PLATFORM_PROFILE, "CL_PLATFORM_PROFILE"},
}};

std::array<CLDeviceInfo, 72> deviceProperties {{
    {CL_DEVICE_ADDRESS_BITS, "CL_DEVICE_ADDRESS_BITS", PropertyType::UnsignedInt},
    {CL_DEVICE_BUILT_IN_KERNELS, "CL_DEVICE_BUILT_IN_KERNELS", PropertyType::String},
    {CL_DEVICE_AVAILABLE, "CL_DEVICE_AVAILABLE", PropertyType::Boolean},
    {CL_DEVICE_COMPILER_AVAILABLE, "CL_DEVICE_COMPILER_AVAILABLE", PropertyType::Boolean},
    {CL_DEVICE_DOUBLE_FP_CONFIG, "CL_DEVICE_DOUBLE_FP_CONFIG", PropertyType::DeviceFPConfig},
    {CL_DEVICE_ENDIAN_LITTLE, "CL_DEVICE_ENDIAN_LITTLE", PropertyType::Boolean},
    {CL_DEVICE_ERROR_CORRECTION_SUPPORT, "CL_DEVICE_ERROR_CORRECTION_SUPPORT", PropertyType::Boolean},
    {CL_DEVICE_EXECUTION_CAPABILITIES, "CL_DEVICE_EXECUTION_CAPABILITIES", PropertyType::DeviceExecCapabilities},
    {CL_DEVICE_EXTENSIONS, "CL_DEVICE_EXTENSIONS", PropertyType::String},
    {CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE", PropertyType::UnsignedLong},
    {CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE", PropertyType::DeviceMemCacheType},
    {CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE", PropertyType::UnsignedInt},
    {CL_DEVICE_GLOBAL_MEM_SIZE, "CL_DEVICE_GLOBAL_MEM_SIZE", PropertyType::UnsignedLong},
    {CL_DEVICE_HOST_UNIFIED_MEMORY, "CL_DEVICE_HOST_UNIFIED_MEMORY", PropertyType::Boolean},
    {CL_DEVICE_IMAGE_SUPPORT, "CL_DEVICE_IMAGE_SUPPORT", PropertyType::Boolean},
    {CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, "CL_DEVICE_IMAGE_MAX_BUFFER_SIZE", PropertyType::Size},
    {CL_DEVICE_IMAGE_MAX_ARRAY_SIZE, "CL_DEVICE_IMAGE_MAX_ARRAY_SIZE", PropertyType::Size},
    {CL_DEVICE_IMAGE_PITCH_ALIGNMENT, "CL_DEVICE_IMAGE_PITCH_ALIGNMENT", PropertyType::UnsignedInt},
    {CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT, "CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT", PropertyType::UnsignedInt},
    {CL_DEVICE_IMAGE2D_MAX_HEIGHT, "CL_DEVICE_IMAGE2D_MAX_HEIGHT", PropertyType::Size},
    {CL_DEVICE_IMAGE2D_MAX_WIDTH, "CL_DEVICE_IMAGE2D_MAX_WIDTH", PropertyType::Size},
    {CL_DEVICE_IMAGE3D_MAX_DEPTH, "CL_DEVICE_IMAGE3D_MAX_DEPTH", PropertyType::Size},
    {CL_DEVICE_IMAGE3D_MAX_HEIGHT, "CL_DEVICE_IMAGE3D_MAX_HEIGHT", PropertyType::Size},
    {CL_DEVICE_IMAGE3D_MAX_WIDTH, "CL_DEVICE_IMAGE3D_MAX_WIDTH", PropertyType::Size},
    {CL_DEVICE_LOCAL_MEM_SIZE, "CL_DEVICE_LOCAL_MEM_SIZE", PropertyType::Size},
    {CL_DEVICE_LOCAL_MEM_TYPE, "CL_DEVICE_LOCAL_MEM_TYPE", PropertyType::UnsignedLong},
    {CL_DEVICE_MAX_CLOCK_FREQUENCY, "CL_DEVICE_MAX_CLOCK_FREQUENCY", PropertyType::UnsignedInt},
    {CL_DEVICE_MAX_COMPUTE_UNITS, "CL_DEVICE_MAX_COMPUTE_UNITS", PropertyType::UnsignedInt},
    {CL_DEVICE_MAX_CONSTANT_ARGS, "CL_DEVICE_MAX_CONSTANT_ARGS", PropertyType::UnsignedInt},
    {CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE", PropertyType::UnsignedLong},
    {CL_DEVICE_MAX_MEM_ALLOC_SIZE, "CL_DEVICE_MAX_MEM_ALLOC_SIZE", PropertyType::UnsignedLong},
    {CL_DEVICE_MAX_PARAMETER_SIZE, "CL_DEVICE_MAX_PARAMETER_SIZE", PropertyType::Size},
    {CL_DEVICE_MAX_READ_IMAGE_ARGS, "CL_DEVICE_MAX_READ_IMAGE_ARGS", PropertyType::UnsignedInt},
    {CL_DEVICE_MAX_SAMPLERS, "CL_DEVICE_MAX_SAMPLERS", PropertyType::UnsignedInt},
    {CL_DEVICE_MAX_WORK_GROUP_SIZE, "CL_DEVICE_MAX_WORK_GROUP_SIZE", PropertyType::Size},
    {CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS", PropertyType::UnsignedInt},
    {CL_DEVICE_MAX_WORK_ITEM_SIZES, "CL_DEVICE_MAX_WORK_ITEM_SIZES", PropertyType::VectorOfSizes},
    {CL_DEVICE_MAX_WRITE_IMAGE_ARGS, "CL_DEVICE_MAX_WRITE_IMAGE_ARGS", PropertyType::UnsignedInt},
    {CL_DEVICE_MEM_BASE_ADDR_ALIGN, "CL_DEVICE_MEM_BASE_ADDR_ALIGN", PropertyType::UnsignedInt},
    {CL_DEVICE_NAME, "CL_DEVICE_NAME", PropertyType::String},
    {CL_DEVICE_LINKER_AVAILABLE, "CL_DEVICE_LINKER_AVAILABLE", PropertyType::Boolean},
    {CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, "CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR", PropertyType::UnsignedInt},
    {CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, "CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT", PropertyType::UnsignedInt},
    {CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, "CL_DEVICE_NATIVE_VECTOR_WIDTH_INT", PropertyType::UnsignedInt},
    {CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, "CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG", PropertyType::UnsignedInt},
    {CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, "CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT", PropertyType::UnsignedInt},
    {CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, "CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE", PropertyType::UnsignedInt},
    {CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, "CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF", PropertyType::UnsignedInt},
    {CL_DEVICE_OPENCL_C_VERSION, "CL_DEVICE_OPENCL_C_VERSION", PropertyType::String},
    {CL_DEVICE_PARTITION_MAX_SUB_DEVICES, "CL_DEVICE_PARTITION_MAX_SUB_DEVICES", PropertyType::UnsignedInt},
    {CL_DEVICE_PARTITION_PROPERTIES, "CL_DEVICE_PARTITION_PROPERTIES", PropertyType::VectorOfDevicePartitionProperties},
    {CL_DEVICE_PARTITION_AFFINITY_DOMAIN, "CL_DEVICE_PARTITION_AFFINITY_DOMAIN", PropertyType::DeviceAffinityDomain},
    {CL_DEVICE_PARTITION_TYPE, "CL_DEVICE_PARTITION_TYPE", PropertyType::VectorOfDevicePartitionProperties},
    {CL_DEVICE_PREFERRED_INTEROP_USER_SYNC, "CL_DEVICE_PREFERRED_INTEROP_USER_SYNC", PropertyType::Boolean},
    {CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR", PropertyType::UnsignedInt},
    {CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT", PropertyType::UnsignedInt},
    {CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT", PropertyType::UnsignedInt},
    {CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG", PropertyType::UnsignedInt},
    {CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT", PropertyType::UnsignedInt},
    {CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE", PropertyType::UnsignedInt},
    {CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF", PropertyType::UnsignedInt},
    {CL_DEVICE_PRINTF_BUFFER_SIZE, "CL_DEVICE_PRINTF_BUFFER_SIZE", PropertyType::Size},
    {CL_DEVICE_PROFILE, "CL_DEVICE_PROFILE", PropertyType::String},
    {CL_DEVICE_PROFILING_TIMER_RESOLUTION, "CL_DEVICE_PROFILING_TIMER_RESOLUTION", PropertyType::Size},
    {CL_DEVICE_REFERENCE_COUNT, "CL_DEVICE_REFERENCE_COUNT", PropertyType::UnsignedInt},
    {CL_DEVICE_QUEUE_PROPERTIES, "CL_DEVICE_QUEUE_PROPERTIES", PropertyType::DeviceLocalMemType},
    {CL_DEVICE_SINGLE_FP_CONFIG, "CL_DEVICE_SINGLE_FP_CONFIG", PropertyType::DeviceFPConfig},
    {CL_DEVICE_TYPE, "CL_DEVICE_TYPE", PropertyType::DeviceType},
    {CL_DEVICE_VENDOR, "CL_DEVICE_VENDOR", PropertyType::String},
    {CL_DEVICE_VENDOR_ID, "CL_DEVICE_VENDOR_ID", PropertyType::UnsignedInt},
    {CL_DEVICE_VERSION, "CL_DEVICE_VERSION", PropertyType::String},
    {CL_DRIVER_VERSION, "CL_DRIVER_VERSION", PropertyType::String},
}};

json GetStringsArrayFromBitfield(cl_bitfield value, const std::unordered_map<cl_bitfield, std::string>& options)
{
    json values;
    for (auto& [bitfield, label] : options)
    {
        if (value & bitfield)
        {
            values.emplace_back(label);
        }
    }
    return values;
}

std::string GetStringFromEnum(cl_bitfield value, const std::unordered_map<cl_bitfield, std::string>& options)
{
    for (auto& [bitfield, label] : options)
    {
        if (value == bitfield)
        {
            return label;
        }
    }
    return "unknown";
}

json GetExtensions(const std::string& strExtensions)
{
    auto extensions = SplitString(strExtensions, " ");
    return json(extensions);
}

json GetKernels(const std::string& strKernels)
{
    auto kernels = SplitString(strKernels, ";");
    if (kernels.size() == 1 && kernels.front().empty())
    {
        return {};
    }
    return json(kernels);
}

json::object_t GetDeviceJSONInfo(const cl::Device& device)
{
    json info;
    for (auto& property : deviceProperties)
    {
        try
        {
            switch (property.type)
            {
                case PropertyType::UnsignedInt:
                {
                    cl_uint value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = value;
                    break;
                }
                case PropertyType::Boolean:
                {
                    cl_bool value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = booleanChoices.at(value);
                    break;
                }
                case PropertyType::DeviceFPConfig:
                {
                    cl_bitfield value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = GetStringsArrayFromBitfield(value, deviceFPConfigChoices);
                    break;
                }
                case PropertyType::DeviceExecCapabilities:
                {
                    cl_bitfield value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = GetStringsArrayFromBitfield(value, deviceFPConfigChoices);
                    break;
                }
                case PropertyType::String:
                {
                    std::string value;
                    device.getInfo(property.field, &value);
                    RemoveNullTerminator(value);
                    if (property.name == "CL_DEVICE_EXTENSIONS")
                    {
                        info[property.name] = GetExtensions(value);
                    }
                    else if (property.name == "CL_DEVICE_BUILT_IN_KERNELS")
                    {
                        info[property.name] = GetKernels(value);
                    }
                    else
                    {
                        info[property.name] = value;
                    }
                    break;
                }
                case PropertyType::UnsignedLong:
                {
                    cl_ulong value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = value;
                    break;
                }
                case PropertyType::DeviceMemCacheType:
                {
                    cl_bitfield value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = GetStringFromEnum(value, deviceMemCacheTypeChoices);
                    break;
                }
                case PropertyType::Size:
                {
                    std::size_t value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = value;
                    break;
                }
                case PropertyType::VectorOfSizes:
                {
                    std::vector<std::size_t> values;
                    device.getInfo(property.field, &values);
                    info[property.name] = json(values);
                    break;
                }
                case PropertyType::DeviceLocalMemType:
                {
                    cl_bitfield value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = GetStringFromEnum(value, deviceLocalMemTypeChoices);
                    break;
                }
                case PropertyType::CommandQueueProperties:
                {
                    cl_bitfield value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = GetStringsArrayFromBitfield(value, commandQueuePropertiesChoices);
                    break;
                }
                case PropertyType::DeviceType:
                {
                    cl_bitfield value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = GetStringsArrayFromBitfield(value, deviceTypeChoices);
                    break;
                }
                case PropertyType::VectorOfDevicePartitionProperties:
                {
                    std::vector<cl_device_partition_property> values;
                    device.getInfo(property.field, &values);
                    json partitionProperties;
                    for (auto value : values)
                    {
                        if (value != 0)
                        {
                            partitionProperties.emplace_back(devicePartitionPropertyChoices.at(value));
                        }
                    }
                    info[property.name] = partitionProperties;
                    break;
                }
                case PropertyType::DeviceAffinityDomain:
                {
                    cl_bitfield value = 0;
                    device.getInfo(property.field, &value);
                    info[property.name] = GetStringsArrayFromBitfield(value, deviceAffinityDomainChoices);
                    break;
                }
            }
        }
        catch (const cl::Error& err)
        {
            spdlog::get(logger)->error("Failed to get info for the device, {}", err.what());
            continue;
        }
    }
    return info;
}

uint32_t CalculateDeviceID(const cl::Device& device)
{
    try
    {
        auto name = device.getInfo<CL_DEVICE_NAME>();
        auto type = device.getInfo<CL_DEVICE_TYPE>();
        auto version = device.getInfo<CL_DEVICE_VERSION>();
        auto vendor = device.getInfo<CL_DEVICE_VENDOR>();
        auto vendorID = device.getInfo<CL_DEVICE_VENDOR_ID>();
        auto driverVersion = device.getInfo<CL_DRIVER_VERSION>();
        auto identifier = std::move(name) + std::to_string(type) + std::move(version) + std::move(vendor) +
            std::to_string(vendorID) + std::move(driverVersion);
        return CRC32(identifier.begin(), identifier.end());
    }
    catch (const cl::Error& err)
    {
        spdlog::get(logger)->error("Failed to calculate device uuid, {}", err.what());
    }
    return 0;
}

json GetDevicesJSONInfo(const std::vector<cl::Device>& devices)
{
    json jsonDevices;
    for (auto& device : devices)
    {
        auto info = GetDeviceJSONInfo(device);
        info["DEVICE_ID"] = CalculateDeviceID(device);
        jsonDevices.push_back(info);
    }
    return jsonDevices;
}

uint32_t CalculatePlatformID(const cl::Platform& platform)
{
    try
    {
        const auto name = platform.getInfo<CL_PLATFORM_NAME>();
        const auto version = platform.getInfo<CL_PLATFORM_VERSION>();
        const auto vendor = platform.getInfo<CL_PLATFORM_VENDOR>();
        const auto profile = platform.getInfo<CL_PLATFORM_PROFILE>();
        const auto identifier = name + version + vendor + profile;
        return CRC32(identifier.begin(), identifier.end());
    }
    catch (const cl::Error& err)
    {
        spdlog::get(logger)->error("Failed to calculate platform uuid, {}", err.what());
    }
    return 0;
}

json GetPlatformJSONInfo(const cl::Platform& platform)
{
    json info;
    for (auto& property : platformProperties)
    {
        try
        {
            std::string value;
            platform.getInfo(property.field, &value);
            RemoveNullTerminator(value);
            info[property.name] = value;
        }
        catch (const cl::Error& err)
        {
            spdlog::get(logger)->error("Failed to get info for a platform, {}", err.what());
        }
    }

    info["PLATFORM_ID"] = CalculatePlatformID(platform);

    auto extensions = platform.getInfo<CL_PLATFORM_EXTENSIONS>();
    RemoveNullTerminator(extensions);
    info["CL_PLATFORM_EXTENSIONS"] = GetExtensions(extensions);

    try
    {
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
        info["DEVICES"] = GetDevicesJSONInfo(devices);
    }
    catch (const cl::Error& err)
    {
        spdlog::get(logger)->error("Failed to get devices for a platform, {}", err.what());
    }

    return info;
}

class CLInfo final : public ICLInfo
{
public:
    nlohmann::json json()
    {
        spdlog::get(logger)->trace("Searching for OpenCL platforms...");
        std::vector<cl::Platform> platforms;
        try
        {
            cl::Platform::get(&platforms);
        }
        catch (const cl::Error& err)
        {
            spdlog::get(logger)->error("No OpenCL platforms were found ({})", err.what());
        }

        spdlog::get(logger)->info("Found OpenCL platforms, {}", platforms.size());
        if (platforms.size() == 0)
        {
            return {};
        }

        std::vector<nlohmann::json> jsonPlatforms;
        for (auto& platform : platforms)
        {
            jsonPlatforms.emplace_back(GetPlatformJSONInfo(platform));
        }

        return nlohmann::json {{"PLATFORMS", jsonPlatforms}};
    }

    uint32_t GetDeviceID(const cl::Device& device)
    {
        return CalculateDeviceID(device);
    }

    std::string GetDeviceDescription(const cl::Device& device)
    {
        auto name = device.getInfo<CL_DEVICE_NAME>();
        auto type = device.getInfo<CL_DEVICE_TYPE>();
        auto version = device.getInfo<CL_DEVICE_VERSION>();
        auto vendor = device.getInfo<CL_DEVICE_VENDOR>();
        auto vendorID = device.getInfo<CL_DEVICE_VENDOR_ID>();
        auto driverVersion = device.getInfo<CL_DRIVER_VERSION>();
        RemoveNullTerminator(name);
        RemoveNullTerminator(version);
        RemoveNullTerminator(vendor);
        RemoveNullTerminator(driverVersion);
        auto description = "name: " + std::move(name) + "; " + "type: " + std::to_string(type) + "; " +
            "version: " + std::move(version) + "; " + "vendor: " + std::move(vendor) + "; " +
            "vendorID: " + std::to_string(vendorID) + "; " + "driverVersion: " + std::move(driverVersion);
        return description;
    }
};

} // namespace

namespace vscode::opencl {

std::shared_ptr<ICLInfo> CreateCLInfo()
{
    return std::shared_ptr<ICLInfo>(new CLInfo());
}

} // namespace vscode::opencl
