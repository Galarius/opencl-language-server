//
//  jsonrpc.cpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/16/21.
//

#include "jsonrpc.hpp"
#include "log.hpp"
#include "utils.hpp"

#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_map>

using namespace nlohmann;

namespace ocls {

namespace {

auto logger() { return spdlog::get(ocls::LogName::jrpc); }

constexpr char LE[] = "\r\n";

} // namespace

class JsonRPC final : public IJsonRPC
{
public:
    friend std::ostream& operator<<(std::ostream& out, JRPCErrorCode const& code)
    {
        out << static_cast<int64_t>(code);
        return out;
    }

    /**
     Register callback to be notified on the specific method notification.
     All unregistered notifications will be responded with MethodNotFound automatically.
     */
    void RegisterMethodCallback(const std::string& method, InputCallbackFunc&& func);
    /**
     Register callback to be notified on client responds to server (our) requests.
     */
    void RegisterInputCallback(InputCallbackFunc&& func);
    /**
     Register callback to be notified when server is going to send the final message to the client.
     Basically it should be redirected to the stdout.
     */
    void RegisterOutputCallback(OutputCallbackFunc&& func);

    void Consume(char c);
    bool IsReady() const;
    void Write(const nlohmann::json& data) const;
    void Reset();
    /**
     Send trace message to client.
     */
    void WriteTrace(const std::string& message, const std::string& verbose);
    void WriteError(JRPCErrorCode errorCode, const std::string& message) const;

private:
    void ProcessBufferContent();
    void ProcessMethod();
    void ProcessBufferHeader();

    void OnInitialize();
    void OnTracingChanged(const nlohmann::json& data);
    bool ReadHeader();
    void FireMethodCallback();
    void FireRespondCallback();

