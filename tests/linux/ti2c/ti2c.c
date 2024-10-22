/*
   Simple I2C access test for 24C32
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#define FALSE 0
#define TRUE 1

int adapter_nr;  // adpter number
int addr;  // device address

// Commands sent through ioctl
// see https://www.kernel.org/doc/Documentation/i2c/dev-interface
unsigned char buffer[12];

struct i2c_msg read_msgs[] = {
  { 0, 0, 2, buffer },          // write memory address
  { 0, I2C_M_RD, 10, buffer+2 } // read memory data
};
struct i2c_msg write_msgs[] = {
  { 0, 0, 12, buffer }          // write memory address and data
};

struct i2c_rdwr_ioctl_data read_eeprom = {
  read_msgs, 2
};
struct i2c_rdwr_ioctl_data write_eeprom = {
  write_msgs, 1
};


// parse parameters
int parse(int argc, char**argv) {
  if (argc < 3) {
    return FALSE;
  }
  adapter_nr = atoi(argv[1]);
  if ((memcmp(argv[2], "0x", 2) == 0) || (memcmp(argv[2],"0X", 2) == 0)) {
    addr = (int) strtol(argv[2]+2, NULL, 16);
  } else {
    addr = atoi(argv[2]);
  }
  return (adapter_nr != 0) && (addr != 0);
}

// Main program
int main (int argc, char **argv) {
  int file;
  char filename[20];

  // parse parameters
  if (!parse(argc, argv)) {
    printf ("Use: ti2c adapter_nr i2c_addr\n");
    return 1;
  }
  printf ("Adapter %d, device add = 0x%02X\n", adapter_nr, addr);

  // open adapter
  snprintf(filename, sizeof(filename)-1, "/dev/i2c-%d", adapter_nr);
  file = open(filename, O_RDWR);
  if (file < 0) {
    printf ("Error accessing adapter %d.\n", adapter_nr);
    return 2;
  }

  // read previous content (10 bytes startint at address 32)
  buffer[0] = 0; buffer[1] = 32;
  read_msgs[0].addr = read_msgs[1].addr = addr;
  ioctl (file, I2C_RDWR, &read_eeprom);
  printf("Old content:\n");
  for (int i = 0; i < 10; i++) {
    printf ("%02d ", buffer[i+2]);
  }
  printf ("\n");

  // write 10 bytes starting at address 32
  // this is a "page write"
  buffer[0] = 0; buffer[1] = 32;
  write_msgs[0].addr = write_msgs[1].addr = addr;
  unsigned char start = buffer[2]+1;
  for (int i = 0; i < 10; i++) {
    buffer[i+2] = start+i;
  }
  ioctl (file, I2C_RDWR, &write_eeprom);

  // wait for write to complete
  sleep(1);

  // read new content
  buffer[0] = 0; buffer[1] = 32;
  read_msgs[0].addr = read_msgs[1].addr = addr;
  memset (buffer+2, 0xFF, 10);
  ioctl (file, I2C_RDWR, &read_eeprom);
  printf("New content:\n");
  for (int i = 0; i < 10; i++) {
    printf ("%02d ", buffer[i+2]);
  }
  printf ("\n");

  // cleanup
  close(file);
  return 0;
}
