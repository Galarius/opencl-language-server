//
//  diagnostics.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/16/21.
//

#include "diagnostics.hpp"
#include "utils.hpp"

#include <CL/opencl.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <regex>
#include <spdlog/spdlog.h>
#include <stdexcept> // std::runtime_error, std::invalid_argument
#include <tuple>

using namespace nlohmann;

namespace {

constexpr char logger[] = "diagnostics";

int ParseSeverity(const std::string& severity)
{
    if (severity == "error")
        return 1;
    else if (severity == "warning")
        return 2;
    else
        return -1;
}

// <program source>:13:5: warning: no previous prototype for function 'getChannel'
std::tuple<std::string, long, long, long, std::string> ParseOutput(const std::smatch& matches)
{
    std::string source = matches[1];
    const long line = std::stoi(matches[2]) - 1; // LSP assumes 0-indexed lines
    const long col = std::stoi(matches[3]);
    const int severity = ParseSeverity(matches[4]);
    // matches[5] - 'fatal'
    std::string message = matches[6];
    return std::make_tuple(std::move(source), line, col, severity, std::move(message));
}

} // namespace

namespace ocls {

class Diagnostics final : public IDiagnostics
{
public:
    explicit Diagnostics(std::shared_ptr<ICLInfo> clInfo);

