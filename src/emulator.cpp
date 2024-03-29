#include "emulator.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_memory_editor.h"
#include "logger.hpp"
#include "utils.hpp"
#include "platform.hpp"
#include "version.hpp"
#include <cstring>
#include <fstream>
#include <random>
#include <thread>

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
    delete m_memory_window;

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_CloseAudioDevice(m_audio_device);
    SDL_DestroyTexture(m_texture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Emulator::init()
{
#ifdef __linux__
    SDL_setenv("SDL_VIDEO_WAYLAND_WMCLASS", "chip8-emulator", 0);
    SDL_setenv("SDL_VIDEO_X11_WMCLASS",     "chip8-emulator", 0);
#endif // Linux

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        logger::error("SDL_Init error: %s", SDL_GetError());
        return false;
    }

    m_window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_window_width, m_window_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!m_window)
    {
        logger::error("SDL_CreateWindow error: %s", SDL_GetError());
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer)
    {
        logger::error("SDL_CreateRenderer error: %s", SDL_GetError());
        return false;
    }

    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DisplayWidth, DisplayHeight);
    if (!m_texture)
    {
        logger::error("SDL_CreateTexture error: %s", SDL_GetError());
        return false;
    }

    // Init audio
    SDL_AudioSpec audio_spec;
    audio_spec.freq     = 44100;
    audio_spec.format   = AUDIO_S16;
    audio_spec.channels = 1;
    audio_spec.samples  = audio_spec.freq / 20;
    audio_spec.userdata = this;

    audio_spec.callback = [](void *user_data, unsigned char *stream, int size)
    {
        Emulator *emu = static_cast<Emulator*>(user_data);
        for (int sample = 0; sample < emu->m_audio_spec.samples; sample++)
        {
            double data = emu->get_audio_sample();

            for (int channel = 0; channel < emu->m_audio_spec.channels; channel++)
            {
                int offset = (sample * sizeof(int16_t) * emu->m_audio_spec.channels) + (channel * sizeof(int16_t));
                uint8_t* buffer = stream + offset;
                emu->write_audio_data(buffer, data);
            }
        }
    };

    m_audio_device = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, &m_audio_spec, 0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& imgui_io = ImGui::GetIO();
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    imgui_io.IniFilename = nullptr;

    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer2_Init(m_renderer);

    m_window_height = m_window_height + (int)ImGui::GetFrameHeight();
    SDL_SetWindowSize(m_window, m_window_width, m_window_height);

    m_memory_window = new MemoryEditor();
    m_memory_window->Open = false;

    return true;
}

void Emulator::run(int argc, char* argv[])
{
    constexpr auto TIMER = 60; // hz
    constexpr auto DELAY = 1000.0f / TIMER;

    uint32_t frame_start = SDL_GetTicks();
    uint32_t frame_time = 0;

    if (argc > 1)
        load_rom_from_file(argv[1]);

    while (!m_exit)
    {
        handle_input();
        if (m_rom_loaded && !m_paused)
        {
            execute_next_instruction();

            if (m_display_updated)
                update_color_buffer();

            frame_time = SDL_GetTicks() - frame_start;
            if (frame_time >= DELAY)
            {
                update_timers();
                frame_start = SDL_GetTicks();
            }
        }

        render();

        // TODO: Tick CPU for about a frame or until waiting for key press
        std::this_thread::sleep_for(std::chrono::microseconds(150));
    }
}

void Emulator::handle_input()
{
    SDL_Event event {};

    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type)
        {
        case SDL_QUIT:
            m_should_exit = true;
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_o &&
                event.key.keysym.mod & KMOD_CTRL &&
                event.key.repeat == 0)
            {
                open_rom_file();
            }

            if (event.key.keysym.sym == SDLK_r &&
                event.key.keysym.mod & KMOD_CTRL &&
                event.key.repeat == 0)
            {
                reset();
            }

            if (event.key.keysym.sym == SDLK_s &&
                event.key.keysym.mod & KMOD_CTRL &&
                event.key.repeat == 0)
            {
                stop();
            }

            if (event.key.keysym.sym == SDLK_p &&
                event.key.keysym.mod & KMOD_CTRL &&
                event.key.repeat == 0)
            {
                toggle_pause();
            }
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

void Emulator::update_color_buffer()
{
    for (int index = 0; index < (DisplayWidth * DisplayHeight); index++)
    {
        uint8_t pixel = m_display[index];
        m_color_buffer[index] = (0x00FFFF00 * pixel) | 0xFF000000;
    }

    m_display_updated = false;
}

