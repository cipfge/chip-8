#include "emulator.hpp"

#if defined(_WIN64) || defined(_WIN32)
#include <Windows.h>
#endif // Windows

static int emulator_main(int argc, char *argv[])
{
    Emulator chip8;
    if (!chip8.init())
        return -1;
    chip8.run(argc, argv);

    return 0;
}

#if defined(_WIN64) || defined(_WIN32)
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
#else
int main(int argc, char *argv[])
{
    return emulator_main(argc, argv);
}
#endif // Platform entry-point
