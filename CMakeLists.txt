# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/dnl/pico/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(AtomClock C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(/home/dnl/pico/pico-arduino-compat/libs/adafruit-ssd1306 build_pac-adafruit-ssd1306)

# Add executable. Default name is the project name, version 0.1

add_executable(AtomClock 
        AtomClock.cpp 
)

pico_set_program_name(AtomClock "AtomClock")
pico_set_program_version(AtomClock "0.1")

pico_enable_stdio_uart(AtomClock 1)
pico_enable_stdio_usb(AtomClock 0)

# Add the standard library to the build
target_link_libraries(AtomClock pico_stdlib
    pac-adafruit-ssd1306
    hardware_spi
    hardware_gpio
)

pico_add_extra_outputs(AtomClock)

