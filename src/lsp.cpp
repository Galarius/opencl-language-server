//
//  lsp.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/16/21.
//

#include "diagnostics.hpp"
#include "jsonrpc.hpp"
#include "log.hpp"
#include "lsp.hpp"
#include "utils.hpp"

#include <atomic>
#include <iostream>
#include <queue>

using namespace nlohmann;

namespace {

constexpr int BuildOptions = 0;
constexpr int MaxProblemsCount = 1;
constexpr int DeviceID = 2;
constexpr int NumConfigurations = 2;

std::optional<nlohmann::json> GetNestedValue(const nlohmann::json &j, const std::vector<std::string> &keys)
{
    const nlohmann::json *current = &j;
    for (const auto &key : keys)
    {
        if (!current->contains(key))
        {
            return std::nullopt;
        }
        current = &(*current)[key];
    }
    return *current;
}

} // namespace

namespace ocls {

auto logger() { return spdlog::get(ocls::LogName::lsp); }

struct Capabilities
{
    bool hasConfigurationCapability = false;
    bool supportDidChangeConfiguration = false;
};

class LSPServer final
    : public ILSPServer
    , public std::enable_shared_from_this<LSPServer>
{
public:
    LSPServer(std::shared_ptr<IJsonRPC> jrpc, std::shared_ptr<ILSPServerEventsHandler> handler)
        : m_jrpc {std::move(jrpc)}
        , m_handler {std::move(handler)}
    {}

    int Run();
    void Interrupt();

private:
    void BuildDiagnosticsRespond(const std::string &uri, const std::string &content);
    void GetConfiguration();
    void OnInitialize(const json &data);
    void OnInitialized(const json &data);
    void OnTextOpen(const json &data);
    void OnTextChanged(const json &data);
    void OnConfiguration(const json &data);
    void OnRespond(const json &data);
    void OnShutdown(const json &data);
    void OnExit();

private:
    std::shared_ptr<IJsonRPC> m_jrpc;
    std::shared_ptr<ILSPServerEventsHandler> m_handler;
    std::atomic<bool> m_interrupted = {false};
};

class LSPServerEventsHandler final : public ILSPServerEventsHandler
{
public:
    LSPServerEventsHandler(
        std::shared_ptr<IJsonRPC> jrpc,
        std::shared_ptr<IDiagnostics> diagnostics,
        std::shared_ptr<utils::IGenerator> generator,
        std::shared_ptr<utils::IExitHandler> exitHandler)
        : m_jrpc {std::move(jrpc)}
        , m_diagnostics {std::move(diagnostics)}
        , m_generator {std::move(generator)}
        , m_exitHandler {std::move(exitHandler)}
    {}

