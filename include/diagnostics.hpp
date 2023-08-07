//
//  diagnostics.hpp
//  opencl-language-server
//
//  Created by Ilya Shoshin (Galarius) on 7/16/21.
//

#pragma once

#include <clinfo.hpp>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

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

std::shared_ptr<IDiagnostics> CreateDiagnostics(std::shared_ptr<ICLInfo> clInfo);

} // namespace ocls
