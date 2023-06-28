#include "logger.hpp"
#include <iostream>
#include <sstream>
#include <cstdarg>
#include <SDL.h>

#if defined(_WIN64) || defined(_WIN32)
#include <Windows.h>
#endif // Windows

namespace logger
{

enum Level
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFORMATION,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_MAX
};

inline void log_write_error(const std::string &message)
{
#ifndef EMULATOR_DEBUG_ENABLED
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr);
#elif defined(_WIN64) || defined(_WIN32)
    OutputDebugStr(message).c_str();
#else
    std::cerr << message << "\n";
#endif // debug enabled and platform
}

inline void log_write(const std::string &message)
{
#if defined(_WIN64) || defined(_WIN32)
    OutputDebugStr(message).c_str();
#else
    std::cout << message << "\n";
#endif // platform
}

void log_format(Level level, const char *fmt, std::va_list args)
{
    constexpr std::size_t LOG_BUFFER_SIZE = 1024;
    static const char *log_level_names[LOG_LEVEL_MAX] = { "[debug]", "[info]", "[warning]", "[error]" };

    char buffer[LOG_BUFFER_SIZE] = { 0 };
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);

    std::stringstream ss;
    ss << log_level_names[level] << ": " <<  buffer;

    if (level == LOG_LEVEL_ERROR)
        log_write_error(ss.str());
    else
        log_write(ss.str());
}

void debug(const char *fmt, ...)
{
#ifdef EMULATOR_DEBUG_ENABLED
    std::va_list args;
    va_start(args, fmt);
    log_format(LOG_LEVEL_DEBUG, fmt, args);
    va_end(args);
#endif // debug enabled
}

void info(const char *fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    log_format(LOG_LEVEL_INFORMATION, fmt, args);
    va_end(args);
}

void warning(const char *fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    log_format(LOG_LEVEL_WARNING, fmt, args);
    va_end(args);
}

void error(const char *fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    log_format(LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
}

} // namespace logger
