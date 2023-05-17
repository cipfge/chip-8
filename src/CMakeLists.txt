set(EMULATOR_SOURCE_FILES
    "main.cpp"
    )

add_executable(chip8 ${EMULATOR_SOURCE_FILES})

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
