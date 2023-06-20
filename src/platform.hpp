#pragma once

#include <string>

struct SDL_Window;

namespace platform
{

std::string open_file_dialog(const SDL_Window* owner);

} // namespace platform
