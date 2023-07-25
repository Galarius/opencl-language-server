//
//  lsp.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/16/21.
//

#pragma once

#include <memory>
#include <optional>

#include "diagnostics.hpp"
#include "jsonrpc.hpp"

namespace ocls {

struct ILSPServer
{
    virtual ~ILSPServer() = default;

    virtual int Run() = 0;
    virtual void Interrupt() = 0;
};

struct ILSPServerEventsHandler
{
    virtual ~ILSPServerEventsHandler() = default;

    virtual void BuildDiagnosticsRespond(const std::string &uri, const std::string &content) = 0;
    virtual void GetConfiguration() = 0;
    virtual std::optional<nlohmann::json> GetNextResponse() = 0;
    virtual void OnInitialize(const nlohmann::json &data) = 0;
    virtual void OnInitialized(const nlohmann::json &data) = 0;
    virtual void OnTextOpen(const nlohmann::json &data) = 0;
    virtual void OnTextChanged(const nlohmann::json &data) = 0;
    virtual void OnConfiguration(const nlohmann::json &data) = 0;
    virtual void OnRespond(const nlohmann::json &data) = 0;
    virtual void OnShutdown(const nlohmann::json &data) = 0;
    virtual void OnExit() = 0;
};

std::shared_ptr<ILSPServerEventsHandler> CreateLSPEventsHandler(
    std::shared_ptr<IJsonRPC> jrpc, std::shared_ptr<IDiagnostics> diagnostics);

std::shared_ptr<ILSPServer> CreateLSPServer(
    std::shared_ptr<IJsonRPC> jrpc, std::shared_ptr<ILSPServerEventsHandler> handler);

std::shared_ptr<ILSPServer> CreateLSPServer(
    std::shared_ptr<IJsonRPC> jrpc, std::shared_ptr<IDiagnostics> diagnostics);

} // namespace ocls
