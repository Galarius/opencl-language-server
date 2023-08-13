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
#include <spdlog/spdlog.h>
#include <stdexcept> // std::runtime_error, std::invalid_argument
#include <sstream>

using namespace nlohmann;

namespace {

constexpr char logger[] = "diagnostics";

} // namespace

namespace ocls {

// - DiagnosticsParser

class DiagnosticsParser final : public IDiagnosticsParser
{
    std::regex m_regex {"^(.*):(\\d+):(\\d+): ((fatal )?error|warning|Scholar): (.*)$"};

public:
    int ParseSeverity(const std::string& severity)
    {
        if (severity == "warning")
        {
            return 2;
        }
        else if (utils::EndsWith(severity, "error"))
        {
            return 1;
        }
        return -1;
    }

    // example input: <program source>:13:5: warning: no previous prototype for function 'getChannel'
    std::tuple<std::string, long, long, long, std::string> ParseMatch(const std::smatch& matches)
    {
        std::string source = matches[1];
        const long line = std::stoi(matches[2]) - 1; // LSP assumes 0-indexed lines
        const long col = std::stoi(matches[3]);
        const int severity = ParseSeverity(matches[4]);
        // matches[5] - 'fatal '
        std::string message = matches[6];
        return std::make_tuple(std::move(source), line, col, severity, std::move(message));
    }

    nlohmann::json CreateDiagnostic(const std::smatch& matches, const std::string& name)
    {
        auto [source, line, col, severity, message] = ParseMatch(matches);
        return {
            {"source", name.empty() ? source : name},
            {"range", {{"start", {{"line", line}, {"character", col}}}, {"end", {{"line", line}, {"character", col}}}}},
            {"severity", severity},
            {"message", message}};
    }

    nlohmann::json ParseDiagnostics(const std::string& buildLog, const std::string& name, uint64_t problemsLimit)
    {
        nlohmann::json diagnostics;
        std::istringstream stream(buildLog);
        std::string errLine;
        uint64_t count = 0;
        std::smatch matches;
        while (std::getline(stream, errLine))
        {
            if (std::regex_search(errLine, matches, m_regex) && matches.size() == 7)
            {
                if (count++ >= problemsLimit)
                {
                    spdlog::get(logger)->info("Maximum number of problems reached, other problems will be skipped");
                    break;
                }
                diagnostics.emplace_back(CreateDiagnostic(matches, name));
            }
        }
        return diagnostics;
    }
};

// - Diagnostics

class Diagnostics final : public IDiagnostics
{
public:
    Diagnostics(std::shared_ptr<ICLInfo> clInfo, std::shared_ptr<IDiagnosticsParser> parser);

    void SetBuildOptions(const nlohmann::json& options);
    void SetBuildOptions(const std::string& options);
    void SetMaxProblemsCount(uint64_t maxNumberOfProblems);
    void SetOpenCLDevice(uint32_t identifier);
    std::string GetBuildLog(const Source& source);
    nlohmann::json GetDiagnostics(const Source& source);

private:
    std::optional<cl::Device> SelectOpenCLDevice(const std::vector<cl::Device>& devices, uint32_t identifier);
    std::optional<cl::Device> SelectOpenCLDeviceByPowerIndex(const std::vector<cl::Device>& devices);
    std::string BuildSource(const std::string& source) const;

private:
    std::shared_ptr<ICLInfo> m_clInfo;
    std::shared_ptr<IDiagnosticsParser> m_parser;
    std::optional<cl::Device> m_device;
    std::string m_BuildOptions;
    uint64_t m_maxNumberOfProblems = INT8_MAX;
};

Diagnostics::Diagnostics(std::shared_ptr<ICLInfo> clInfo, std::shared_ptr<IDiagnosticsParser> parser)
    : m_clInfo {std::move(clInfo)}
    , m_parser {std::move(parser)}
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
    return m_parser->ParseDiagnostics(buildLog, srcName, m_maxNumberOfProblems);
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

std::shared_ptr<IDiagnosticsParser> CreateDiagnosticsParser()
{
    return std::make_shared<DiagnosticsParser>();
}

std::shared_ptr<IDiagnostics> CreateDiagnostics(std::shared_ptr<ICLInfo> clInfo)
{
    return std::make_shared<Diagnostics>(std::move(clInfo), CreateDiagnosticsParser());
}

std::shared_ptr<IDiagnostics> CreateDiagnostics(
    std::shared_ptr<ICLInfo> clInfo, std::shared_ptr<IDiagnosticsParser> parser)
{
    return std::make_shared<Diagnostics>(std::move(clInfo), parser);
}

} // namespace ocls
