find_package(SDL2 REQUIRED)

set(EMULATOR_SOURCE_FILES
    "emulator.hpp"
    "emulator.cpp"
    "platform.hpp"
    "utils.hpp"
    "main.cpp"
    )

set(APPLICATION_TYPE "")
if (WIN32)
    set(APPLICATION_TYPE "WIN32")
endif()

add_executable(chip8 ${APPLICATION_TYPE} ${EMULATOR_SOURCE_FILES})

target_link_libraries(chip8
    SDL2::SDL2
    )

target_include_directories(chip8
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
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
