#ifndef PET_MPU_SOFT_I2C_H
#define PET_MPU_SOFT_I2C_H

#include <stdint.h>

void mpu_bus_init(void);
int mpu_bus_probe(uint8_t address);
int mpu_bus_write(uint8_t address, const uint8_t *data, unsigned length);
int mpu_bus_read_register_byte(uint8_t address, uint8_t reg, uint8_t *value);
uint8_t mpu_bus_scl_level(void);
uint8_t mpu_bus_sda_level(void);

#endif

