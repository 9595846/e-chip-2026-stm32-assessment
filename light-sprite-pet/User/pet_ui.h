#ifndef PET_UI_H
#define PET_UI_H

#include <stdint.h>

typedef enum {
    PET_FACE_NORMAL = 0,
    PET_FACE_HAPPY,
    PET_FACE_HUNGRY,
    PET_FACE_SLEEPY,
    PET_FACE_SLEEP,
    PET_FACE_SURPRISED,
    PET_FACE_ANGRY,
    PET_FACE_DIZZY,
    PET_FACE_LOOK_LEFT,
    PET_FACE_LOOK_RIGHT,
    PET_FACE_EATING,
    PET_FACE_NAILONG,
    PET_FACE_COUNT
} PetFace;

typedef enum {
    PET_INPUT_NONE = 0,
    PET_INPUT_FEED,
    PET_INPUT_PLAY,
    PET_INPUT_SLEEP_TOGGLE,
    PET_INPUT_STATUS_TOGGLE
} PetInput;

void pet_ui_init(uint32_t now);
void pet_ui_task(uint32_t now);
void pet_ui_set_face(unsigned face);
void pet_ui_next(void);
void pet_ui_previous(void);
void pet_ui_set_auto(uint8_t enabled, uint32_t now);
PetFace pet_ui_face(void);
uint8_t pet_ui_auto_enabled(void);
uint8_t pet_ui_oled_online(void);
PetInput pet_ui_take_input(void);
void pet_ui_set_pet_status(uint8_t visible, uint8_t fullness,
                           uint8_t happiness, uint8_t energy, uint8_t sleeping);
void pet_ui_set_diagnostics(uint8_t visible, uint16_t light, uint8_t mpu_online,
                            int16_t ax, int16_t ay, int16_t az,
                            int16_t gx, int16_t gy, int16_t gz);

#endif

