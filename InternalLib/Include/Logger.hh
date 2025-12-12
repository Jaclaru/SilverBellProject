#pragma once

#include "InternalLibMarco.hh"

#include <spdlog/spdlog.h>

namespace SilverBell::Utility
{
    class INTERNALLIB_API Logger
    {
    public:

        static void InitializeAsyncLogging();

        static spdlog::logger* GetLogger();
    };

}

// --- 编译期日志等级开关 ---
// 默认启用到 INFO，如需更严格控制，在编译选项里定义：
// /D SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN  或  -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN
#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#define SILVER_BELL_LOG(Level, Format, ...) \
    ::SilverBell::Utility::Logger::GetLogger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, Level, Format, ##__VA_ARGS__)

// 分级宏：按 SPDLOG_ACTIVE_LEVEL 在编译期关闭
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_TRACE(Format, ...)    SILVER_BELL_LOG(spdlog::level::trace, Format, ##__VA_ARGS__)
#else
#define LOG_TRACE(Format, ...)    do {} while(0)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG(Format, ...)    SILVER_BELL_LOG(spdlog::level::debug, Format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(Format, ...)    do {} while(0)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_INFO(Format, ...)     SILVER_BELL_LOG(spdlog::level::info, Format, ##__VA_ARGS__)
#else
#define LOG_INFO(Format, ...)     do {} while(0)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_WARN(Format, ...)     SILVER_BELL_LOG(spdlog::level::warn, Format, ##__VA_ARGS__)
#else
#define LOG_WARN(Format, ...)     do {} while(0)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_ERROR(Format, ...)    SILVER_BELL_LOG(spdlog::level::err, Format, ##__VA_ARGS__)
#else
#define LOG_ERROR(Format, ...)    do {} while(0)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define LOG_CRITICAL(Format, ...) SILVER_BELL_LOG(spdlog::level::critical, Format, ##__VA_ARGS__)
#else
#define LOG_CRITICAL(Format, ...) do {} while(0)
#endif
