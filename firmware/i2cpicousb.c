/**
 * @file i2cpicousb.c
 * @author Daniel Quadros
 * @brief USB adapter for I2C devices
 * @version 0.1
 * @date 2024-10-15
 * 
 * An RP2040 based adaptation of the i2c-tiny-usb.
 * 
 * The I2C-Pico-USB project is inspired by
 * - i2c-tiny-usb by Till Harbaum (https://github.com/harbaum/I2C-Tiny-USB)
 * - i2c-star by Daniel Thompson(https://github.com/daniel-thompson/i2c-star)
 * 
 * TODO:
 * 
 * - Support CMD_SET_DELAY
 * - Support I2C_FUNC_10BIT_ADDR
 * 
 * @copyright Copyright (c) 2024, Daniel Quadros
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "bbi2c.h"
#include "i2cusb.h"
#include "hwconfig.h"

#if LIB_PICO_STDIO_UART
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

/* the currently support capability is quite limited */
const unsigned long func = I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_NOSTART;

/* I2C status */
#define STATUS_IDLE	   0
#define STATUS_ADDRESS_ACK 1
#define STATUS_ADDRESS_NACK 2

static const uint16_t DEFAULT_PERIOD_US = 10; // 100kHz

static uint8_t status = STATUS_IDLE;

static uint8_t reply_buf[64];

// Current command
static struct i2c_cmd cmd;

//--------------------------------------------------------------------+
// Local routines
//--------------------------------------------------------------------+

static bool usb_i2c_setup(uint8_t rhport, tusb_control_request_t const* request);
static bool usb_i2c_data(void);

//--------------------------------------------------------------------+
// Main Program
//--------------------------------------------------------------------+
int main(void)
{
  // Initialize stdio (if enabled in CMakeList.txt)
  #if LIB_PICO_STDIO_UART
  stdio_uart_init_full(UART_ID, 115200, TX_PIN, RX_PIN);
  #endif

  // Initialize the LED (if available)
  #ifdef LED_PIN
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_put(LED_PIN, LED_OFF);
  #endif

  // Initialize I2C interface
  bbi2c_init(DEFAULT_PERIOD_US);

  // Initialize the USB Stack
  dbg_printf("Starting USB\n");
  board_init();
  tusb_init();

  // Main loop
  while (1)
  {
    tud_task();
  }

  return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
  #ifdef LED_PIN
  gpio_put(LED_PIN, LED_ON);
  #endif
  dbg_printf("Device mounted\n");
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  #ifdef LED_PIN
  gpio_put(LED_PIN, LED_OFF);
  #endif
  dbg_printf("Device unmounted\n");
}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
//
// Data transfers must be prepared at the setup stage:
// https://github.com/hathach/tinyusb/discussions/2551
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {

  if (stage == CONTROL_STAGE_SETUP) {
    dbg_printf("\n");
  }
  dbg_printf("Control xfer: stage=%d req:%d type:%02X\n", stage, request->bRequest, request->bmRequestType);

  switch (stage) {
    case CONTROL_STAGE_SETUP:
      switch (request->bRequest) {

        case CMD_ECHO:
          /* Echo wValue */
          dbg_printf("Echo %04X\n", request->wValue);
          memcpy(reply_buf, &request->wValue, sizeof(request->wValue));
          return tud_control_xfer(rhport, request, reply_buf, sizeof(request->wValue));

        case CMD_GET_FUNC:
          /* Report our capabilities */
          dbg_printf("Get Func\n");
          memcpy(reply_buf, &func, sizeof(func));
          return tud_control_xfer(rhport, request, reply_buf, sizeof(func));

        case CMD_SET_DELAY: // TODO
          /* This was used in i2c-tiny-usb to choose the clock
           * frequency by specifying the shortest time between
           * clock edges.
           *
           * This implementation silently ignores delay requests. We
           * run the hardware as fast as we are permitted.
           */
          dbg_printf("Set Delay %d\n", request->wValue);
          return tud_control_status(rhport, request);

        case CMD_I2C_IO:
        case CMD_I2C_IO | CMD_I2C_BEGIN:
        case CMD_I2C_IO | CMD_I2C_END:
        case CMD_I2C_IO | CMD_I2C_BEGIN | CMD_I2C_END:
          return usb_i2c_setup(rhport, request);

        case CMD_GET_STATUS:
          dbg_printf("Get status\n");
          memcpy(reply_buf, &status, sizeof(status));
          return tud_control_xfer(rhport, request, reply_buf, sizeof(status));

      }
      return false; // unsuported request
    case CONTROL_STAGE_DATA:
      switch (request->bRequest) {
        case CMD_I2C_IO:
        case CMD_I2C_IO | CMD_I2C_BEGIN:
        case CMD_I2C_IO | CMD_I2C_END:
        case CMD_I2C_IO | CMD_I2C_BEGIN | CMD_I2C_END:
          return usb_i2c_data();
      }
      return true;
    default:
      return true;  // Nothing to do in ACK stage
  }

  return true;
}

