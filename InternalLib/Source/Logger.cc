#include "Logger.hh"

#include <spdlog/async.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#ifdef _WIN32
#  include <windows.h>
#  include <fcntl.h>
#endif

using namespace SilverBell::Utility;

namespace 
{
    constexpr std::size_t QUEUE_SIZE = 8192;
    constexpr std::size_t THREAD_COUNT = 1;
    constexpr std::size_t ROTATE_MAX_SIZE = static_cast<std::size_t>(10) * 1024 * 1024;
    constexpr std::size_t ROTATE_MAX_FILES = 3;

    using namespace std::string_literals;
    const auto LOGGER_NAME = "SilverBell Engine"s;
    const auto LOG_FILE = L"Logs/SilverBell_Engine.txt"s;
    constexpr spdlog::level::level_enum LEVEL_ENUM = spdlog::level::trace;
    constexpr spdlog::level::level_enum FLASH_ON_LEVEL_ENUM = spdlog::level::warn;

    std::shared_ptr<spdlog::async_logger> AsyncLogger;
}

void Logger::InitializeAsyncLogging()
{
    static std::once_flag sOnceFlag;
    std::call_once(sOnceFlag, [&]() {

#ifdef _WIN32
        // 切换控制台到 UTF-8 输出与输入
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // 启用虚拟终端（ANSI 转义序列）以支持颜色输出
        const HANDLE Handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (Handle != INVALID_HANDLE_VALUE) 
        {
            DWORD ConsoleMode = 0;
            if (GetConsoleMode(Handle, &ConsoleMode)) 
            {
                ConsoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(Handle, ConsoleMode);
            }
        }

        // 可选：若程序是 GUI 应用且没有控制台，需要 AllocConsole + 重定向 stdout/stderr
        // AllocConsole();
        // freopen("CONOUT$", "w", stdout);
        // freopen("CONOUT$", "w", stderr);
        // _setmode(_fileno(stdout), _O_TEXT); // 或根据需要使用 _O_U8TEXT
#endif

        spdlog::init_thread_pool(QUEUE_SIZE, THREAD_COUNT);

        auto ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto FileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(LOG_FILE, ROTATE_MAX_SIZE, ROTATE_MAX_FILES, true);

        std::vector<spdlog::sink_ptr> Sinks{ ConsoleSink, FileSink };

        AsyncLogger = std::make_shared<spdlog::async_logger>(
            LOGGER_NAME, Sinks.begin(), Sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::overrun_oldest
        );

        AsyncLogger->set_level(LEVEL_ENUM);
        // 缓冲在达到该等级时强制刷新
        AsyncLogger->flush_on(FLASH_ON_LEVEL_ENUM);
        // 设置日志格式，%t会在日志中输出当前线程的 ID（thread id），用于区分不同线程的日志输出。
        AsyncLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s:%# %!] %v");

        spdlog::register_logger(AsyncLogger);
        spdlog::set_default_logger(AsyncLogger);
    });
}

spdlog::logger* Logger::GetLogger()
{
    // 总是保证默认日志器已初始化
    return AsyncLogger.get();
}