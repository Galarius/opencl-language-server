//
//  oslogger.hpp
//  opencl-language-server
//
//  Created by Ilia Shoshin on 24/02/24.
//

#pragma once

#if !defined(__APPLE__)
#error "unsupported on this platform"
#endif

#include <string>
#include <mutex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>
#include <os/log.h>

namespace ocls {

template <typename Mutex>
class oslogger_sink : public spdlog::sinks::base_sink<Mutex>
{
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        const std::string message = fmt::to_string(formatted);
        const std::string category(msg.logger_name.begin(), msg.logger_name.end());

        /*
         * From <os/log.h>
         *
         * The logging runtime maintains a global collection of all os_log_t
         * objects, one per subsystem/category pair. The os_log_t for a given
         * subsystem/category is created upon the first call to os_log_create and
         * any subsequent calls return the same object. These objects are never
         * deallocated, so dynamic creation (e.g. on a per-operation basis) is
         * generally inappropriate.
         *
         * A value will always be returned to allow for dynamic enablement.
         */
        const os_log_t log = os_log_create("com.galarius.vscode-opencl.server", category.c_str());

        switch (msg.level)
        {
            case spdlog::level::trace:
            case spdlog::level::debug:
                os_log_debug(log, "%{public}s", message.c_str());
                break;
            case spdlog::level::info:
            case spdlog::level::warn:
                os_log_info(log, "%{public}s", message.c_str());
                break;
            case spdlog::level::err:
                os_log_error(log, "%{public}s", message.c_str());
                break;
            case spdlog::level::critical:
                os_log_fault(log, "%{public}s", message.c_str());
                break;
            default:
                os_log(log, "%{public}s", message.c_str());
                break;
        }
    }

    void flush_() override {}
};

using oslogger_sink_mt = oslogger_sink<std::mutex>;
using oslogger_sink_st = oslogger_sink<spdlog::details::null_mutex>;

} // namespace ocls