    void SetBuildOptions(const nlohmann::json& options);
    void SetBuildOptions(const std::string& options);
    void SetMaxProblemsCount(uint64_t maxNumberOfProblems);
    void SetOpenCLDevice(uint32_t identifier);
    std::string GetBuildLog(const Source& source);
    nlohmann::json GetDiagnostics(const Source& source);

private:
    std::optional<cl::Device> SelectOpenCLDevice(const std::vector<cl::Device>& devices, uint32_t identifier);
    std::optional<cl::Device> SelectOpenCLDeviceByPowerIndex(const std::vector<cl::Device>& devices);
    nlohmann::json BuildDiagnostics(const std::string& buildLog, const std::string& name);
    std::string BuildSource(const std::string& source) const;

private:
    std::shared_ptr<ICLInfo> m_clInfo;
    std::optional<cl::Device> m_device;
    std::regex m_regex {"^(.*):(\\d+):(\\d+): ((fatal )?error|warning|Scholar): (.*)$"};
    std::string m_BuildOptions;
    uint64_t m_maxNumberOfProblems = INT8_MAX;
};

Diagnostics::Diagnostics(std::shared_ptr<ICLInfo> clInfo) : m_clInfo {std::move(clInfo)}
{
    SetOpenCLDevice(0);
}

std::optional<cl::Device> Diagnostics::SelectOpenCLDeviceByPowerIndex(const std::vector<cl::Device>& devices)
{
    auto maxIt = std::max_element(devices.begin(), devices.end(), [this](const cl::Device& a, const cl::Device& b) {
        const auto powerIndexA = m_clInfo->GetDevicePowerIndex(a);
        const auto powerIndexB = m_clInfo->GetDevicePowerIndex(b);
        return powerIndexA < powerIndexB;
    });

    if (maxIt == devices.end())
    {
        return std::nullopt;
    }

    return *maxIt;
}

std::optional<cl::Device> Diagnostics::SelectOpenCLDevice(const std::vector<cl::Device>& devices, uint32_t identifier)
{
    auto log = spdlog::get(logger);
    std::optional<cl::Device> selectedDevice;

    // Find device by identifier
    auto it = std::find_if(devices.begin(), devices.end(), [this, &identifier](const cl::Device& device) {
        try
        {
            return m_clInfo->GetDeviceID(device) == identifier;
        }
        catch (const cl::Error&)
        {
            return false;
        }
    });

    if (it != devices.end())
    {
        return *it;
    }

    // If device is not found by identifier, then find the device based on power index
    auto device = SelectOpenCLDeviceByPowerIndex(devices);
    if (device && (!m_device || m_clInfo->GetDevicePowerIndex(*device) > m_clInfo->GetDevicePowerIndex(*m_device)))
    {
        selectedDevice = device;
    }

    return selectedDevice;
}

void Diagnostics::SetOpenCLDevice(uint32_t identifier)
{
    auto log = spdlog::get(logger);
    log->trace("Selecting OpenCL device...");

    const auto devices = m_clInfo->GetDevices();

    if (devices.size() == 0)
    {
        return;
    }

    m_device = SelectOpenCLDevice(devices, identifier);

    if (!m_device)
    {
        log->warn("No suitable OpenCL device was found.");
        return;
    }

    auto description = m_clInfo->GetDeviceDescription(*m_device);
    log->info("Selected OpenCL device: {}", description);
}

std::string Diagnostics::BuildSource(const std::string& source) const
{
    if (!m_device.has_value())
    {
        throw std::runtime_error("missing OpenCL device");
    }

    std::vector<cl::Device> ds {*m_device};
    cl::Context context(ds, NULL, NULL, NULL);
    cl::Program program;
    try
    {
        spdlog::get(logger)->debug("Building program with options: {}", m_BuildOptions);
        program = cl::Program(context, source, false);
        program.build(ds, m_BuildOptions.c_str());
    }
    catch (cl::Error& err)
    {
        if (err.err() != CL_BUILD_PROGRAM_FAILURE)
        {
            spdlog::get(logger)->error("Failed to build program: {} ({})", err.what(), err.err());
            throw err;
        }
    }

    std::string build_log;

    try
    {
        program.getBuildInfo(*m_device, CL_PROGRAM_BUILD_LOG, &build_log);
    }
    catch (cl::Error& err)
    {
        spdlog::get(logger)->error("Failed get build info, error, {}", err.what());
    }

    return build_log;
}

nlohmann::json Diagnostics::BuildDiagnostics(const std::string& buildLog, const std::string& name)
{
    std::smatch matches;
    auto errorLines = utils::SplitString(buildLog, "\n");
    json diagnostics;
    uint64_t count = 0;
    for (auto errLine : errorLines)
    {
        std::regex_search(errLine, matches, m_regex);
        if (matches.size() != 7)
            continue;

        if (count++ > m_maxNumberOfProblems)
        {
            spdlog::get(logger)->info("Maximum number of problems reached, other problems will be slipped");
            break;
        }

        auto [source, line, col, severity, message] = ParseOutput(matches);
        json diagnostic;
        json range {
            {"start",
             {
                 {"line", line},
                 {"character", col},
             }},
            {"end",
             {
                 {"line", line},
                 {"character", col},
             }},
        };
        diagnostic["source"] = name.empty() ? source : name;
        diagnostic["range"] = range;
        diagnostic["severity"] = severity;
        diagnostic["message"] = message;
        diagnostics.emplace_back(diagnostic);
    }
    return diagnostics;
}

std::string Diagnostics::GetBuildLog(const Source& source)
{
    if (!m_device.has_value())
    {
        throw std::runtime_error("missing OpenCL device");
    }
    spdlog::get(logger)->trace("Getting diagnostics...");
    return BuildSource(source.text);
}

nlohmann::json Diagnostics::GetDiagnostics(const Source& source)
{
    std::string buildLog = GetBuildLog(source);
    std::string srcName;
    if (!source.filePath.empty())
    {
        auto filePath = std::filesystem::path(source.filePath).string();
        srcName = std::filesystem::path(filePath).filename().string();
    }
    buildLog = BuildSource(source.text);
    spdlog::get(logger)->trace("BuildLog:\n{}", buildLog);
    return BuildDiagnostics(buildLog, srcName);
}

void Diagnostics::SetBuildOptions(const json& options)
{
    try
    {
        auto concat = [](const std::string& acc, const json& j) { return acc + j.get<std::string>() + " "; };
        auto opts = std::accumulate(options.begin(), options.end(), std::string(), concat);
        SetBuildOptions(opts);
    }
    catch (std::exception& e)
    {
        spdlog::get(logger)->error("Failed to parse build options, {}", e.what());
    }
}

void Diagnostics::SetBuildOptions(const std::string& options)
{
    m_BuildOptions = options;
    spdlog::get(logger)->trace("Set build options, {}", m_BuildOptions);
}

void Diagnostics::SetMaxProblemsCount(uint64_t maxNumberOfProblems)
{
    spdlog::get(logger)->trace("Set max number of problems: {}", maxNumberOfProblems);
    m_maxNumberOfProblems = maxNumberOfProblems;
}

std::shared_ptr<IDiagnostics> CreateDiagnostics(std::shared_ptr<ICLInfo> clInfo)
{
    return std::make_shared<Diagnostics>(std::move(clInfo));
}

} // namespace ocls
