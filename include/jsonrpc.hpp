//
//  jsonrpc.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 7/16/21.
//

#pragma once

#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

namespace ocls {

using InputCallbackFunc = std::function<void(const nlohmann::json&)>;
using OutputCallbackFunc = std::function<void(const std::string&)>;

// clang-format off
enum class JRPCErrorCode : int
{
    ///@{
    ParseError = -32700,     ///< Parse error    Invalid JSON was received by the server. An error occurred on the
                             ///< server while parsing the JSON text.
    InvalidRequest = -32600, ///< Invalid Request    The JSON sent is not a valid Request object.
    MethodNotFound = -32601, ///< Method not found    The method does not exist / is not available.
    InvalidParams = -32602,  ///< Invalid params    Invalid method parameter(s).
    InternalError = -32603,  ///< Internal error    Internal JSON-RPC error.
    // -32000 to -32099    Server error    Reserved for implementation-defined server-errors.
    NotInitialized = -32002 ///< The first client's message is not equal to "initialize"
    ///@}
};
// clang-format on

struct IJsonRPC
{
    /**
     Register callback to be notified on the specific method notification.
     All unregistered notifications will be responded with MethodNotFound automatically.
    */
    virtual void RegisterMethodCallback(const std::string& method, InputCallbackFunc&& func) = 0;
    /**
     Register callback to be notified on client responds to server (our) requests.
     */
    virtual void RegisterInputCallback(InputCallbackFunc&& func) = 0;
    /**
     Register callback to be notified when server is going to send the final message to the client.
     Basically it should be redirected to the stdout.
     */
    virtual void RegisterOutputCallback(OutputCallbackFunc&& func) = 0;

    virtual void Consume(char c) = 0;
    virtual bool IsReady() const = 0;
    virtual void Write(const nlohmann::json& data) const = 0;
    virtual void Reset() = 0;
    /**
     Send trace message to client.
     */
    virtual void WriteTrace(const std::string& message, const std::string& verbose) = 0;
    virtual void WriteError(JRPCErrorCode errorCode, const std::string& message) const = 0;
};

std::shared_ptr<IJsonRPC> CreateJsonRPC();

} // namespace ocls
