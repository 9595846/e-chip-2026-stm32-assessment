#ifndef PET_OLED_H
#define PET_OLED_H

#include <stdint.h>
#include "pet_ui.h"

void oled_reset_state(void);
void oled_task(PetFace face, uint8_t auto_mode, uint32_t now);
uint8_t oled_is_online(void);
void oled_set_pet_status(uint8_t visible, uint8_t fullness,
                         uint8_t happiness, uint8_t energy, uint8_t sleeping);
void oled_set_diagnostics(uint8_t visible, uint16_t light, uint8_t mpu_online,
                          int16_t ax, int16_t ay, int16_t az,
                          int16_t gx, int16_t gy, int16_t gz);

#endif

