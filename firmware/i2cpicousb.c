/**
 * @file i2cpicousb.c
 * @author Daniel Quadros
 * @brief USB adapter for I2C devices
 * @version 0.1
 * @date 2024-10-14
 * 
 * An RP2040 based adaptation of the i2c-tiny-usb.
 * 
 * The I2C-Pico-USB project is inspired by
 * - i2c-tiny-usb by Till Harbaum (https://github.com/harbaum/I2C-Tiny-USB)
 * - i2c-star by Daniel Thompson(https://github.com/daniel-thompson/i2c-star)
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

/* the currently support capability is quite limited */
const unsigned long func = I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_NOSTART;

/* I2C status */
#define STATUS_IDLE	   0
#define STATUS_ADDRESS_ACK 1
#define STATUS_ADDRESS_NACK 2

static uint8_t status = STATUS_IDLE;

static uint8_t reply_buf[64];

// Current command
static struct i2c_cmd cmd;

//--------------------------------------------------------------------+
// Local routines
//--------------------------------------------------------------------+

static bool usb_i2c_io(tusb_control_request_t const* request);

//--------------------------------------------------------------------+
// Main Program
//--------------------------------------------------------------------+
int main(void)
{
  // Initialize stdio (if used)
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
  bbi2c_init(10);

  // Initialize the USB Stack
  #if LIB_PICO_STDIO_UART
  printf("Starting USB\n");
  #endif
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
  #if LIB_PICO_STDIO_UART
  printf("Device mounted\n");
  #endif
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  #ifdef LED_PIN
  gpio_put(LED_PIN, LED_OFF);
  #endif
  #if LIB_PICO_STDIO_UART
  printf("Device unmounted\n");
  #endif
}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {

  #if LIB_PICO_STDIO_UART
  printf("Control xfer: stage=%d req:%d\n", stage, request->bRequest);
  #endif

  switch (stage) {
    case CONTROL_STAGE_SETUP:
      switch (request->bRequest) {

        case CMD_ECHO:
          /* Echo wValue */
          #if LIB_PICO_STDIO_UART
          printf("Echo %04X\n", request->wValue);
          #endif
          memcpy(reply_buf, &request->wValue, sizeof(request->wValue));
          return tud_control_xfer(rhport, request, reply_buf, sizeof(request->wValue));

        case CMD_GET_FUNC:
          /* Report our capabilities */
          #if LIB_PICO_STDIO_UART
          printf("Get Func\n");
          #endif
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
          #if LIB_PICO_STDIO_UART
          printf("Set Delay %d\n", request->wValue);
          #endif
          return tud_control_status(rhport, request);

        case CMD_I2C_IO:
        case CMD_I2C_IO | CMD_I2C_BEGIN:
        case CMD_I2C_IO | CMD_I2C_END:
        case CMD_I2C_IO | CMD_I2C_BEGIN | CMD_I2C_END:
          if (usb_i2c_io(request)) {
            return tud_control_status(rhport, request);
          } else {
            return false;
          }

        case CMD_GET_STATUS:
          #if LIB_PICO_STDIO_UART
          printf("Get status\n");
          #endif
          memcpy(reply_buf, &status, sizeof(status));
          return tud_control_xfer(rhport, request, reply_buf, sizeof(status));

      }
      return false; // unsuported request
    case CONTROL_STAGE_DATA:
      #if LIB_PICO_STDIO_UART
      printf("DATA stage\n");
      #endif
      break;
    deafult:
      return false;  // Nothing to do in ACK stage
  }

  return true;
}

/* Handles an I2C I/O request */
static bool usb_i2c_io(tusb_control_request_t const* req) {

  // Reinterpret the request as an I2C I/O request
	cmd.cmd = req->bRequest;
	cmd.addr = req->wIndex;
	cmd.flags = req->wValue;
	cmd.len = req->wLength;
  #if LIB_PICO_STDIO_UART
  printf("I2CIO Cmd: %d, Addr: %04x, Flags: %04x, Len: %d\n", cmd.cmd, cmd.addr, cmd.flags, cmd.len);
  #endif

  bool nostop = (cmd.cmd & CMD_I2C_END) && (cmd.len != 0);

  // Send (re)start
  if (cmd.cmd & CMD_I2C_BEGIN) {
    bbi2c_start();
  } else {
    bbi2c_repstart();
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
    #if LIB_PICO_STDIO_UART
    printf("***ACK*** on addr %02X\n", cmd.addr);
    #endif
  } else {
    status = STATUS_ADDRESS_NACK;
    cmd.len = 0;  // abort
    bbi2c_stop();
    #if LIB_PICO_STDIO_UART
    printf("NAK on addr %02X\n", cmd.addr);
    #endif
  }

  return status == STATUS_ADDRESS_ACK;
}

/*
// Invoked when data is received
void tud_vendor_rx_cb(uint8_t itf) {
  #if LIB_PICO_STDIO_UART
  printf("Rx\n");
  #endif

  bool stop = cmd.cmd & CMD_I2C_END;

  if (status == STATUS_ADDRESS_ACK) {
    if (cmd.len > sizeof(reply_buf)) {
      cmd.len = sizeof(reply_buf);
    }
    if ((cmd.flags & I2C_M_RD)) {
      if (cmd.len) {
        #if LIB_PICO_STDIO_UART
        printf("Reading %d\n", cmd.len);
        #endif
        for (int i = 0; i < cmd.len; i++) {
          cmd.len--;
          reply_buf[i] = bbi2c_read(cmd.len == 0);
        }
        if ((cmd.cmd & CMD_I2C_END) && (cmd.len == 0)) {
          bbi2c_stop();
        }
        tud_vendor_n_write(0, reply_buf, cmd.len);
      }
    } else {
      #if LIB_PICO_STDIO_UART
      printf("Writing %d\n", cmd.len);
      #endif
      for (int i = 0; i < cmd.len; i++) {
        cmd.len--;
        reply_buf[i] = bbi2c_read(cmd.len == 0);
      }
      if ((cmd.cmd & CMD_I2C_END) && (cmd.len == 0)) {
        bbi2c_stop();
      }
    }
  }
}
*/

