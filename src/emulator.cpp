#include "emulator.hpp"
#include <string>
#include <cstring>
#include <SDL.h>

// Font data
uint8_t Emulator::m_font[Emulator::FontSize] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80
};

// Keys map
int Emulator::m_keymap[Emulator::KeyCount] = {
    SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
    SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
    SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V
};

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

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                m_window_width = event.window.data1;
                m_window_height = event.window.data2;
            }
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

void Emulator::reset()
{
    m_registers.PC = ResetVector;
    m_registers.SP = 0x00;
    m_registers.I = 0x00;
    m_delay_timer = 0;
    m_sound_timer = 0;

    m_opcode.type = 0;
    m_opcode.x = 0;
    m_opcode.y = 0;
    m_opcode.n = 0;
    m_opcode.kk = 0;
    m_opcode.nnn = 0;

    for (int index = 0; index < 16; index++)
        m_registers.V[index] = 0x00;

    std::memset(m_stack, 0x00, sizeof(m_stack));
    std::memset(m_display, 0x00, sizeof(m_display));
    m_display_updated = true;
}

uint8_t Emulator::read(uint16_t address)
{
    return m_memory[address];
}

uint16_t Emulator::read_word(uint16_t address)
{
    return (read(address) << 8 | read(address + 1));
}

void Emulator::write(uint16_t address, uint8_t value)
{
    m_memory[address] = value;
}

void Emulator::stack_push(uint16_t value)
{
    m_stack[m_registers.SP] = value;
    m_registers.SP++;
}

uint16_t Emulator::stack_pop()
{
    m_registers.SP--;
    return m_stack[m_registers.SP];
}

void Emulator::fetch()
{
    uint16_t value = read_word(m_registers.PC);

    // Decode instruction
    m_opcode.type = (value >> 12) & 0x000F;
    m_opcode.x = (value >> 8) & 0x000F;
    m_opcode.y = (value >> 4) & 0x000F;
    m_opcode.n = value & 0x000F;
    m_opcode.kk = value & 0x00FF;
    m_opcode.nnn = value & 0x0FFF;

    // Increment program counter
    m_registers.PC += 2;
}

