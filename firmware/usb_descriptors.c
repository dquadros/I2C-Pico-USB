/*
 * This file is based on a file originally part of the
 * MicroPython project, http://micropython.org/
 *
 * Adapted from the Raspberry Pi PIco C/C++ SDK
 * as part of an example of implementing a USB CDC device
 * 
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Daniel Quadros
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2019 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "tusb.h"
#include "pico/unique_id.h"

// From i2c-tiny-usb
#define USBD_VID (0x0403)
#define USBD_PID (0xc631)
#define USBD_DEVICE (0x0205)

//#define USBD_DESC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)
//#define USBD_MAX_POWER_MA (250)

#define USBD_STR_0 (0x00)
#define USBD_STR_MANUF (0x01)
#define USBD_STR_PRODUCT (0x02)
#define USBD_STR_SERIAL (0x03)

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
static const tusb_desc_device_t usbd_desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0110,
    .bDeviceClass = TUSB_CLASS_VENDOR_SPECIFIC,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USBD_VID,
    .idProduct = USBD_PID,
    .bcdDevice = USBD_DEVICE,
    .iManufacturer = USBD_STR_MANUF,
    .iProduct = USBD_STR_PRODUCT,
    .iSerialNumber = USBD_STR_SERIAL,
    .bNumConfigurations = 1,
};

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + sizeof(tusb_desc_interface_t))

#define TUD_CTRLITF_DESCRIPTOR(_itfnum, _itfclass, _itfsclass, _itfprotocol, _stridx) \
    sizeof(tusb_desc_interface_t), TUSB_DESC_INTERFACE, _itfnum, 0, 0, _itfclass, _itfsclass, _itfprotocol, _stridx

static uint8_t const desc_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, 0, 100),
  TUD_CTRLITF_DESCRIPTOR(0, 0xFF, 0, 0, USBD_STR_0)
};

static char usbd_serial_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

static const char *const usbd_desc_str[] = {
    [USBD_STR_MANUF] = "dqsoft.com.br",
    [USBD_STR_PRODUCT] = "i2c-pico-usb",
    [USBD_STR_SERIAL] = usbd_serial_str,
};


// Invoked when received GET DEVICE DESCRIPTOR
const uint8_t *tud_descriptor_device_cb(void) {
    return (const uint8_t *)&usbd_desc_device;
}

// Invoked when received GET CONFIGURATION DESCRIPTOR
const uint8_t *tud_descriptor_configuration_cb(__unused uint8_t index) {
    return desc_configuration;
}

// Invoked when received GET STRING DESCRIPTOR request
const uint16_t *tud_descriptor_string_cb(uint8_t index, __unused uint16_t langid) {
    #define DESC_STR_MAX (20)
    static uint16_t desc_str[DESC_STR_MAX];

    // Assign the SN using the unique flash id
    if (!usbd_serial_str[0]) {
        pico_get_unique_board_id_string(usbd_serial_str, sizeof(usbd_serial_str));
    }

    uint8_t len;
    if (index == 0) {
        desc_str[1] = 0x0409; // supported language is English
        len = 1;
    } else {
        if (index >= sizeof(usbd_desc_str) / sizeof(usbd_desc_str[0])) {
            return NULL;
        }
        const char *str = usbd_desc_str[index];
        for (len = 0; len < DESC_STR_MAX - 1 && str[len]; ++len) {
            desc_str[1 + len] = str[len];
        }
    }

    // first byte is length (including header), second byte is string type
    desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * len + 2));

    return desc_str;
}

