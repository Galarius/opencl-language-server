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
};

struct IDiagnostics
{
    virtual void SetBuildOptions(const nlohmann::json& options) = 0;
    virtual void SetMaxProblemsCount(int maxNumberOfProblems) = 0;
    virtual void SetOpenCLDevice(uint32_t identifier) = 0;
    virtual nlohmann::json Get(const Source& source) = 0;
};

std::shared_ptr<IDiagnostics> CreateDiagnostics(std::shared_ptr<ICLInfo> clInfo);

} // namespace ocls