void Emulator::execute()
{
    fetch();

    switch (m_opcode.type)
    {
    case 0x0:
        switch (m_opcode.nnn)
        {
        case 0x0E0:
            std::memset(m_display, 0x00, sizeof(m_display));
            m_display_updated = true;
            break;

        case 0x00EE:
            m_registers.PC = stack_pop();
            break;
        }
        break;

    case 0x1:
        m_registers.PC = m_opcode.nnn;
        break;

    case 0x2:
        stack_push(m_registers.PC);
        m_registers.PC = m_opcode.nnn;
        break;

    case 0x3:
        if (m_registers.V[m_opcode.x] == m_opcode.kk)
            m_registers.PC += 2;
        break;

    case 0x4:
        if (m_registers.V[m_opcode.x] != m_opcode.kk)
            m_registers.PC += 2;
        break;

    case 0x5:
        if (m_registers.V[m_opcode.x] == m_registers.V[m_opcode.y])
            m_registers.PC += 2;
        break;

    case 0x6:
        m_registers.V[m_opcode.x] = m_opcode.kk;
        break;

    case 0x7:
        m_registers.V[m_opcode.x] += m_opcode.kk;
        break;

    case 0x8:
        switch (m_opcode.n)
        {
        case 0x0:
            m_registers.V[m_opcode.x] = m_registers.V[m_opcode.y];
            break;

        case 0x1:
            m_registers.V[m_opcode.x] |= m_registers.V[m_opcode.y];
            break;

        case 0x2:
            m_registers.V[m_opcode.x] &= m_registers.V[m_opcode.y];
            break;

        case 0x3:
            m_registers.V[m_opcode.x] ^= m_registers.V[m_opcode.y];
            break;

        case 0x4:
            m_registers.V[0xF] = (((uint16_t)m_registers.V[m_opcode.x] + (uint16_t)m_registers.V[m_opcode.y]) > 0xFF) ? 1 : 0;
            m_registers.V[m_opcode.x] += m_registers.V[m_opcode.y];
            break;

        case 0x5:
            m_registers.V[0xF] = (m_registers.V[m_opcode.x] > m_registers.V[m_opcode.y]) ? 1 : 0;
            m_registers.V[m_opcode.x] -= m_registers.V[m_opcode.y];
            break;

        case 0x6:
            m_registers.V[0xF] = m_registers.V[m_opcode.x] & 1;
            m_registers.V[m_opcode.x] >>= 1;
            break;

        case 0x7:
            m_registers.V[0xF] = (m_registers.V[m_opcode.y] > m_registers.V[m_opcode.x]) ? 1 : 0;
            m_registers.V[m_opcode.x] = m_registers.V[m_opcode.y] - m_registers.V[m_opcode.x];
            break;

        case 0xE:
            m_registers.V[0xF] = (m_registers.V[m_opcode.y] >> 7) & 1;
            m_registers.V[m_opcode.x] = m_registers.V[m_opcode.y] << 1;
            break;
        }
        break;

    case 0x9:
        if (m_registers.V[m_opcode.x] != m_registers.V[m_opcode.y])
            m_registers.PC += 2;
        break;

    case 0xA:
        m_registers.I = m_opcode.nnn;
        break;

    case 0xB:
        m_registers.PC = m_opcode.nnn + m_registers.V[0];
        break;

    case 0xC:
        m_registers.V[m_opcode.x] = (std::rand() % 0xFF) & m_opcode.kk;
        break;

    case 0xD:
        draw_pixel();
        break;

    case 0xE:
        switch (m_opcode.kk)
        {
        case 0x9E:
            if (SDL_GetKeyboardState(nullptr)[m_keymap[m_registers.V[m_opcode.x] & 15]])
                m_registers.PC += 2;
            break;

        case 0xA1:
            if (!SDL_GetKeyboardState(nullptr)[m_keymap[m_registers.V[m_opcode.x] & 15]])
                m_registers.PC += 2;
            break;
        }
        break;

    case 0xF:
        switch (m_opcode.kk)
        {
        case 0x07:
            m_registers.V[m_opcode.x] = m_delay_timer;
            break;

        case 0x0A:
            if (!wait_key_press())
                m_registers.PC -= 2;
                return;
            break;

        case 0x15:
            m_delay_timer =m_registers.V[m_opcode.x];
            break;

        case 0x18:
            m_sound_timer = m_registers.V[m_opcode.x];
            break;

        case 0x1E:
            m_registers.V[0xF] = ((m_registers.I + m_registers.V[m_opcode.x]) > 0xFFF) ? 1 : 0;
            m_registers.I += m_registers.V[m_opcode.x];
            break;

        case 0x29:
            m_registers.I = m_registers.V[m_opcode.x] * 5;
            break;

        case 0x33:
            m_memory[m_registers.I & 0xFFF] = (m_registers.V[m_opcode.x] % 1000) / 100;
            m_memory[(m_registers.I + 1) & 0xFFF] =  (m_registers.V[m_opcode.x] % 10) / 10;
            m_memory[(m_registers.I + 2) & 0xFFF] = m_registers.V[m_opcode.x] % 10;
            break;

        case 0x55:
            for (int index = 0; index <= m_opcode.x; index++)
                m_memory[(m_registers.I++) & 0xFFF] = m_registers.V[index];
            break;

        case 0x65:
            for (int index = 0; index <= m_opcode.x; index++)
                m_registers.V[index] = m_memory[(m_registers.I++) & 0xFFF];
            break;
        }
        break;
    }
}

void Emulator::update_timers()
{
    if (m_delay_timer > 0)
        m_delay_timer--;

    if (m_sound_timer > 0)
        m_sound_timer--;
}

void Emulator::draw_pixel()
{
    auto x = m_registers.V[m_opcode.x];
    auto y = m_registers.V[m_opcode.y];
    auto height = m_opcode.n;

    m_registers.V[0xF] = 0;
    for (uint8_t row = 0; row < height; row++)
    {
        uint8_t data = m_memory[m_registers.I + row];
        for (uint8_t column = 0; column < 8; column++)
        {
            if ((data & 0x80) != 0)
            {
                if (m_display[(x + column + ((y + row) * 64))] == 1)
                {
                    m_registers.V[0xF] = 1;
                }
                m_display[x + column + ((y + row) * 64)] ^= 1;
            }

            data <<= 1;
        }
    }

    m_display_updated = true;
}

bool Emulator::wait_key_press()
{
    bool key_pressed = false;

    for (int index = 0; index < KeyCount; index++)
    {
        if (SDL_GetKeyboardState(nullptr)[m_keymap[index]])
        {
            m_registers.V[m_opcode.x] = index;
            key_pressed = true;
        }
    }

    return key_pressed;
}
