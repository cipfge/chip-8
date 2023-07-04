#include "platform.hpp"

#ifdef __linux__
#include <stdio.h>

namespace platform
{

std::string open_file_dialog(SDL_Window* owner)
{
    // Not used in Linux
    (void)owner;

    constexpr auto PATH_MAX_SIZE = 512;
    char selected_file[PATH_MAX_SIZE] = { 0 };

    // TODO: Use fork + exec and auto detect zenity or kdialog
    // const std::string command = "zenity --file-selection --file-filter=\"CHIP-8 ROM (*.ch8) | *.ch8\"";
    const std::string command = "kdialog --getopenfilename $HOME \"CHIP-8 ROM (*.ch8)\"";

    FILE* handle = popen(command.c_str(), "r");
    fgets(selected_file, sizeof(selected_file), handle);
    pclose(handle);

    return std::string(selected_file);
}

} // namespace platform
#endif // Linux
