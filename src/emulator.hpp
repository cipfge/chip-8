#pragma once

#include <cstdint>

struct SDL_Window;
struct SDL_Renderer;

class Emulator
{
public:
    Emulator() = default;
    ~Emulator();

    bool init(int argc, char *argv[]);
    void run();

private:
    SDL_Window *m_window = nullptr;
    SDL_Renderer *m_renderer = nullptr;

    uint32_t m_window_width = 800;
    uint32_t m_window_height = 800;
    bool m_exit = false;

    void handle_input();
    void render();
};
