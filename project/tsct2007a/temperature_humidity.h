#include <unistd.h>

#define Temp_I2C_ADDR   0x44    //SH40-xD1B

void TH_i2c_write_byte(uint8_t dev_addr, uint8_t addr, uint8_t data);
void TH_i2c_read_byte(uint8_t dev_addr, uint8_t addr, uint8_t* dataBuffer);

void getTH();