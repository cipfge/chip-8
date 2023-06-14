#include "platform.hpp"
#include "emulator.hpp"

static int emulator_main(int argc, char *argv[])
{
    Emulator chip8;
    if (!chip8.init(argc, argv))
        return -1;
    chip8.run();

    return 0;
}

#if defined(EMULATOR_PLATFORM_LINUX) || defined(EMULATOR_PLATFORM_APPLE)

int main(int argc, char *argv[])
{
    return emulator_main(argc, argv);
}

#else // Windows

#include <Windows.h>

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;

    return emulator_main(__argc, __argv);
}

#endif // Platform entry-point
