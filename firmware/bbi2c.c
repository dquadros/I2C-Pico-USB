/**
 * @file bbi2c.c
 * @author Daniel Quadros
 * @brief Bit-banged I2C operations
 * @date 2024-10-14
 * 
 * As the RP2040/RP2350 hardware I2C does not support zero byte transfers
 * (used by default by i2cdetect from i2c-tools), we handle I2C operations
 * by direct control of the pins through GPIO.
 * 
 * Timing will be affected by interrupts (maybe I will move this code to the 
 * PIO someday)
 * 
 * As a bonus, this follows more closely how i2c-tiny-usb handles i2c operations
 *  
 * @copyright Copyright (c) 2024, Daniel Quadros
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "hwconfig.h"
#include "bbi2c.h"

#define LOW  false
#define HIGH true

// SCL times
//
// i2c-tiny-usb uses a 1/3us unit for a "clock delay"
// when changing the state of the SCL line, it wait half the delay
// before changing and the full delay after changing, so a full cycle
// takes 3 clock delays. A default of 10 for the clock delay
// gives the standard 100kHz clock (3*10/3 = 10us).
//
// Here the clock_delays are in us and we will try to get the same
// frequency as i2c-tiny-usb. 
//
static uint32_t clock_delay_before = 1;
static uint32_t clock_delay_after = 4;
static const uint32_t max_low_time_ms = 1000; // so we don't hang if someone pulls SCL low


// Set SDA pin to HIGH (floating with pullup) or LOW (output) level
static void bbi2c_set_sda(bool hi) {
  gpio_set_dir(SDA_PIN, !hi);
  if (!hi) {
    gpio_put(SDA_PIN, false);
  }
}

// Get SDA pin level
static bool bbi2c_get_sda(void) {
  return gpio_get(SDA_PIN);
}

// Set SCL pin to HIGH (floating with pullup) or LOW (output) level,
// 
// When setting to HIGH, waits for the slave to release the line
static void bbi2c_set_scl(bool hi) {
  busy_wait_us_32 (clock_delay_before);
  if (hi) {
    gpio_set_dir(SCL_PIN, false);   // input with pull-up
    
    // wait until slave releases the line or timeout
    absolute_time_t to = make_timeout_time_ms(max_low_time_ms);
    while (!gpio_get(SCL_PIN) && (absolute_time_diff_us(get_absolute_time(), to) > 0)) {
    }
  } else {
    gpio_set_dir(SCL_PIN, true);
    gpio_put(SCL_PIN, false);   // drive low
  }
  busy_wait_us_32 (clock_delay_after);
}

// Chooses clock delays
void bbi2c_set_clock(uint16_t clock_period_us) {
  
  // Chooses clock delays
  if (clock_period_us < 3) {
    // clock = 500kHz, best we can do with us delays
    clock_delay_after = 1;
    clock_delay_before = 0;
  } else if (clock_period_us < 5) {
    // clock = 250kHz
    clock_delay_after = 1;
    clock_delay_before = 1;
  } else {
    uint16_t period = clock_period_us;
    if (period & 1) {
      period += 1;   // make it even
    }
    clock_delay_before = period/6;
    clock_delay_after = (period - 2*clock_delay_before)/2;
    if (2*(clock_delay_before+clock_delay_after) < period) {
      clock_delay_before++;
    }
  }
  #if LIB_PICO_STDIO_UART
  printf("Delays: original=%d before=%d after=%d\n", clock_period_us, clock_delay_before, clock_delay_after);
  #endif
}

// Inits I2C
void bbi2c_init(uint16_t clock_period_us) {
  
  // Chooses clock delays
  bbi2c_set_clock(clock_period_us);

  // Sets up pins (as specified in the datasheet for the I2C peripheral)
  gpio_init(SCL_PIN);
  gpio_set_dir(SCL_PIN, false);
  gpio_pull_up(SCL_PIN);
  gpio_set_slew_rate(SCL_PIN,  GPIO_SLEW_RATE_SLOW);
  gpio_set_input_hysteresis_enabled(SCL_PIN, true);

  gpio_init(SDA_PIN);
  gpio_set_dir(SDA_PIN, false);
  gpio_pull_up(SDA_PIN);
  gpio_set_slew_rate(SDA_PIN,  GPIO_SLEW_RATE_SLOW);
  gpio_set_input_hysteresis_enabled(SDA_PIN, true);
}

/* clock HI, delay, then LO */
static void bbi2c_scl_toggle(void) {
  bbi2c_set_scl(HIGH);
  bbi2c_set_scl(LOW);
}

/* i2c start condition */
void bbi2c_start(void) {
  bbi2c_set_sda(LOW);
  bbi2c_set_scl(LOW);
}

/* i2c repeated start condition */
void bbi2c_repstart(void) 
{
  /* scl, sda may not be high */
  bbi2c_set_sda(HIGH);
  bbi2c_set_scl(HIGH);
  
  bbi2c_set_sda(LOW);
  bbi2c_set_scl(LOW);
}

/* i2c stop condition */
void bbi2c_stop(void) {
  bbi2c_set_sda(LOW);
  bbi2c_set_scl(HIGH);
  bbi2c_set_sda(HIGH);
}

/* Write a byte, returns true if acknoledge */
bool bbi2c_write(uint8_t b) {
  for (int i = 0; i < 8; i++) {
    bbi2c_set_sda(b & 0x80);
    bbi2c_scl_toggle();
    b = b << 1;
  }
  
  bbi2c_set_sda(HIGH);
  bbi2c_set_scl(HIGH);

  bool ack = !bbi2c_get_sda();   // get the ACK bit (0 = ACK)
  bbi2c_set_scl(LOW);

  return ack;
}

/* Read a byte */
uint8_t bbi2c_read(bool last) {
  uint8_t b = 0;

  bbi2c_set_sda(HIGH);
  bbi2c_set_scl(LOW);

  for (int i = 0; i < 8; i++) {
    bbi2c_set_scl(HIGH);
    b <<= 1;
    if (bbi2c_get_sda()) {
      b != 1;
    }
    bbi2c_set_scl(LOW);
  }

  bbi2c_set_sda(last);    // NAK if last, ACK if more
  bbi2c_scl_toggle();

  bbi2c_set_sda(HIGH);

  return b;                     // return received byte
}