/* Handles an I2C I/O request in the setup stage */
static bool usb_i2c_setup(uint8_t rhport, tusb_control_request_t const* req) {

  // Reinterpret the request as an I2C I/O request
	cmd.cmd = req->bRequest;
	cmd.addr = req->wIndex;
	cmd.flags = req->wValue;
	cmd.len = req->wLength;
  dbg_printf("I2CIO Cmd: %d, Addr: %04x, Flags: %04x, Len: %d\n", cmd.cmd, cmd.addr, cmd.flags, cmd.len);

  // Send (re)start
  if (cmd.cmd & CMD_I2C_BEGIN) {
    bbi2c_start();
  } else {
    bbi2c_restart();
  }

  // Send Address
  uint8_t addr = ( cmd.addr << 1 );
  if (cmd.flags & I2C_M_RD )
    addr |= 1;  
  if (bbi2c_write(addr)) {
    status = STATUS_ADDRESS_ACK;
    if ((cmd.cmd & CMD_I2C_END) && (cmd.len == 0)) {
      // pediu para enviar stop e não tem dados
      bbi2c_stop();  
    }
  } else {
    status = STATUS_ADDRESS_NACK;
    bbi2c_stop();
    dbg_printf("NAK on addr %02X\n", cmd.addr);
    return false;
  }

  // That is all if no data to transfer
  if (cmd.len == 0) {
    return tud_control_status(rhport, req);
  }

  // Transfer data
  if (cmd.flags & I2C_M_RD ) {
    // On reading we get the data from the I2C device and
    // pass it to the USB stack
    dbg_printf("Reading %d\n", cmd.len);
    uint16_t to_read = cmd.len;
    while (to_read) {
      uint16_t len = to_read;
      if (len > sizeof(reply_buf)) {
        len = sizeof(reply_buf);
      }
      for (int i = 0; i < len; i++) {
        to_read--;
        reply_buf[i] = bbi2c_read(to_read == 0);
      }
      if (!tud_control_xfer(rhport, req, reply_buf, len)) {
        bbi2c_stop();
        dbg_printf("Error in tud_control_xfer\n");
        return false;
      }
    }
    if (cmd.cmd & CMD_I2C_END) {
      bbi2c_stop();
    }
    return true;
  } else {
    // On writing, we request the data from the USB stack
    // actual writing will be done in the DATA stage
    uint16_t len = cmd.len;
    if (len > sizeof(reply_buf)) {
      len = sizeof(reply_buf);
    }
    return tud_control_xfer(rhport, req, reply_buf, len);
  }
}

/* Handles an I2C I/O request in the data stage */
static bool usb_i2c_data() {
  if ((cmd.flags & I2C_M_RD) == 0) {
    // We are only interest in writing requests
    // reply_buf has the data from the host
    uint16_t len = cmd.len;
    if (len > sizeof(reply_buf)) {
      len = sizeof(reply_buf);
    }
    dbg_printf("Writing %d\n", len);
    for (int i = 0; i < len; i++) {
      if (!bbi2c_write(reply_buf[i])) {
        bbi2c_stop();
        dbg_printf("Error in bbi2c_write\n");
        return false;
      }
    }
    if (cmd.cmd & CMD_I2C_END) {
      bbi2c_stop();
    }
  }
  return true;
}