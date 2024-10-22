# I2C-Pico-USB
An RP2040 based adaptation of the i2c-tiny-usb.

"Attach any I2C client chip (thermo sensors, AD converter, displays, relays driver, ...) to your PC via USB ... quick, easy and cheap! Drivers for Linux, Windows and MacOS available." (from the README.md file in i2c-tiny-usb). 

The I2C-Pico-USB project is inspired by [i2c-tiny-usb by Till Harbaum](https://github.com/harbaum/I2C-Tiny-USB) and [i2c-star by Daniel Thompson](https://github.com/daniel-thompson/i2c-star). The firmware should run on any RP2040 (or RP2350) board, like the Raspberry Pi Pico and Pico 2.

After starting this project I found a similar one from Nicolai Electronics: https://github.com/Nicolai-Electronics/rp2040-i2c-interface.

Following i2c-tiny-usb, all USB transfers are done via the control endpoint.

As the I2C peripheral in TP2040/RP2350 does not support zero-length transfers, I2C operations are done through bit-banging.

You can enable debug printing (through UART) in CMakeLists.txt 

```pico_enable_stdio_uart(i2cpicousb 0)```

but this will slow down the firmware.

## Tests Done (so far)

**THIS IS A WORK IN PROGRESS**

The hardware used for testing were:

* Boards:
	* Raspberry Pi Pico
	* Raspberry Pi Pico 2
	* XIAO RP2040
* I2C devices:
	* MCP9808 temperature sensor
	* AT24C32A EEPROM
	* PCF8583 clock/calendar with 240 byte RAM

### Linux

* ic2-tools
* ti2c.c: C program using the I2C dev interface (https://www.kernel.org/doc/Documentation/i2c/dev-interface)

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

You may need to fix the pins used for I2C in 'hwconfig.h'.

## Hardware Setup

At minimum, you will need a RP2040/RP2350 board with USB connector, the I2C devices will be connected to two pins of the board (SDA and SCL). This pins can be configures in the 'hwconfig.h', by default SDA is GPIO6 and SCL is GPIO7. As I2C is implemented by bit-banging, they can be any GPIO.

A hardware UART can be enabled for debuging. The default 'hwconfig.h' file specifies uart0 with TX at GPIO0 and RX at GPIO1.

### I2C Pullups

I2C requires pullup resistors at the SCL and SDA lines. The firmware activates the internal pullup resistors in the microcontroller. Most of the time things will just work, but it gets "interesting" when they don't.

As detailed in the application note at the link bellow, a too low pullup can impede a device to drive low a line and a too high pullup may not be enough to drive a line up. The internal pullups are in the 60k to 80k range, more to the "too high" range. Some boards may include pullup resistors, remember that all the pullup resistors will be in parallel so the resulting value can go to the "too low" range.

https://www.ti.com/lit/an/slva689/slva689.pdf

### Using 5V I2C Devices

The RP microcontrollers operate at 3.3V, and the internal pullups are connected to this level.

If you want to connect devices that operate at 5V, you should use a bidirectional level shifter. The following application note gives more details and show some ways to implement the level shifter: 

https://cdn-shop.adafruit.com/datasheets/an97055.pdf

But... you will find a lot of examples in the Internet where 3.3V and 5V devices are used together with no level shifter.  I2C devices do not output a high level, the high level comes from the pullup resistors. If you have pullups only to 3.3V, it may work, as long as the 5V devices accept 3,3V as a high-level. If you have pullups to 3.3V and 5V, the 3.3V pullup will bring the voltage down to something that **may** not fry the 3.3V devices. I still recommend using level shifters.

## Testing and Using the I2C-Pico-USB 

### Linux

Current versions of Linux include the necessary driver. For testing we are going to use i2c-tools. If it is not installed in your system, use the package manager (for example, "sudo apt-get install i2c-tools" for Debian based Linux).

You can check that adapter is working and recognized with the command

```sudo i2cdetect -l```

The output should include a line like this:

```i2c-n i2c-tiny-usb at ...```

Where 'n' is a number that identifies the I2CBUS. You will use this number in other i2c-tools commands. To check the address of connected devices, use

```sudo i2c-detect n```

You will get first a warning that probing for i2c devices can have bad effects on some devices.

The ```i2c-get``` and ```i2c-put``` commands can be used to read and write to and from I2C devices that use the concept of registers, where a read is preceded by a write of the register's address (form 0x00 to 0xFF).

The following examples are for accessing four bytes, starting at address 0x40, in PCF8583 clock/calendar's ARM with the default I2C address of 0x50:

```
sudo i2cset n 0x50 0x40 1 2 3 4 i 
sudo i2cget n 0x50 0x40 i 4
```

Details for the i2c-tools commands can be found at the man pages.

## Windows

You will need to install a driver (libusb) for the adapter. The easiest way is to use Zadig (https://zadig.akeo.ie/). Plug the adapter, run Zadig and select "libusb-win32".

There are many ways to use libusb under Windows. In my testing I used PyUSB (https://github.com/pyusb/pyusb) with Python 3. Notice that this is a low-level API that just gives access to the adapters endpoints.
