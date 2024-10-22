/**
 * @file hwconfig.h
 * @author Daniel Quadros
 * @date 2024-10-15
 * 
 * Hardware definitions for the I2C-Tiny-USB interface 
 * 
 */

// Pin that has a LED connected
#define LED_PIN 25

// Logic levels to turn LED on and off
#ifdef SEEED_XIAO_RP2040
#define LED_ON  0
#define LED_OFF 1
#else
#define LED_ON  1
#define LED_OFF 0
#endif

// I2C Pins
#define SDA_PIN 6
#define SCL_PIN 7

// UART for Debug
#define UART_ID uart0
#define TX_PIN  0
#define RX_PIN  1

