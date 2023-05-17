#pragma once

#if defined(_WIN64) || defined(_WIN32)
    #define EMULATOR_PLATFORM_WINDOWS
#elif defined(__linux__)
    #define EMULATOR_PLATFORM_LINUX
#else
    #error "Unsupported platform detected"
#endif
