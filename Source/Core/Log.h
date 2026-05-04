#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

namespace Kojeom
{
class Log
{
public:
    static void Init(const std::string& logFilePath = "engine.log")
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true);
        std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };
        auto logger = std::make_shared<spdlog::logger>("engine", sinks.begin(), sinks.end());
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        logger->set_level(spdlog::level::debug);
        spdlog::set_default_logger(logger);
        s_initialized = true;
    }

    static bool IsInitialized() { return s_initialized; }

private:
    static bool s_initialized;
};
}

#define KE_LOG_INFO(...)    do { if(Kojeom::Log::IsInitialized()) spdlog::info(__VA_ARGS__); } while(0)
#define KE_LOG_WARN(...)    do { if(Kojeom::Log::IsInitialized()) spdlog::warn(__VA_ARGS__); } while(0)
#define KE_LOG_ERROR(...)   do { if(Kojeom::Log::IsInitialized()) spdlog::error(__VA_ARGS__); } while(0)
#define KE_LOG_DEBUG(...)   do { if(Kojeom::Log::IsInitialized()) spdlog::debug(__VA_ARGS__); } while(0)
