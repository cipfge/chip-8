#include "emulator.hpp"
#include <string>
#include <SDL.h>

Emulator::~Emulator()
{
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Emulator::init(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        std::string message = "SDL_Init error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr);
        return false;
    }

    m_window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_window_width, m_window_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!m_window)
    {
        std::string message = "SDL_CreateWindow error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr);
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer)
    {
        std::string message = "SDL_CreateRenderer error: " + std::string(SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr);
        return false;
    }

    return true;
}

void Emulator::run()
{
    while (!m_exit)
    {
        handle_input();
        render();
    }
}

void Emulator::handle_input()
{
    SDL_Event event {};

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            m_exit = true;
            break;

        default:
            break;
        }
    }
}

void Emulator::render()
{
    SDL_RenderClear(m_renderer);
    SDL_RenderPresent(m_renderer);
}
