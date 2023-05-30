//
//  lsp.cpp
//  opencl-language-server
//
//  Created by Ilya Shoshin (Galarius) on 7/16/21.
//

#include "lsp.hpp"
#include "diagnostics.hpp"
#include "jsonrpc.hpp"
#include "utils.hpp"

#include <atomic>
#include <glogger.hpp>

#include <queue>

using namespace nlohmann;

namespace vscode::opencl {

constexpr char TracePrefix[] = "#lsp ";

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
    LSPServer() : m_diagnostics(CreateDiagnostics(CreateCLInfo())) {}

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
    JsonRPC m_jrpc;
    std::shared_ptr<IDiagnostics> m_diagnostics;
    std::queue<json> m_outQueue;
    Capabilities m_capabilities;
    std::queue<std::pair<std::string, std::string>> m_requests;
    bool m_shutdown = false;
    std::atomic<bool> m_interrupted = {false};
};

void LSPServer::GetConfiguration()
{
    if (!m_capabilities.hasConfigurationCapability)
    {
        GLogDebug(TracePrefix, "Does not have configuration capability");
        return;
    }
    GLogDebug(TracePrefix, "Make configuration request");
    json buildOptions = {{"section", "OpenCL.server.buildOptions"}};
    json maxNumberOfProblems = {{"section", "OpenCL.server.maxNumberOfProblems"}};
    json openCLDeviceID = {{"section", "OpenCL.server.deviceID"}};
    const auto requestId = utils::GenerateId();
    m_requests.push(std::make_pair("workspace/configuration", requestId));
    m_outQueue.push(
        {{"id", requestId},
         {"method", "workspace/configuration"},
         {"params", {{"items", json::array({buildOptions, maxNumberOfProblems, openCLDeviceID})}}}});
}


