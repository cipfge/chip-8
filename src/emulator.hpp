#pragma once

#include <cstdint>
#include <string>
#include <SDL.h>

class Emulator
{
public:
    Emulator() = default;
    ~Emulator();

    struct Registers
    {
        uint16_t PC = 0x0000;
        uint16_t SP = 0x0000;
        uint16_t I = 0x0000;
        uint8_t V[16] = { 0 };
    };

    struct Opcode
    {
        uint16_t type = 0x0000;
        uint16_t x = 0x0000;
        uint16_t y = 0x0000;
        uint16_t n = 0x0000;
        uint16_t kk = 0x0000;
        uint16_t nnn = 0x0000;
    };

    static inline constexpr uint32_t MemorySize = 4096;
    static inline constexpr uint32_t StackSize = 16;
    static inline constexpr uint32_t FontSize = 80;
    static inline constexpr uint32_t ResetVector = 0x200;
    static inline constexpr uint32_t DisplayWidth = 64;
    static inline constexpr uint32_t DisplayHeight = 32;
    static inline constexpr uint32_t KeyCount = 16;

    bool init();
    void run();

private:
    SDL_Window *m_window = nullptr;
    SDL_Renderer *m_renderer = nullptr;
    SDL_Texture *m_texture = nullptr;
    SDL_AudioSpec m_audio_spec {};
    SDL_AudioDeviceID m_audio_device = 0;

    int m_window_width = 500;
    int m_window_height = 250;
    bool m_should_exit = false;
    bool m_exit = false;
    bool m_show_about = false;
    bool m_paused = false;

    Registers m_registers;
    Opcode m_opcode;

    uint8_t m_memory[MemorySize] = { 0 };
    uint16_t m_stack[StackSize] = { 0 };
    uint8_t m_display[DisplayWidth * DisplayHeight] = { 0 };
    uint32_t m_color_buffer[DisplayWidth * DisplayHeight] = { 0 };
    bool m_display_updated = false;
    uint8_t m_delay_timer = 0;
    uint8_t m_sound_timer = 0;

    static uint8_t m_font[FontSize];
    static int m_keymap[KeyCount];

    // Audio sample index
    int m_audio_position = 0;

    void handle_input();
    void update_color_buffer();
    void render();
    void render_user_interface();
    void render_menubar();
    void render_exit_dialog();
    void render_about_dialog();

    void reset();
    uint8_t read(uint16_t address);
    uint16_t read_word(uint16_t address);
    void write(uint16_t address, uint8_t value);
    void stack_push(uint16_t value);
    uint16_t stack_pop();
    void fetch();
    void execute();
    void update_timers();

    uint8_t generate_random_byte();

    void draw_pixel();
    bool wait_key_press();

    double get_audio_sample();
    void write_audio_data(uint8_t* buffer, double data);

    void open_rom_file();

    void error(const std::string& message);
};