    void BuildDiagnosticsRespond(const std::string &uri, const std::string &content);
    void GetConfiguration();
    std::optional<json> GetNextResponse();
    void OnInitialize(const json &data);
    void OnInitialized(const json &data);
    void OnTextOpen(const json &data);
    void OnTextChanged(const json &data);
    void OnConfiguration(const json &data);
    void OnRespond(const json &data);
    void OnShutdown(const json &data);
    void OnExit();

private:
    std::shared_ptr<IJsonRPC> m_jrpc;
    std::shared_ptr<IDiagnostics> m_diagnostics;
    std::shared_ptr<utils::IGenerator> m_generator;
    std::shared_ptr<utils::IExitHandler> m_exitHandler;
    std::queue<json> m_outQueue;
    Capabilities m_capabilities;
    std::queue<std::pair<std::string, std::string>> m_requests;
    bool m_shutdown = false;
};

// ILSPServerEventsHandler

void LSPServerEventsHandler::GetConfiguration()
{
    if (!m_capabilities.hasConfigurationCapability)
    {
        logger()->trace("Does not have configuration capability");
        return;
    }

    logger()->trace("Make configuration request");
    json buildOptions = {{"section", "OpenCL.server.buildOptions"}};
    json maxNumberOfProblems = {{"section", "OpenCL.server.maxNumberOfProblems"}};
    json openCLDeviceID = {{"section", "OpenCL.server.deviceID"}};
    const auto requestId = m_generator->GenerateID();
    m_requests.push(std::make_pair("workspace/configuration", requestId));
    m_outQueue.push(
        {{"id", requestId},
         {"method", "workspace/configuration"},
         {"params", {{"items", json::array({buildOptions, maxNumberOfProblems, openCLDeviceID})}}}});
}

std::optional<json> LSPServerEventsHandler::GetNextResponse()
{
    if (m_outQueue.empty())
    {
        return std::nullopt;
    }

    auto data = m_outQueue.front();
    m_outQueue.pop();
    return data;
}

void LSPServerEventsHandler::OnInitialize(const json &data)
{
    logger()->trace("Received 'initialize' request");
    if (!data.contains("id"))
    {
        logger()->error("'initialize' message does not contain 'id'");
        return;
    }
    auto requestId = data["id"];

    auto configurationCapability = GetNestedValue(data, {"params", "capabilities", "workspace", "configuration"});
    if (configurationCapability)
    {
        m_capabilities.hasConfigurationCapability = configurationCapability->get<bool>();
    }

    auto didChangeConfiguration =
        GetNestedValue(data, {"params", "capabilities", "workspace", "didChangeConfiguration", "dynamicRegistration"});
    if (didChangeConfiguration)
    {
        m_capabilities.supportDidChangeConfiguration = didChangeConfiguration->get<bool>();
    }

    auto configuration = GetNestedValue(data, {"params", "initializationOptions", "configuration"});
    if (configuration)
    {
        auto buildOptions = GetNestedValue(*configuration, {"buildOptions"});
        auto maxNumberOfProblems = GetNestedValue(*configuration, {"maxNumberOfProblems"});
        auto deviceID = GetNestedValue(*configuration, {"deviceID"});
        if (buildOptions)
        {
            m_diagnostics->SetBuildOptions(*buildOptions);
        }
        if (maxNumberOfProblems)
        {
            m_diagnostics->SetMaxProblemsCount(*maxNumberOfProblems);
        }
        if (deviceID)
        {
            m_diagnostics->SetOpenCLDevice(*deviceID);
        }
    }

    json capabilities = {
        {"textDocumentSync",
         {
             {"openClose", true},
             {"change", 1}, // TextDocumentSyncKind.Full
             {"willSave", false},
             {"willSaveWaitUntil", false},
             {"save", false},
         }},
    };

    m_outQueue.push({{"id", requestId}, {"result", {{"capabilities", capabilities}}}});
}

void LSPServerEventsHandler::OnInitialized(const json &)
{
    logger()->trace("Received 'initialized' message");

    if (!m_capabilities.supportDidChangeConfiguration)
    {
        logger()->trace("Does not support didChangeConfiguration registration");
        return;
    }

    json registrations = {{
        {"id", m_generator->GenerateID()},
        {"method", "workspace/didChangeConfiguration"},
    }};

    json params = {
        {"registrations", registrations},
    };

    m_outQueue.push({{"id", m_generator->GenerateID()}, {"method", "client/registerCapability"}, {"params", params}});
}

void LSPServerEventsHandler::BuildDiagnosticsRespond(const std::string &uri, const std::string &content)
{
    try
    {
        const auto filePath = utils::UriToFilePath(uri);
        logger()->trace("'{}' -> '{}'", uri, filePath);
        json diags = m_diagnostics->GetDiagnostics({filePath, content});
        m_outQueue.push(
            {{"method", "textDocument/publishDiagnostics"},
             {"params",
              {
                  {"uri", uri},
                  {"diagnostics", diags},
              }}});
    }
    catch (std::exception &err)
    {
        auto msg = std::string("Failed to get diagnostics: ") + err.what();
        logger()->error(msg);
        m_jrpc->WriteError(JRPCErrorCode::InternalError, msg);
    }
}

void LSPServerEventsHandler::OnTextOpen(const json &data)
{
    logger()->trace("Received 'textOpen' message");
    auto uri = GetNestedValue(data, {"params", "textDocument", "uri"});
    auto content = GetNestedValue(data, {"params", "textDocument", "text"});
    if (uri && content)
    {
        BuildDiagnosticsRespond(uri->get<std::string>(), content->get<std::string>());
    }
}

void LSPServerEventsHandler::OnTextChanged(const json &data)
{
    logger()->trace("Received 'textChanged' message");
    auto uri = GetNestedValue(data, {"params", "textDocument", "uri"});
    auto contentChanges = GetNestedValue(data, {"params", "contentChanges"});
    if (contentChanges && contentChanges->size() > 0)
    {
        // Only one content change with the full content of the document is supported.
        auto lastIdx = contentChanges->size() - 1;
        auto lastContent = (*contentChanges)[lastIdx];
        if (lastContent.contains("text"))
        {
            auto text = lastContent["text"].get<std::string>();
            BuildDiagnosticsRespond(uri->get<std::string>(), text);
        }
    }
}

void LSPServerEventsHandler::OnConfiguration(const json &data)
{
    logger()->trace("Received 'configuration' respond");

    try
    {
        auto result = data.at("result");
        if (result.empty())
        {
            logger()->warn("Empty configuration");
            return;
        }

        if (result.size() < NumConfigurations)
        {
            logger()->warn("Unexpected number of options");
            return;
        }

        if (result[BuildOptions].is_array())
        {
            m_diagnostics->SetBuildOptions(result[BuildOptions]);
        }

        if (result[MaxProblemsCount].is_number_integer())
        {
            auto maxProblemsCount = result[MaxProblemsCount].get<int64_t>();
            m_diagnostics->SetMaxProblemsCount(static_cast<int>(maxProblemsCount));
        }

        if (result[DeviceID].is_number_integer())
        {
            auto deviceID = result[DeviceID].get<int64_t>();
            m_diagnostics->SetOpenCLDevice(static_cast<uint32_t>(deviceID));
        }
    }
    catch (std::exception &err)
    {
        logger()->error("Failed to update settings, {}", err.what());
    }
}

void LSPServerEventsHandler::OnRespond(const json &data)
{
    logger()->trace("Received client respond");
    if (m_requests.empty())
    {
        logger()->warn("Unexpected respond {}", data.dump());
        return;
    }

    try
    {
        const auto id = data["id"];
        auto request = m_requests.front();
        if (id == request.second && "workspace/configuration" == request.first)
        {
            OnConfiguration(data);
        }
        else
        {
            logger()->warn("Out of order respond");
        }
        m_requests.pop();
    }
    catch (std::exception &err)
    {
        logger()->error("OnRespond failed, {}", err.what());
    }
}

void LSPServerEventsHandler::OnShutdown(const json &data)
{
    logger()->trace("Received 'shutdown' request");
    m_outQueue.push({{"id", data["id"]}, {"result", nullptr}});
    m_shutdown = true;
}

void LSPServerEventsHandler::OnExit()
{
    logger()->trace("Received 'exit', after 'shutdown': {}", utils::FormatBool(m_shutdown));
    if (m_shutdown)
    {
        m_exitHandler->OnExit(EXIT_SUCCESS);
    }
    else
    {
        m_exitHandler->OnExit(EXIT_FAILURE);
    }
}

// ILSPServer

int LSPServer::Run()
{
    logger()->trace("Setting up...");
    auto self = this->shared_from_this();
    // clang-format off
    // Register handlers for methods
    m_jrpc->RegisterMethodCallback("initialize", [self](const json &request)
    {
        self->m_handler->OnInitialize(request);
    });
    m_jrpc->RegisterMethodCallback("initialized", [self](const json &request)
    {
        self->m_handler->OnInitialized(request);
    });
    m_jrpc->RegisterMethodCallback("shutdown", [self](const json &request)
    {
        self->m_handler->OnShutdown(request);
    });
    m_jrpc->RegisterMethodCallback("exit", [self](const json &)
    {
        self->m_handler->OnExit();
    });
    m_jrpc->RegisterMethodCallback("textDocument/didOpen", [self](const json &request)
    {
        self->m_handler->OnTextOpen(request);
    });
    m_jrpc->RegisterMethodCallback("textDocument/didChange", [self](const json &request)
    {
        self->m_handler->OnTextChanged(request);
    });
    m_jrpc->RegisterMethodCallback("workspace/didChangeConfiguration", [self](const json &)
    {
        self->m_handler->GetConfiguration();
    });
    // Register handler for client responds
    m_jrpc->RegisterInputCallback([self](const json &respond)
    {
        self->m_handler->OnRespond(respond);
    });
    // Register handler for message delivery
    m_jrpc->RegisterOutputCallback([](const std::string &message)
    {
        #if defined(WIN32)
            printf_s("%s", message.c_str());
            fflush(stdout);
        #else
            std::cout << message << std::flush;
        #endif    
    });
    // clang-format on

    logger()->trace("Listening...");
    char c;
    while (std::cin.get(c))
    {
        if (m_interrupted.load())
        {
            return EINTR;
        }
        m_jrpc->Consume(c);
        if (m_jrpc->IsReady())
        {
            m_jrpc->Reset();
            while (true)
            {
                auto data = m_handler->GetNextResponse();
                if (!data.has_value())
                {
                    break;
                }
                m_jrpc->Write(*data);
            }
        }
    }
    return 0;
}

void LSPServer::Interrupt()
{
    m_interrupted.store(true);
}

std::shared_ptr<ILSPServerEventsHandler> CreateLSPEventsHandler(
    std::shared_ptr<IJsonRPC> jrpc,
    std::shared_ptr<IDiagnostics> diagnostics,
    std::shared_ptr<utils::IGenerator> generator,
    std::shared_ptr<utils::IExitHandler> exitHandler)
{
    return std::make_shared<LSPServerEventsHandler>(jrpc, diagnostics, generator, exitHandler);
}

std::shared_ptr<ILSPServer> CreateLSPServer(
    std::shared_ptr<IJsonRPC> jrpc, std::shared_ptr<ILSPServerEventsHandler> handler)
{
    return std::make_shared<LSPServer>(std::move(jrpc), std::move(handler));
}

std::shared_ptr<ILSPServer> CreateLSPServer(std::shared_ptr<IJsonRPC> jrpc, std::shared_ptr<IDiagnostics> diagnostics)
{
    auto generator = utils::CreateDefaultGenerator();
    auto exitHandler = utils::CreateDefaultExitHandler();
    auto handler = std::make_shared<LSPServerEventsHandler>(jrpc, diagnostics, generator, exitHandler);
    return std::make_shared<LSPServer>(std::move(jrpc), std::move(handler));
}

} // namespace ocls
