find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

set(EMULATOR_SOURCE_FILES
    "imgui/imconfig.h"
    "imgui/imgui.cpp"
    "imgui/imgui_demo.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui.h"
    "imgui/imgui_impl_sdl2.cpp"
    "imgui/imgui_impl_sdl2.h"
    "imgui/imgui_impl_sdlrenderer2.cpp"
    "imgui/imgui_impl_sdlrenderer2.h"
    "imgui/imgui_internal.h"
    "imgui/imgui_memory_editor.h"
    "imgui/imgui_tables.cpp"
    "imgui/imgui_widgets.cpp"
    "imgui/imstb_rectpack.h"
    "imgui/imstb_textedit.h"
    "imgui/imstb_truetype.h"
    "emulator.hpp"
    "emulator.cpp"
    "logger.hpp"
    "logger.cpp"
    "utils.hpp"
    "platform.hpp"
    "platform_linux.cpp"
    "platform_windows.cpp"
    "main.cpp"
    )

set(CMAKE_INCLUDE_CURRENT_DIR ON)

configure_file(
    "version.hpp.in"
    ${CMAKE_BINARY_DIR}/src/version.hpp
    )

set(APPLICATION_TYPE "")
set(RESOURCE_FILES "")

if (WIN32)
    set(APPLICATION_TYPE "WIN32")
    list(APPEND RESOURCE_FILES "chip8.rc")
endif()

add_executable(chip8
    ${APPLICATION_TYPE}
    ${EMULATOR_SOURCE_FILES}
    ${RESOURCE_FILES}
    )

if (TARGET SDL2::SDL2main)
    target_link_libraries(chip8 PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(chip8 PRIVATE SDL2::SDL2)

target_include_directories(chip8
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    )

set_target_properties(chip8
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )

target_compile_definitions(chip8
    PRIVATE
        "$<$<CONFIG:Debug>:EMULATOR_DEBUG_ENABLED>"
)
