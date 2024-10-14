/*
 * Definitions for bit-banged I2C
 */

void bbi2c_init(uint16_t clock_period_us);
void bbi2c_set_clock(uint16_t clock_period_us);
void bbi2c_start(void);
void bbi2c_repstart(void);
void bbi2c_stop(void);
bool bbi2c_write(uint8_t b);
uint8_t bbi2c_read(bool last);
