# adapted from https://github.com/pyusb/pyusb/blob/master/docs/tutorial.rst

import usb.core
import usb.util
import usb.control
from time import sleep


# dev.ctrl_transfer(bmRequestType, bRequest, wValue, wIndex, payload)
#  union {
#    struct TU_ATTR_PACKED {
#      uint8_t recipient :  5; ///< Recipient type tusb_request_recipient_t.
#      uint8_t type      :  2; ///< Request type tusb_request_type_t.
#      uint8_t direction :  1; ///< Direction type. tusb_dir_t
#    } bmRequestType_bit;
#    uint8_t bmRequestType;
#  };

# USB definitions
TUSB_REQ_RCPT_DEVICE = 0
TUSB_REQ_RCPT_INTERFACE = 1
TUSB_REQ_RCPT_ENDPOINT = 2
TUSB_REQ_RCPT_OTHER = 3

TUSB_REQ_TYPE_STANDARD = 0 << 5
TUSB_REQ_TYPE_CLASS = 1 << 5
TUSB_REQ_TYPE_VENDOR = 2 << 5
TUSB_REQ_TYPE_INVALID = 3 << 5

TUSB_DIR_OUT = 0 << 7
TUSB_DIR_IN  = 1 << 7

REQ_OUT = TUSB_DIR_OUT | TUSB_REQ_TYPE_VENDOR |TUSB_REQ_RCPT_DEVICE
REQ_IN = TUSB_DIR_IN | TUSB_REQ_TYPE_VENDOR |TUSB_REQ_RCPT_DEVICE

I2C_M_RD = 0x01

CMD_ECHO = 0
CMD_GET_FUNC = 1
CMD_SET_DELAY = 2
CMD_GET_STATUS = 3
CMD_I2C_IO = 4
CMD_I2C_BEGIN = 1  # flag fo I2C_IO
CMD_I2C_END = 2    # flag fo I2C_IO


# check i2c_usb device present
dev = usb.core.find(idVendor=0x0403, idProduct=0xc631)
if dev is None:
    raise ValueError('Device not found')

# configure device
dev.set_configuration()

# test echo command
req = dev.ctrl_transfer(REQ_IN, CMD_ECHO, 0x1234, 0, 2)
assert req == bytearray([0x34, 0x12])
print ('ECHO OK')
print ()

def i2c_write(addr, flags, data):
    return dev.ctrl_transfer(REQ_OUT, CMD_I2C_IO | flags, 0, addr, data)

def i2c_read(addr, flags, len):
    return dev.ctrl_transfer(REQ_IN, CMD_I2C_IO | flags, I2C_M_RD, addr, len)

EEPROM_ADDR = 0x50
PCF8583_ADDR = 0x51
MCP9808_ADDR = 0x18

def eeprom_test():
    print ('EEPROM TEST')

    # check if  EEPROM present
    try:
        i2c_write(EEPROM_ADDR, CMD_I2C_BEGIN|CMD_I2C_END, '')
    except:
        print ('EEPROM not found')
        return

    # read contents of 10 bytes starting from 0x20
    i2c_write(EEPROM_ADDR, CMD_I2C_BEGIN, bytearray([0x00, 0x20]))
    ret = i2c_read(EEPROM_ADDR, CMD_I2C_END, 10)
    print ('Original Content: ')
    print (''.join(['{:02X} '.format(x) for x in ret]))

    # change memory content
    data = [0x00, 0x20]
    data.extend([(x+1)%256 for x in ret])
    i2c_write(EEPROM_ADDR, CMD_I2C_BEGIN|CMD_I2C_END, bytearray(data))
    sleep(0.1)

    # read contents of 10 bytes starting from 0x20
    i2c_write(EEPROM_ADDR, CMD_I2C_BEGIN, bytearray([0x00, 0x20]))
    ret = i2c_read(EEPROM_ADDR, CMD_I2C_END, 10)
    print ('New Content: ')
    print (''.join(['{:02X} '.format(x) for x in ret]))

def pcf8583_test():
    print ('PCF8583 TEST')

    # check if  PCF8583 present
    try:
        i2c_write(PCF8583_ADDR, CMD_I2C_BEGIN|CMD_I2C_END, '')
    except:
        print ('PCF8583 not found')
        return

    # initialize RAM area to test
    data = [0x45]
    data.extend([x for x in range(1, 17)])
    i2c_write(PCF8583_ADDR, CMD_I2C_BEGIN|CMD_I2C_END, bytearray(data))

    # read contents of 16 bytes starting from 0x45
    i2c_write(PCF8583_ADDR, CMD_I2C_BEGIN, bytearray([0x45]))
    ret = i2c_read(PCF8583_ADDR, CMD_I2C_END, 16)
    print ('Original Content: ')
    print (''.join(['{:02X} '.format(x) for x in ret]))

    # change memory content
    data = [0x45]
    data.extend([(x+3)%256 for x in ret])
    i2c_write(PCF8583_ADDR, CMD_I2C_BEGIN|CMD_I2C_END, bytearray(data))

    # read contents of 16 bytes starting from 0x45
    i2c_write(PCF8583_ADDR, CMD_I2C_BEGIN, bytearray([0x45]))
    ret = i2c_read(PCF8583_ADDR, CMD_I2C_END, 16)
    print ('New Content: ')
    print (''.join(['{:02X} '.format(x) for x in ret]))


def mcp9808_test():
    print ('MCP9808 SENSOR TEST')

    # check if sensor present
    try:
        i2c_write(MCP9808_ADDR, CMD_I2C_BEGIN|CMD_I2C_END, '')
    except:
        print ('MCP9808 not found')
        return
    
    # change upper limit register to 20 C
    # 0000 0001 0100 0000
    i2c_write(MCP9808_ADDR, CMD_I2C_BEGIN|CMD_I2C_END, bytearray([ 0x02, 0x01, 0x40 ]))
    
    # read upper limit register
    i2c_write(MCP9808_ADDR, CMD_I2C_BEGIN, bytearray([0x02]))
    ret = i2c_read(MCP9808_ADDR, CMD_I2C_END, 2)
    temp = (((ret[0] << 8) + ret[1]) >> 2) / 4
    print ('Upper limit = {} C - {}'.format(temp, 'OK' if temp==20.0 else 'ERROR'))
    
    # change upper limit register to 33.5 C
    # 0000 0010 0001 1000
    i2c_write(MCP9808_ADDR, CMD_I2C_BEGIN|CMD_I2C_END, bytearray([ 0x02, 0x02, 0x18 ]))
    
    # read upper limit register
    i2c_write(MCP9808_ADDR, CMD_I2C_BEGIN, bytearray([0x02]))
    ret = i2c_read(MCP9808_ADDR, CMD_I2C_END, 2)
    temp = (((ret[0] << 8) + ret[1]) >> 2) / 4
    print ('Upper limit = {} C - {}'.format(temp, 'OK' if temp==33.5 else 'ERROR'))
    

eeprom_test()
print ()
mcp9808_test()
print ()
pcf8583_test()
print ()