void Emulator::render()
{
    SDL_RenderClear(m_renderer);
    SDL_UpdateTexture(m_texture, nullptr, m_color_buffer, DisplayWidth * sizeof(uint32_t));
    SDL_Rect screen_rect = { 0, (int)ImGui::GetFrameHeight(), m_window_width, m_window_height - (int)ImGui::GetFrameHeight() };
    SDL_RenderCopy(m_renderer, m_texture, nullptr, &screen_rect);

    render_user_interface();

    SDL_RenderPresent(m_renderer);
}

void Emulator::render_user_interface()
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGui::NewFrame();

    render_menubar();
    if (m_should_exit)
        ImGui::OpenPopup("Exit");
    render_exit_dialog();

    if (m_show_about)
        ImGui::OpenPopup("About");
    render_about_dialog();

    if (m_show_cpu_window)
        render_cpu_window();

    if (m_memory_window->Open)
        render_memory_window();

    ImGui::EndFrame();
    ImGui::Render();

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}

void Emulator::render_menubar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open ROM...", "Ctr+O"))
                open_rom_file();

            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4"))
                m_should_exit = true;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Emulation"))
        {
            std::string pause_str = "Pause";
            if (m_paused)
                pause_str = "Resume";

            if (ImGui::MenuItem(pause_str.c_str(), "Ctr+P"))
                toggle_pause();

            if (ImGui::MenuItem("Reset", "Ctr+R"))
                reset();

            ImGui::Separator();
            if (ImGui::MenuItem("Stop", "Ctr+S"))
                stop();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("CPU Window", NULL, &m_show_cpu_window);
            ImGui::MenuItem("Memory Window", NULL, &m_memory_window->Open);
            ImGui::Separator();

            if (ImGui::BeginMenu("Theme"))
            {
                if (ImGui::MenuItem(("Custom theme")))
                    set_custom_dark_theme();

                if (ImGui::MenuItem("Classic"))
                    ImGui::StyleColorsClassic();

                if (ImGui::MenuItem("Dark theme"))
                    ImGui::StyleColorsDark();

                if (ImGui::MenuItem("Light theme"))
                    ImGui::StyleColorsLight();
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About CHIP-8..."))
                m_show_about = true;

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Emulator::render_exit_dialog()
{
    if (ImGui::BeginPopupModal("Exit", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        m_should_exit = false;
        m_paused = true;

        ImGui::Text("Are you sure you want to exit?");
        ImGui::Separator();

        if (ImGui::Button("Yes", ImVec2(120, 0)))
        {
            m_exit = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();

        if (ImGui::Button("No", ImVec2(120, 0)))
        {
            m_paused = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Emulator::render_about_dialog()
{
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        m_show_about = false;
        m_paused = true;

        ImGui::Text(CHIP8_VERSION_NAME);
        ImGui::Text("Version: %s", CHIP8_VERSION_NUMBER);
        ImGui::Separator();

        ImGui::SetItemDefaultFocus();
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            m_paused = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Emulator::render_cpu_window()
{
    ImGui::Begin("CPU", &m_show_cpu_window);

    ImGui::Text("   PC: 0x%04X", m_registers.PC);
    ImGui::Text("   SP: 0x%04X", m_registers.SP);
    ImGui::Text("    I: 0x%04X", m_registers.I);

    ImGui::Text(" V[0]: 0x%02X", m_registers.V[0]); ImGui::SameLine();
    ImGui::Text(" V[1]: 0x%02X", m_registers.V[1]); ImGui::SameLine();
    ImGui::Text(" V[2]: 0x%02X", m_registers.V[2]); ImGui::SameLine();
    ImGui::Text(" V[3]: 0x%02X", m_registers.V[3]);
    ImGui::Text(" V[4]: 0x%02X", m_registers.V[4]); ImGui::SameLine();
    ImGui::Text(" V[5]: 0x%02X", m_registers.V[5]); ImGui::SameLine();
    ImGui::Text(" V[6]: 0x%02X", m_registers.V[6]); ImGui::SameLine();
    ImGui::Text(" V[7]: 0x%02X", m_registers.V[7]);
    ImGui::Text(" V[8]: 0x%02X", m_registers.V[8]); ImGui::SameLine();
    ImGui::Text(" V[9]: 0x%02X", m_registers.V[9]); ImGui::SameLine();
    ImGui::Text("V[10]: 0x%02X", m_registers.V[10]); ImGui::SameLine();
    ImGui::Text("V[11]: 0x%02X", m_registers.V[11]);
    ImGui::Text("V[12]: 0x%02X", m_registers.V[12]); ImGui::SameLine();
    ImGui::Text("V[13]: 0x%02X", m_registers.V[13]); ImGui::SameLine();
    ImGui::Text("V[14]: 0x%02X", m_registers.V[14]); ImGui::SameLine();
    ImGui::Text("V[15]: 0x%02X", m_registers.V[15]);

    ImGui::End();
}

void Emulator::render_memory_window()
{
    m_memory_window->DrawWindow("Memory", m_memory, sizeof(m_memory), ResetVector);
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

    if (m_paused)
        m_paused = false;
}

void Emulator::stop()
{
    std::memset(m_memory, 0x00, sizeof(m_memory));
    m_rom_loaded = false;
    reset();
}

void Emulator::toggle_pause()
{
    m_paused = !m_paused;
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

void Emulator::execute_next_instruction()
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
        m_registers.V[m_opcode.x] = generate_random_byte() & m_opcode.kk;
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
            break;

        case 0x15:
            m_delay_timer = m_registers.V[m_opcode.x];
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
            m_memory[(m_registers.I + 1) & 0xFFF] =  (m_registers.V[m_opcode.x] / 10) % 10;
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
    {
        SDL_PauseAudioDevice(m_audio_device, 0);
        m_sound_timer--;
    }
    else
    {
        SDL_PauseAudioDevice(m_audio_device, 1);
    }
}

uint8_t Emulator::generate_random_byte()
{
    std::random_device rand_dev;
    std::mt19937 rng(rand_dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,255);
    return dist(rng);
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

double Emulator::get_audio_sample()
{
    double sample_rate = (double)(m_audio_spec.freq);
    double period = sample_rate / 800;

    m_audio_position++;
    if (m_audio_position % (int)period == 0)
        m_audio_position = 0;

    double angular_freq = (1.0 / period) * 2.0 * M_PI;
    return sin(m_audio_position * angular_freq);
}

void Emulator::write_audio_data(uint8_t* buffer, double data)
{
    int16_t* b = (int16_t*)buffer;
    double range = (double)INT16_MAX - (double)INT16_MIN;
    double normalized = data * range / 2.0;
    *b = normalized;
}

void Emulator::open_rom_file()
{
    std::string rom_path = platform::open_file_dialog(m_window);
    if (rom_path.empty())
        return;

#ifdef __linux__
    // Remove '\n' from rom path, fixme
    rom_path.pop_back();
#endif // Linux

    load_rom_from_file(rom_path);
}

void Emulator::load_rom_from_file(const std::string& rom_path)
{
    std::ifstream rom_file(rom_path, std::ifstream::binary);
    if (!rom_file.is_open())
    {
       logger::error("Cannot open ROM file %s", rom_path.c_str());
       return;
    }

    uint32_t buffer_size = utils::get_file_size(rom_path);
    if ((MemorySize - ResetVector) < buffer_size)
    {
        logger::error("Invalid ROM size");
        return;
    }

    char* buffer = static_cast<char*>(malloc(sizeof(char) * buffer_size));
    if (!rom_file.read(buffer, buffer_size))
    {
        logger::error("Cannot read ROM from file %s", rom_path.c_str());
        return;
    }

    for (int index = 0; index < buffer_size; index++)
        m_memory[index + ResetVector] = (uint8_t)buffer[index];

    m_rom_loaded = true;
    reset();
}

void Emulator::set_custom_dark_theme()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    style.Colors[ImGuiCol_Tab]                   = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_NavHighlight]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

    style.WindowPadding     = ImVec2(8.00f, 8.00f);
    style.FramePadding      = ImVec2(5.00f, 2.00f);
    style.CellPadding       = ImVec2(6.00f, 6.00f);
    style.ItemSpacing       = ImVec2(6.00f, 6.00f);
    style.ItemInnerSpacing  = ImVec2(6.00f, 6.00f);
    style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
    style.IndentSpacing     = 25;
    style.ScrollbarSize     = 15;
    style.GrabMinSize       = 10;
    style.WindowBorderSize  = 1;
    style.ChildBorderSize   = 1;
    style.PopupBorderSize   = 1;
    style.FrameBorderSize   = 1;
    style.TabBorderSize     = 1;
    style.WindowRounding    = 7;
    style.ChildRounding     = 4;
    style.FrameRounding     = 3;
    style.PopupRounding     = 4;
    style.ScrollbarRounding = 9;
    style.GrabRounding      = 3;
    style.LogSliderDeadzone = 4;
    style.TabRounding       = 4;
}
