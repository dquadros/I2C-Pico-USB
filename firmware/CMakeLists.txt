# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.14)

set(PICO_BOARD pico2 CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(i2cpicousb)

set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_executable(i2cpicousb
    i2cpicousb.c
    bbi2c.c
    usb_descriptors.c
)

target_include_directories(i2cpicousb PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(i2cpicousb PRIVATE
    pico_stdlib
    pico_unique_id
    hardware_gpio
    tinyusb_device
    tinyusb_board
)

pico_enable_stdio_uart(i2cpicousb 1)

pico_add_extra_outputs(i2cpicousb)

