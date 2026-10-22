#include "pet_game.h"
#include "pet_ui.h"
#include "servo.h"
#include "sensors.h"
#include "config.h"

#if APP_STAGE >= 5

static PetGameState g_pet;
static uint32_t g_last_tick;
static uint32_t g_face_until;
static uint8_t g_action_face;
static uint8_t g_tick_count;
#if APP_STAGE >= 6
static uint32_t g_last_diag_refresh;
#endif

static uint8_t add_limit(uint8_t value, unsigned amount)
{
    unsigned result = (unsigned)value + amount;
    return (uint8_t)(result > 100u ? 100u : result);
}

static uint8_t subtract_limit(uint8_t value, unsigned amount)
{
    return (uint8_t)(value > amount ? value - amount : 0u);
}

static void refresh_status(void)
{
#if APP_STAGE >= 6
    const SensorData *sensor = sensors_data();
    pet_ui_set_pet_status((uint8_t)(g_pet.view_page == 1u), g_pet.fullness,
                          g_pet.happiness, g_pet.energy, g_pet.sleeping);
    pet_ui_set_diagnostics((uint8_t)(g_pet.view_page == 2u),
        sensor->light_raw, sensor->mpu_online,
        sensor->ax, sensor->ay, sensor->az,
        sensor->gx, sensor->gy, sensor->gz);
#else
    pet_ui_set_pet_status(g_pet.status_visible, g_pet.fullness,
                          g_pet.happiness, g_pet.energy, g_pet.sleeping);
#endif
}

static void base_face(void)
{
    if (g_pet.sleeping) { pet_ui_set_face(PET_FACE_SLEEP); }
    else if (g_pet.fullness <= 20u) { pet_ui_set_face(PET_FACE_HUNGRY); }
    else if (g_pet.energy <= 20u) { pet_ui_set_face(PET_FACE_SLEEPY); }
    else if (g_pet.happiness <= 20u) { pet_ui_set_face(PET_FACE_ANGRY); }
    else { pet_ui_set_face(PET_FACE_NORMAL); }
}

static void show_action(PetFace face, uint32_t now, uint32_t duration)
{
    pet_ui_set_auto(0u, now);
    pet_ui_set_face(face);
    g_action_face = 1u;
    g_face_until = now + duration;
}

void pet_game_init(uint32_t now)
{
    g_pet.fullness = 75u;
    g_pet.happiness = 80u;
    g_pet.energy = 80u;
    g_pet.sleeping = 0u;
    g_pet.status_visible = 0u;
    g_pet.view_page = 0u;
    g_last_tick = now;
    g_face_until = now;
    g_action_face = 0u;
    g_tick_count = 0u;
#if APP_STAGE >= 6
    g_last_diag_refresh = now;
#endif
    refresh_status();
    base_face();
}

void pet_game_feed(uint32_t now)
{
    g_pet.sleeping = 0u;
    g_pet.fullness = add_limit(g_pet.fullness, 20u);
    g_pet.happiness = add_limit(g_pet.happiness, 3u);
    show_action(PET_FACE_EATING, now, 1400u);
    servo_wag(now);
    refresh_status();
}

void pet_game_play(uint32_t now)
{
    g_pet.sleeping = 0u;
    if (g_pet.energy < 10u) {
        show_action(PET_FACE_SLEEPY, now, 1200u);
    } else {
        g_pet.happiness = add_limit(g_pet.happiness, 18u);
        g_pet.energy = subtract_limit(g_pet.energy, 8u);
        g_pet.fullness = subtract_limit(g_pet.fullness, 3u);
        show_action(PET_FACE_HAPPY, now, 1400u);
        servo_wag(now);
    }
    refresh_status();
}

void pet_game_toggle_sleep(uint32_t now)
{
    g_pet.sleeping = (uint8_t)!g_pet.sleeping;
    g_action_face = 0u;
    pet_ui_set_auto(0u, now);
    base_face();
    servo_center();
    refresh_status();
}

void pet_game_toggle_status(void)
{
#if APP_STAGE >= 6
    g_pet.view_page = (uint8_t)((g_pet.view_page + 1u) % 3u);
    g_pet.status_visible = (uint8_t)(g_pet.view_page == 1u);
#else
    g_pet.status_visible = (uint8_t)!g_pet.status_visible;
#endif
    refresh_status();
}

void pet_game_reset(uint32_t now)
{
    pet_game_init(now);
}

void pet_game_restore_face(void)
{
    if (!g_action_face) { base_face(); }
}

void pet_game_task(uint32_t now)
{
    PetInput input = pet_ui_take_input();
    switch (input) {
    case PET_INPUT_FEED: pet_game_feed(now); break;
    case PET_INPUT_PLAY: pet_game_play(now); break;
    case PET_INPUT_SLEEP_TOGGLE: pet_game_toggle_sleep(now); break;
    case PET_INPUT_STATUS_TOGGLE: pet_game_toggle_status(); break;
    default: break;
    }

    if ((uint32_t)(now - g_last_tick) >= 10000u) {
        g_last_tick += 10000u;
        g_tick_count++;
        g_pet.fullness = subtract_limit(g_pet.fullness, 1u);
        if (g_pet.sleeping) {
            g_pet.energy = add_limit(g_pet.energy, 5u);
        } else {
            g_pet.energy = subtract_limit(g_pet.energy, 2u);
            if ((g_tick_count % 3u) == 0u) {
                g_pet.happiness = subtract_limit(g_pet.happiness, 1u);
            }
        }
        if (g_pet.fullness <= 20u) {
            g_pet.happiness = subtract_limit(g_pet.happiness, 1u);
        }
        if (!g_action_face) { base_face(); }
        refresh_status();
    }
    if (g_action_face && (int32_t)(now - g_face_until) >= 0) {
        g_action_face = 0u;
        base_face();
    }
#if APP_STAGE >= 6
    if (g_pet.view_page == 2u && (uint32_t)(now - g_last_diag_refresh) >= 160u) {
        g_last_diag_refresh = now;
        refresh_status();
    }
#endif
}

const PetGameState *pet_game_state(void) { return &g_pet; }
uint8_t pet_game_is_sleeping(void) { return g_pet.sleeping; }

#else
static PetGameState g_pet;
void pet_game_init(uint32_t now) { (void)now; }
void pet_game_task(uint32_t now) { (void)now; }
void pet_game_feed(uint32_t now) { (void)now; }
void pet_game_play(uint32_t now) { (void)now; }
void pet_game_toggle_sleep(uint32_t now) { (void)now; }
void pet_game_toggle_status(void) { }
void pet_game_reset(uint32_t now) { (void)now; }
void pet_game_restore_face(void) { }
const PetGameState *pet_game_state(void) { return &g_pet; }
uint8_t pet_game_is_sleeping(void) { return 0u; }
#endif

