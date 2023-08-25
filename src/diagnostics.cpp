//
//  diagnostics.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/16/21.
//

#include "device.hpp"
#include "diagnostics.hpp"
#include "log.hpp"
#include "utils.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept> // std::runtime_error, std::invalid_argument
#include <sstream>

using namespace nlohmann;

namespace {

auto logger()
{
    return spdlog::get(ocls::LogName::diagnostics);
}

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
        nlohmann::json diagnostics = nlohmann::json::array();
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
                    logger()->warn("Maximum number of problems reached, other problems will be skipped");
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
    std::optional<ocls::Device> GetDevice() const;
    std::string GetBuildLog(const Source& source);
    nlohmann::json GetDiagnostics(const Source& source);

private:
    std::optional<ocls::Device> SelectOpenCLDevice(const std::vector<ocls::Device>& devices, uint32_t identifier);
    std::optional<ocls::Device> SelectOpenCLDeviceByPowerIndex(const std::vector<ocls::Device>& devices);
    std::string BuildSource(const std::string& source) const;

private:
    std::shared_ptr<ICLInfo> m_clInfo;
    std::shared_ptr<IDiagnosticsParser> m_parser;
    std::optional<ocls::Device> m_device;
    std::string m_BuildOptions;
    uint64_t m_maxNumberOfProblems = INT8_MAX;
};

Diagnostics::Diagnostics(std::shared_ptr<ICLInfo> clInfo, std::shared_ptr<IDiagnosticsParser> parser)
    : m_clInfo {std::move(clInfo)}
    , m_parser {std::move(parser)}
{
    SetOpenCLDevice(0);
}

// - IDiagnostics

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
        logger()->error("Failed to parse build options, {}", e.what());
    }
}

void Diagnostics::SetBuildOptions(const std::string& options)
{
    m_BuildOptions = options;
    logger()->trace("Set build options: {}", m_BuildOptions);
}

void Diagnostics::SetMaxProblemsCount(uint64_t maxNumberOfProblems)
{
    logger()->trace("Set max number of problems: {}", maxNumberOfProblems);
    m_maxNumberOfProblems = maxNumberOfProblems;
}

void Diagnostics::SetOpenCLDevice(uint32_t identifier)
{
    logger()->trace("Selecting OpenCL device [{}]...", identifier);

    const auto devices = m_clInfo->GetDevices();

    if (devices.size() == 0)
    {
        return;
    }

    auto selectedDevice = SelectOpenCLDevice(devices, identifier);
    if (!selectedDevice)
    {
        logger()->warn("No suitable OpenCL device was found");
        return;
    }

    m_device = selectedDevice;
    logger()->debug("Selected OpenCL device: {}", (*m_device).GetDescription());
}

std::optional<ocls::Device> Diagnostics::GetDevice() const
{
    return m_device;
}

std::string Diagnostics::GetBuildLog(const Source& source)
{
    if (!m_device.has_value())
    {
        throw std::runtime_error("missing OpenCL device");
    }
    logger()->trace("Getting diagnostics...");
    return BuildSource(source.text);
}

nlohmann::json Diagnostics::GetDiagnostics(const Source& source)
{
    std::string srcName;
    if (!source.filePath.empty())
    {
        auto filePath = std::filesystem::path(source.filePath).string();
        srcName = std::filesystem::path(filePath).filename().string();
    }
    std::string buildLog = GetBuildLog(source);
    logger()->trace("BuildLog:\n{}", buildLog);
    return m_parser->ParseDiagnostics(buildLog, srcName, m_maxNumberOfProblems);
}

// -

std::optional<ocls::Device> Diagnostics::SelectOpenCLDeviceByPowerIndex(const std::vector<ocls::Device>& devices)
{
    auto maxIt = std::max_element(devices.begin(), devices.end(), [](const ocls::Device& a, const ocls::Device& b) {
        return a.GetPowerIndex() < b.GetPowerIndex();
    });

    if (maxIt == devices.end())
    {
        return std::nullopt;
    }

    return *maxIt;
}

std::optional<ocls::Device> Diagnostics::SelectOpenCLDevice(
    const std::vector<ocls::Device>& devices, uint32_t identifier)
{
    if (identifier > 0)
    {
        logger()->trace("Searching for the device by ID '{}'...", identifier);
        auto it = std::find_if(devices.begin(), devices.end(), [&identifier](const ocls::Device& device) {
            try
            {
                return device.GetID() == identifier;
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
    }

    // If device is not found by identifier, then find the device based on power index
    logger()->trace("Searching for the device by power index...");
    auto device = SelectOpenCLDeviceByPowerIndex(devices);
    if (device && (!m_device || (*device).GetPowerIndex() > (*m_device).GetPowerIndex()))
    {
        return device;
    }

    return std::nullopt;
}

std::string Diagnostics::BuildSource(const std::string& source) const
{
    if (!m_device.has_value())
    {
        throw std::runtime_error("missing OpenCL device");
    }

    std::vector<cl::Device> ds {(*m_device).getUnderlyingDevice()};
    cl::Context context(ds, NULL, NULL, NULL);
    cl::Program program;
    try
    {
        if (m_BuildOptions.empty())
        {
            logger()->trace("Building program...");
        }
        else
        {
            logger()->trace("Building program with options: {}...", m_BuildOptions);
        }
        program = cl::Program(context, source, false);
        program.build(ds, m_BuildOptions.c_str());
    }
    catch (cl::Error& err)
    {
        if (err.err() != CL_BUILD_PROGRAM_FAILURE)
        {
            logger()->error("Failed to build the program: {} ({})", err.what(), err.err());
            throw err;
        }
    }

    std::string build_log;

    try
    {
        program.getBuildInfo(m_device->getUnderlyingDevice(), CL_PROGRAM_BUILD_LOG, &build_log);
    }
    catch (cl::Error& err)
    {
        logger()->error("Failed get build info, {}", err.what());
    }

    return build_log;
}

// -

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
