#include "pet_ui.h"
#include "board.h"
#include "config.h"
#include "oled.h"

typedef struct {
    uint8_t sample;
    uint8_t stable;
    uint8_t long_sent;
    uint32_t changed_at;
    uint32_t pressed_at;
} KeyState;

static KeyState g_keys[4];
static PetFace g_face;
static uint8_t g_auto_mode;
static uint32_t g_last_scan;
static uint32_t g_last_auto_change;
static int16_t g_encoder_accumulator;
static PetInput g_pending_input;

static void handle_short_press(unsigned key, uint32_t now)
{
#if APP_STAGE >= 5
    (void)now;
    g_pending_input = (PetInput)(PET_INPUT_FEED + key);
#else
    switch (key) {
    case 0u:
        pet_ui_set_auto(0u, now);
        pet_ui_next();
        break;
    case 1u:
        pet_ui_set_auto(0u, now);
        pet_ui_previous();
        break;
    case 2u:
        pet_ui_set_face(PET_FACE_NORMAL);
        pet_ui_set_auto(0u, now);
        break;
    default:
        pet_ui_set_auto((uint8_t)!g_auto_mode, now);
        break;
    }
#endif
}

static void handle_long_press(unsigned key, uint32_t now)
{
#if APP_STAGE >= 5
    (void)now;
    g_pending_input = (PetInput)(PET_INPUT_FEED + key);
#else
    pet_ui_set_auto(0u, now);
    switch (key) {
    case 0u: pet_ui_set_face(PET_FACE_HAPPY); break;
    case 1u: pet_ui_set_face(PET_FACE_SLEEP); break;
    case 2u: pet_ui_set_face(PET_FACE_ANGRY); break;
    default: pet_ui_set_face(PET_FACE_DIZZY); break;
    }
#endif
}

static void scan_keys(uint32_t now)
{
    uint8_t raw = board_keys_raw();
    unsigned i;

    for (i = 0u; i < 4u; i++) {
        uint8_t pressed = (uint8_t)((raw >> i) & 1u);
        KeyState *key = &g_keys[i];

        if (pressed != key->sample) {
            key->sample = pressed;
            key->changed_at = now;
        }
        if (pressed != key->stable && (uint32_t)(now - key->changed_at) >= KEY_DEBOUNCE_MS) {
            key->stable = pressed;
            if (pressed) {
                key->pressed_at = now;
                key->long_sent = 0u;
            } else if (!key->long_sent) {
                handle_short_press(i, now);
            }
        }
        if (key->stable && !key->long_sent && (uint32_t)(now - key->pressed_at) >= KEY_LONG_MS) {
            key->long_sent = 1u;
            handle_long_press(i, now);
        }
    }
}

static void scan_encoder(uint32_t now)
{
    int16_t delta = board_encoder_delta();
    (void)now;
    if (delta == 0) {
        return;
    }
    g_auto_mode = 0u;
    g_encoder_accumulator = (int16_t)(g_encoder_accumulator + delta * ENCODER_DIRECTION);
    while (g_encoder_accumulator >= ENCODER_COUNTS_PER_DETENT) {
        g_encoder_accumulator = (int16_t)(g_encoder_accumulator - ENCODER_COUNTS_PER_DETENT);
        pet_ui_next();
    }
    while (g_encoder_accumulator <= -ENCODER_COUNTS_PER_DETENT) {
        g_encoder_accumulator = (int16_t)(g_encoder_accumulator + ENCODER_COUNTS_PER_DETENT);
        pet_ui_previous();
    }
}

void pet_ui_init(uint32_t now)
{
    unsigned i;
    uint8_t raw = board_keys_raw();
    g_face = PET_FACE_NORMAL;
    g_auto_mode = 0u;
    g_last_scan = now;
    g_last_auto_change = now;
    g_encoder_accumulator = 0;
    g_pending_input = PET_INPUT_NONE;
    for (i = 0u; i < 4u; i++) {
        g_keys[i].sample = (uint8_t)((raw >> i) & 1u);
        g_keys[i].stable = g_keys[i].sample;
        g_keys[i].long_sent = 0u;
        g_keys[i].changed_at = now;
        g_keys[i].pressed_at = now;
    }
    oled_reset_state();
}

void pet_ui_task(uint32_t now)
{
    if ((uint32_t)(now - g_last_scan) >= 5u) {
        g_last_scan = now;
        scan_keys(now);
        scan_encoder(now);
    }
    if (g_auto_mode && (uint32_t)(now - g_last_auto_change) >= 3000u) {
        g_last_auto_change = now;
        pet_ui_next();
    }
    oled_task(g_face, g_auto_mode, now);
}

void pet_ui_set_face(unsigned face)
{
    if (face < (unsigned)PET_FACE_COUNT) {
        g_face = (PetFace)face;
    }
}

void pet_ui_next(void)
{
    g_face = (PetFace)(((unsigned)g_face + 1u) % (unsigned)PET_FACE_COUNT);
}

void pet_ui_previous(void)
{
    g_face = (PetFace)(((unsigned)g_face + (unsigned)PET_FACE_COUNT - 1u) % (unsigned)PET_FACE_COUNT);
}

void pet_ui_set_auto(uint8_t enabled, uint32_t now)
{
    g_auto_mode = enabled ? 1u : 0u;
    g_last_auto_change = now;
}

PetFace pet_ui_face(void)
{
    return g_face;
}

uint8_t pet_ui_auto_enabled(void)
{
    return g_auto_mode;
}

uint8_t pet_ui_oled_online(void)
{
    return oled_is_online();
}

PetInput pet_ui_take_input(void)
{
    PetInput result = g_pending_input;
    g_pending_input = PET_INPUT_NONE;
    return result;
}

void pet_ui_set_pet_status(uint8_t visible, uint8_t fullness,
                           uint8_t happiness, uint8_t energy, uint8_t sleeping)
{
    oled_set_pet_status(visible, fullness, happiness, energy, sleeping);
}

void pet_ui_set_diagnostics(uint8_t visible, uint16_t light, uint8_t mpu_online,
                            int16_t ax, int16_t ay, int16_t az,
                            int16_t gx, int16_t gy, int16_t gz)
{
    oled_set_diagnostics(visible, light, mpu_online, ax, ay, az, gx, gy, gz);
}

