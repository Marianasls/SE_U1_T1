# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(SE_U1_T1 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(SE_U1_T1 
        SE_U1_T1.c 
        lib/ssd1306.c # Biblioteca para o display OLED
        lib/led_matrix.c # Biblioteca para matrix de LEDS
    )

pico_set_program_name(SE_U1_T1 "SE_U1_T1")
pico_set_program_version(SE_U1_T1 "0.1")

# Generate PIO header
file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib)
pico_generate_pio_header(SE_U1_T1 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/lib)


# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(SE_U1_T1 1)
pico_enable_stdio_usb(SE_U1_T1 1)

# Add the standard library to the build
target_link_libraries(SE_U1_T1
        pico_stdlib
        hardware_i2c
        hardware_adc
        hardware_pio
        )

# Add the standard include files to the build
target_include_directories(SE_U1_T1 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(SE_U1_T1 
        
        )

pico_add_extra_outputs(SE_U1_T1)

