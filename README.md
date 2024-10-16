# I2C-Pico-USB
An RP2040 based adaptation of the i2c-tiny-usb.

**THIS IS A WORK IN PROGRESS**

*Very little testing done so far, using i2c_tools under Linux with a MCP9808 sensor.*

"Attach any I2C client chip (thermo sensors, AD converter, displays, relays driver, ...) to your PC via USB ... quick, easy and cheap! Drivers for Linux, Windows and MacOS available." (from the READ.ME file in i2c-tiny-usb). 

The I2C-Pico-USB project is inspired by [i2c-tiny-usb by Till Harbaum](https://github.com/harbaum/I2C-Tiny-USB) and [i2c-star by Daniel Thompson](https://github.com/daniel-thompson/i2c-star). The firmware should run on any RP2040 (or RP2350) board, like the Raspberry Pi Pico and Pico 2.

After starting this project I found a similar one from Nicolai Electronics: https://github.com/Nicolai-Electronics/rp2040-i2c-interface.

Following i2c-tiny-usb, all USB transfers are done via the control endpoint.

As the I2C peripheral in TP2040/RP2350 does not support zero-length transfers, I2C operations are done through bit-banging.

You can enable debug printing (through UART) in CMakeLists.txt 

```pico_enable_stdio_uart(i2cpicousb 0)```

but this will slow down the firmware.

## Compiling the Firmware

### Raspberry Pi Pico

The firmware was developed with the official Raspberry Pi Pico SDK v2.0.0. The Raspberry Pi Pico Getting Started document describes how to setup the environment. Once you got everything installed:

* clone this repository (```git clone https://github.com/dquadros/I2C-Pico-USB.git```)
* create a ```build``` subdiretory inside the ```I2C-Pico-USB``` directory
* in the ```build``` directory execute ```cmake ..``` (```cmake ..  -G "NMake Makefiles``` under windows)
* execute ```make``` (```nmake``` under Windows)

A I2C-Pico-USB.uf2 file will be created, this file should be loaded into the RP2040 board

### Raspberry Pi Pico 2

Add the ```-DPICO_BOARD=pico2``` option to the cmake command in the instructions above.

### Other Boards

If the board is directly supported by the Raspberry Pi Pico SDK, just use the  ```-DPICO_BOARD={board}``` option in the cmake command. Otherwise, use pico for RP2040 based boards and pico2 for RP2350 boards.

You may need to fix the pins used for I2C in hardware.h.

## Hardware Setup

TBA

## Testing and Using

TBA

