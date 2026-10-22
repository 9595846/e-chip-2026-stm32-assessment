#ifndef PET_GAME_H
#define PET_GAME_H

#include <stdint.h>

typedef struct {
    uint8_t fullness;
    uint8_t happiness;
    uint8_t energy;
    uint8_t sleeping;
    uint8_t status_visible;
    uint8_t view_page;
} PetGameState;

void pet_game_init(uint32_t now);
void pet_game_task(uint32_t now);
void pet_game_feed(uint32_t now);
void pet_game_play(uint32_t now);
void pet_game_toggle_sleep(uint32_t now);
void pet_game_toggle_status(void);
void pet_game_reset(uint32_t now);
void pet_game_restore_face(void);
const PetGameState *pet_game_state(void);
uint8_t pet_game_is_sleeping(void);

#endif

