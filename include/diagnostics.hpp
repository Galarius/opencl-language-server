//
//  diagnostics.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/16/21.
//

#pragma once

#include <clinfo.hpp>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <regex>
#include <tuple>

namespace ocls {

struct Source
{
    std::string filePath;
    std::string text;

    bool operator==(const Source& other) const
    {
        return filePath == other.filePath && text == other.text;
    }
};

struct IDiagnosticsParser
{
    virtual ~IDiagnosticsParser() = default;

    virtual std::tuple<std::string, long, long, long, std::string> ParseMatch(const std::smatch& matches) = 0;
    virtual nlohmann::json ParseDiagnostics(
        const std::string& buildLog, const std::string& name, uint64_t problemsLimit) = 0;
};

struct IDiagnostics
{
    virtual ~IDiagnostics() = default;

    virtual void SetBuildOptions(const nlohmann::json& options) = 0;
    virtual void SetBuildOptions(const std::string& options) = 0;
    virtual void SetMaxProblemsCount(uint64_t maxNumberOfProblems) = 0;
    virtual void SetOpenCLDevice(uint32_t identifier) = 0;

    virtual std::string GetBuildLog(const Source& source) = 0;
    virtual nlohmann::json GetDiagnostics(const Source& source) = 0;
};

std::shared_ptr<IDiagnosticsParser> CreateDiagnosticsParser();
std::shared_ptr<IDiagnostics> CreateDiagnostics(std::shared_ptr<ICLInfo> clInfo);
std::shared_ptr<IDiagnostics> CreateDiagnostics(
    std::shared_ptr<ICLInfo> clInfo, std::shared_ptr<IDiagnosticsParser> parser);

} // namespace ocls
