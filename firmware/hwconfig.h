/**
 * @file hwconfig.h
 * @author Daniel Quadros
 * @date 2024-10-11
 * 
 * Hardware definitions for the I2C-Tiny-USB interface 
 * 
 */

// Pin that has a LED connected
#define LED_PIN 25

// Logic levels to turn LED on and off
#define LED_ON  1
#define LED_OFF 0


// I2C Interface
#define I2C_ID   i2c1

// I2C Pins
#define SDA_PIN 6
#define SCL_PIN 7

#define I2C_SPEED 100000

// UART for Debug
#define UART_ID uart0
#define TX_PIN  0
#define RX_PIN  1