void LSPServer::OnInitialize(const json &data)
{
    GLogDebug(TracePrefix, "Received 'initialize' request");
    try
    {
        m_capabilities.hasConfigurationCapability =
            data["params"]["capabilities"]["workspace"]["configuration"].get<bool>();
        m_capabilities.supportDidChangeConfiguration =
            data["params"]["capabilities"]["workspace"]["didChangeConfiguration"]["dynamicRegistration"].get<bool>();
        auto configuration = data["params"]["initializationOptions"]["configuration"];

        auto buildOptions = configuration["buildOptions"];
        m_diagnostics->SetBuildOptions(buildOptions);

        auto maxNumberOfProblems = configuration["maxNumberOfProblems"].get<int64_t>();
        m_diagnostics->SetMaxProblemsCount(static_cast<int>(maxNumberOfProblems));

        auto deviceID = configuration["deviceID"].get<int64_t>();
        m_diagnostics->SetOpenCLDevice(static_cast<uint32_t>(deviceID));
    }
    catch (std::exception &err)
    {
        GLogError(TracePrefix, "Failed to parse initialize parameters: ", err.what());
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

    m_outQueue.push({{"id", data["id"]}, {"result", {{"capabilities", capabilities}}}});
}

void LSPServer::OnInitialized(const json &)
{
    GLogDebug(TracePrefix, "Received 'initialized' message");
    if (!m_capabilities.supportDidChangeConfiguration)
    {
        GLogDebug(TracePrefix, "Does not support didChangeConfiguration registration");
        return;
    }

    json registrations = {{
        {"id", utils::GenerateId()},
        {"method", "workspace/didChangeConfiguration"},
    }};

    json params = {
        {"registrations", registrations},
    };

    m_outQueue.push({{"id", utils::GenerateId()}, {"method", "client/registerCapability"}, {"params", params}});
}

void LSPServer::BuildDiagnosticsRespond(const std::string &uri, const std::string &content)
{
    try
    {
        const auto filePath = utils::UriToPath(uri);
        GLogDebug("Converted uri '", uri, "' to path '", filePath, "'");

        json diags = m_diagnostics->Get({filePath, content});
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
        GLogError(TracePrefix, msg);
        m_jrpc.WriteError(JsonRPC::ErrorCode::InternalError, msg);
    }
}

void LSPServer::OnTextOpen(const json &data)
{
    GLogDebug(TracePrefix, "Received 'textOpen' message");
    std::string srcUri = data["params"]["textDocument"]["uri"].get<std::string>();
    std::string content = data["params"]["textDocument"]["text"].get<std::string>();
    BuildDiagnosticsRespond(srcUri, content);
}

void LSPServer::OnTextChanged(const json &data)
{
    GLogDebug(TracePrefix, "Received 'textChanged' message");
    std::string srcUri = data["params"]["textDocument"]["uri"].get<std::string>();
    std::string content = data["params"]["contentChanges"][0]["text"].get<std::string>();

    BuildDiagnosticsRespond(srcUri, content);
}

void LSPServer::OnConfiguration(const json &data)
{
    GLogDebug(TracePrefix, "Received 'configuration' respond");
    auto result = data["result"];
    if (result.empty())
    {
        GLogWarn(TracePrefix, "Empty result");
        return;
    }

    if (result.size() != 3)
    {
        GLogWarn(TracePrefix, "Unexpected result items count");
        return;
    }

    try
    {
        auto buildOptions = result[0];
        m_diagnostics->SetBuildOptions(buildOptions);

        auto maxProblemsCount = result[1].get<int64_t>();
        m_diagnostics->SetMaxProblemsCount(static_cast<int>(maxProblemsCount));

        auto deviceID = result[2].get<int64_t>();
        m_diagnostics->SetOpenCLDevice(static_cast<uint32_t>(deviceID));
    }
    catch (std::exception &err)
    {
        GLogError(TracePrefix, "Failed to update settings", err.what());
    }
}

void LSPServer::OnRespond(const json &data)
{
    GLogDebug(TracePrefix, "Received client respond");
    const auto id = data["id"];
    if (!m_requests.empty())
    {
        auto request = m_requests.front();
        if (id == request.second && request.first == "workspace/configuration")
            OnConfiguration(data);
        m_requests.pop();
    }
}

void LSPServer::OnShutdown(const json &data)
{
    GLogDebug(TracePrefix, "Received 'shutdown' request");
    m_outQueue.push({{"id", data["id"]}, {"result", nullptr}});
    m_shutdown = true;
}

void LSPServer::OnExit()
{
    GLogDebug(TracePrefix, "Received 'exit', after 'shutdown': ", m_shutdown ? "yes" : "no");
    if (m_shutdown)
        exit(EXIT_SUCCESS);
    else
        exit(EXIT_FAILURE);
}

int LSPServer::Run()
{
    GLogInfo("Setting up...");
    auto self = this->shared_from_this();
    // clang-format off
    // Register handlers for methods
    m_jrpc.RegisterMethodCallback("initialize", [self](const json &request)
    {
        self->OnInitialize(request);
    });
    m_jrpc.RegisterMethodCallback("initialized", [self](const json &request)
    {
        self->OnInitialized(request);
    });
    m_jrpc.RegisterMethodCallback("shutdown", [self](const json &request)
    {
        self->OnShutdown(request);
    });
    m_jrpc.RegisterMethodCallback("exit", [self](const json &)
    {
        self->OnExit();
    });
    m_jrpc.RegisterMethodCallback("textDocument/didOpen", [self](const json &request)
    {
        self->OnTextOpen(request);
    });
    m_jrpc.RegisterMethodCallback("textDocument/didChange", [self](const json &request)
    {
        self->OnTextChanged(request);
    });
    m_jrpc.RegisterMethodCallback("workspace/didChangeConfiguration", [self](const json &)
    {
        self->GetConfiguration();
    });
    // Register handler for client responds
    m_jrpc.RegisterInputCallback([self](const json &respond)
    {
        self->OnRespond(respond);
    });
    // Register handler for message delivery
    m_jrpc.RegisterOutputCallback([](const std::string &message)
    {
        #if defined(WIN32)
            printf_s("%s", message.c_str());
            fflush(stdout);
        #else
            std::cout << message << std::flush;
        #endif    
    });
    // clang-format off
    
    GLogInfo("Listening...");
    char c;
    while (std::cin.get(c))
    {
        if(m_interrupted.load()) {
            return EINTR;
        }
        m_jrpc.Consume(c);
        if (m_jrpc.IsReady())
        {
            m_jrpc.Reset();
            while (!m_outQueue.empty())
            {
                auto data = m_outQueue.front();
                m_jrpc.Write(data);
                m_outQueue.pop();
            }
        }
    }
    return 0;
}

void LSPServer::Interrupt() 
{
    m_interrupted.store(true);
}

std::shared_ptr<ILSPServer> CreateLSPServer()
{
    return std::shared_ptr<ILSPServer>(new LSPServer());
}

} // namespace vscode::opencl