    void LogBufferContent() const;
    void LogMessage(const std::string& message) const;
    void LogAndHandleParseError(std::exception& e);
    void LogAndHandleUnexpectedMessage();

private:
    std::string m_method;
    std::string m_buffer;
    nlohmann::json m_body;
    std::unordered_map<std::string, std::string> m_headers;
    std::unordered_map<std::string, InputCallbackFunc> m_callbacks;
    OutputCallbackFunc m_outputCallback;
    InputCallbackFunc m_respondCallback;
    bool m_isProcessing = true;
    bool m_initialized = false;
    bool m_validHeader = false;
    bool m_tracing = false;
    bool m_verbosity = false;
    unsigned long m_contentLength = 0;
    std::regex m_headerRegex {"([\\w-]+): (.+)\\r\\n(?:([^:]+)\\r\\n)?"};
};

void JsonRPC::RegisterMethodCallback(const std::string& method, InputCallbackFunc&& func)
{
    logger()->trace("Set callback for method: {}", method);
    m_callbacks[method] = std::move(func);
}

void JsonRPC::RegisterInputCallback(InputCallbackFunc&& func)
{
    logger()->trace("Set callback for client responds");
    m_respondCallback = std::move(func);
}

void JsonRPC::RegisterOutputCallback(OutputCallbackFunc&& func)
{
    logger()->trace("Set output callback");
    m_outputCallback = std::move(func);
}

void JsonRPC::Consume(char c)
{
    m_buffer += c;

    if (m_validHeader)
    {
        ProcessBufferContent();
    }
    else
    {
        ProcessBufferHeader();
    }
}

bool JsonRPC::IsReady() const
{
    return !m_isProcessing;
}

void JsonRPC::Write(const json& data) const
{
    assert(m_outputCallback);

    std::string message;
    try
    {
        json body = data;
        body.emplace("jsonrpc", "2.0");
        std::string content = body.dump();
        message.append(std::string("Content-Length: ") + std::to_string(content.size()) + LE);
        message.append(std::string("Content-Type: application/vscode-jsonrpc;charset=utf-8") + LE);
        message.append(LE);
        message.append(content);

        LogMessage(message);

        m_outputCallback(message);
    }
    catch (std::exception& err)
    {
        logger()->error("Failed to write message: '{}', error: {}", message, err.what());
    }
}

void JsonRPC::Reset()
{
    m_method = std::string();
    m_buffer.clear();
    m_body.clear();
    m_headers.clear();
    m_validHeader = false;
    m_contentLength = 0;
    m_isProcessing = true;
}

void JsonRPC::WriteTrace(const std::string& message, const std::string& verbose)
{
    if (!m_tracing)
    {
        logger()->debug("JRPC tracing is disabled");
        logger()->trace("The message was: '{}', verbose: {}", message, verbose);
        return;
    }

    if (!verbose.empty() && !m_verbosity)
    {
        logger()->debug("JRPC verbose tracing is disabled");
        logger()->trace("The verbose message was: {}", verbose);
    }

    // clang-format off
    Write(json({
        {"method", "$/logTrace"}, 
        {"params", {
            {"message", message}, 
            {"verbose", m_verbosity ? verbose : ""
        }}}
    }));
    // clang-format on
}

// private

void JsonRPC::ProcessBufferContent()
{
    if (m_buffer.length() != m_contentLength)
    {
        return;
    }

    try
    {
        LogBufferContent();

        m_body = json::parse(m_buffer);
        const auto method = m_body["method"];
        if (method.is_string())
        {
            m_method = method.get<std::string>();
            ProcessMethod();
        }
        else
        {
            FireRespondCallback();
        }
        m_isProcessing = false;
    }
    catch (std::exception& e)
    {
        LogAndHandleParseError(e);
    }
}

void JsonRPC::ProcessMethod()
{
    if (m_method == "initialize")
    {
        OnInitialize();
    }
    else if (!m_initialized)
    {
        return LogAndHandleUnexpectedMessage();
    }
    else if (m_method == "$/setTrace")
    {
        OnTracingChanged(m_body);
    }
    FireMethodCallback();
}

void JsonRPC::ProcessBufferHeader()
{
    if (ReadHeader())
    {
        m_buffer.clear();
    }

    if (m_buffer == LE)
    {
        m_buffer.clear();
        m_validHeader = m_contentLength > 0;
        if (m_validHeader)
        {
            m_buffer.reserve(m_contentLength);
        }
        else
        {
            WriteError(JRPCErrorCode::InvalidRequest, "Invalid content length");
        }
    }
}

void JsonRPC::OnInitialize()
{
    try
    {
        const auto traceValue = m_body["params"]["trace"].get<std::string>();
        m_tracing = traceValue != "off";
        m_verbosity = traceValue == "verbose";
        m_initialized = true;
        logger()->trace("Tracing options: is verbose: {}, is on: {}",
                        utils::FormatBool(m_verbosity),
                        utils::FormatBool(m_tracing));
    }
    catch (std::exception& err)
    {
        logger()->error("Failed to read tracing options, {}", err.what());
    }
}

void JsonRPC::OnTracingChanged(const json& data)
{
    try
    {
        const auto traceValue = data["params"]["value"].get<std::string>();
        m_tracing = traceValue != "off";
        m_verbosity = traceValue == "verbose";
        logger()->trace("Tracing options were changed, is verbose: {}, is on: {}",
                        utils::FormatBool(m_verbosity),
                        utils::FormatBool(m_tracing));
    }
    catch (std::exception& err)
    {
        logger()->error("Failed to read tracing options, {}", err.what());
    }
}

bool JsonRPC::ReadHeader()
{
    std::sregex_iterator next(m_buffer.begin(), m_buffer.end(), m_headerRegex);
    std::sregex_iterator end;
    const bool found = std::distance(next, end) > 0;
    while (next != end)
    {
        std::smatch match = *next;
        std::string key = match.str(1);
        std::string value = match.str(2);
        if (key == "Content-Length")
        {
            m_contentLength = std::stoi(value);
        }
        m_headers[key] = value;
        ++next;
    }
    return found;
}

void JsonRPC::FireRespondCallback()
{
    if (m_respondCallback)
    {
        logger()->trace("Calling handler for a client respond");
        m_respondCallback(m_body);
    }
}

void JsonRPC::FireMethodCallback()
{
    assert(m_outputCallback);
    auto callback = m_callbacks.find(m_method);
    if (callback == m_callbacks.end())
    {
        const bool isRequest = m_body["params"]["id"] != nullptr;
        const bool mustRespond = isRequest || m_method.rfind("$/", 0) == std::string::npos;
        logger()->trace("Got request: {}, respond is required: {}",
                        utils::FormatBool(isRequest),
                        utils::FormatBool(mustRespond));
        if (mustRespond)
        {
            WriteError(JRPCErrorCode::MethodNotFound, "Method '" + m_method + "' is not supported.");
        }
    }
    else
    {
        try
        {
            logger()->trace("Calling handler for method: '{}'", m_method);
            callback->second(m_body);
        }
        catch (std::exception& err)
        {
            logger()->error("Failed to handle method '{}', err: {}", m_method, err.what());
        }
    }
}

void JsonRPC::WriteError(JRPCErrorCode errorCode, const std::string& message) const
{
    logger()->trace("Reporting error: '{}' ({})", message, static_cast<int>(errorCode));
    json obj = {
        {"error",
         {
             {"code", static_cast<int>(errorCode)},
             {"message", message},
         }}};
    Write(obj);
}

void JsonRPC::LogBufferContent() const
{
    if (spdlog::get_level() > spdlog::level::debug)
    {
        return;
    }

    std::stringstream ss;
    ss << "\n>>>>>>>>>>>>>>>>\n";
    for (auto& header : m_headers)
    {
        ss << header.first << ": " << header.second;
    }
    ss << m_buffer;
    ss << "\n>>>>>>>>>>>>>>>>\n";

    logger()->debug(ss.str());
}

void JsonRPC::LogMessage(const std::string& message) const
{
    if (spdlog::get_level() > spdlog::level::debug)
    {
        return;
    }
    
    std::stringstream ss;
    ss  << "\n<<<<<<<<<<<<<<<<\n"
        << message
        << "\n<<<<<<<<<<<<<<<<\n";

    logger()->debug(ss.str());
}

void JsonRPC::LogAndHandleParseError(std::exception& e)
{
    logger()->error("Failed to parse request with reason: '{}'\n{}", e.what(), "\n", m_buffer);
    m_buffer.clear();
    WriteError(JRPCErrorCode::ParseError, "Failed to parse request");
}

void JsonRPC::LogAndHandleUnexpectedMessage()
{
    logger()->error("Unexpected first message: '{}'", m_method);
    WriteError(JRPCErrorCode::NotInitialized, "Server was not initialized.");
}

std::shared_ptr<IJsonRPC> CreateJsonRPC()
{
    return std::shared_ptr<IJsonRPC>(new JsonRPC());
}

} // namespace ocls
