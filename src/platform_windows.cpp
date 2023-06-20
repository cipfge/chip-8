#include "platform.hpp"

#if defined(_WIN64) || defined(_WIN32)
#include <Windows.h>
#include <commdlg.h>
#include <SDL_syswm.h>

namespace platform
{

inline HWND sdl_window_handle(const SDL_Window* window)
{
    if (!window)
        return nullptr;

    SDL_SysWMinfo win_info;
    SDL_VERSION(&win_info.version);
    SDL_GetWindowWMInfo(window, &win_info);

    return win_info.info.win.window;
}

std::string open_file_dialog(const SDL_Window* owner)
{
    constexpr auto PATH_MAX_SIZE = 512;

    CHAR file_name[PATH_MAX_SIZE] = { 0 };
    CHAR current_dir[PATH_MAX_SIZE] = { 0 };

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = sdl_window_handle(owner);
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = sizeof(file_name);
    ofn.lpstrFilter = "CHIP-8 ROM (*.ch8)\0 * .ch8\0All Files (*.*)\0 * .*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetCurrentDirectoryA(PATH_MAX_SIZE, current_dir))
        ofn.lpstrInitialDir = current_dir;

    if (GetOpenFileNameA(&ofn) == TRUE)
        return ofn.lpstrFile;

    return {};
}

} // namespace platform
#endif // Windows
