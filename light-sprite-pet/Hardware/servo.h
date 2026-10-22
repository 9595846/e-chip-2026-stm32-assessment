#ifndef PET_SERVO_H
#define PET_SERVO_H

#include <stdint.h>

void servo_init(void);
void servo_set_angle(uint8_t angle);
void servo_enable(uint8_t enabled);
void servo_wag(uint32_t now);
void servo_sway(uint32_t now);
void servo_center(void);
void servo_task(uint32_t now);
uint8_t servo_angle(void);
uint8_t servo_is_enabled(void);

#endif

