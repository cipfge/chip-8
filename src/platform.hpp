#pragma once

#include <string>

struct SDL_Window;

namespace platform
{

std::string open_file_dialog(SDL_Window* owner);

} // namespace platform
