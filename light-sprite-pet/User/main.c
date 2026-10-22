#include "board.h"
#include "config.h"
#if APP_STAGE >= 2
#include "pet_ui.h"
#endif
#if APP_STAGE >= 3
#include "sensors.h"
#endif
#if APP_STAGE >= 6
#include "mpu_soft_i2c.h"
#endif
#if APP_STAGE >= 4
#include "servo.h"
#endif
#if APP_STAGE >= 5
#include "pet_game.h"
#endif
#include <stdio.h>
#include <string.h>

#define LINE_SIZE 64u

static uint8_t g_led_auto = 1u;
static uint8_t g_buzzer_test;
static uint32_t g_buzzer_test_start;
#if APP_STAGE >= 3
static uint8_t g_sensor_auto = 1u;
static uint8_t g_sensor_reaction_active;
static PetFace g_sensor_reaction_face;
static uint32_t g_sensor_reaction_until;
#endif
#if APP_STAGE >= 4
static uint8_t g_tail_auto = 1u;
#endif

static void start_buzzer_test(uint32_t now)
{
    g_buzzer_test = 1u;
    g_buzzer_test_start = now;
}

static void update_buzzer_test(uint32_t now)
{
    uint32_t elapsed;

    if (!g_buzzer_test) {
        return;
    }
    elapsed = (uint32_t)(now - g_buzzer_test_start);
    if (elapsed < 80u || (elapsed >= 200u && elapsed < 280u)) {
        board_buzzer_set(1u);
    } else {
        board_buzzer_set(0u);
    }
    if (elapsed >= 320u) {
        g_buzzer_test = 0u;
        board_buzzer_set(0u);
    }
}

#if APP_STAGE >= 6
static int diagnostic_write(uint8_t address, uint8_t reg, uint8_t value)
{
    uint8_t bytes[2];
    bytes[0] = reg;
    bytes[1] = value;
    return mpu_bus_write(address, bytes, 2u);
}

static void send_i2c_scan(void)
{
    char text[96];
    int ack68 = mpu_bus_probe(0x68u);
    int ack69 = mpu_bus_probe(0x69u);
    (void)snprintf(text, sizeof(text),
        "I2C BUS SCL=%u SDA=%u\r\n",
        (unsigned)mpu_bus_scl_level(), (unsigned)mpu_bus_sda_level());
    (void)uart_send(text);
    (void)snprintf(text, sizeof(text),
        "I2C SCAN 0x68=%s 0x69=%s\r\n",
        ack68 ? "ACK" : "NO_ACK", ack69 ? "ACK" : "NO_ACK");
    (void)uart_send(text);
}

static void send_mpu_diagnostic(void)
{
    char text[112];
    uint8_t who68 = 0u, who69 = 0u;
    uint8_t address = 0u;
    int ack68 = mpu_bus_probe(0x68u);
    int ack69 = mpu_bus_probe(0x69u);
    int read68 = ack68 && mpu_bus_read_register_byte(0x68u, 0x75u, &who68);
    int read69 = ack69 && mpu_bus_read_register_byte(0x69u, 0x75u, &who69);

    (void)snprintf(text, sizeof(text),
        "MPU DIAG BUS SCL=%u SDA=%u\r\n",
        (unsigned)mpu_bus_scl_level(), (unsigned)mpu_bus_sda_level());
    (void)uart_send(text);
    (void)snprintf(text, sizeof(text),
        "ADDR 0x68 ACK=%u READ=%u WHO=0x%02X\r\n",
        (unsigned)(ack68 ? 1u : 0u), (unsigned)(read68 ? 1u : 0u), (unsigned)who68);
    (void)uart_send(text);
    (void)snprintf(text, sizeof(text),
        "ADDR 0x69 ACK=%u READ=%u WHO=0x%02X\r\n",
        (unsigned)(ack69 ? 1u : 0u), (unsigned)(read69 ? 1u : 0u), (unsigned)who69);
    (void)uart_send(text);

    if (read68 && (who68 == 0x68u || who68 == 0x69u)) { address = 0x68u; }
    else if (read69 && (who69 == 0x68u || who69 == 0x69u)) { address = 0x69u; }
    if (address == 0u) {
        (void)uart_send("MPU RESULT=NO_VALID_DEVICE\r\n");
        return;
    }
    (void)snprintf(text, sizeof(text),
        "MPU INIT ADDR=0x%02X WAKE=%u FILTER=%u GYRO=%u ACCEL=%u\r\n",
        (unsigned)address,
        (unsigned)(diagnostic_write(address, 0x6Bu, 0x00u) ? 1u : 0u),
        (unsigned)(diagnostic_write(address, 0x1Au, 0x03u) ? 1u : 0u),
        (unsigned)(diagnostic_write(address, 0x1Bu, 0x08u) ? 1u : 0u),
        (unsigned)(diagnostic_write(address, 0x1Cu, 0x08u) ? 1u : 0u));
    (void)uart_send(text);
}
#endif

static void send_status(void)
{
    char text[180];
#if APP_STAGE >= 4
    const SensorData *sensor = sensors_data();
    (void)snprintf(text, sizeof(text),
        "OK STAGE=%u LED1=%u LED2=%u FACE=%u OLED=%u SENSOR_AUTO=%u LIGHT=%u MPU=%u TAIL_AUTO=%u TAIL=%u TAIL_ON=%u UPTIME_MS=%lu\r\n",
        (unsigned)APP_STAGE,
        (unsigned)board_led1_get(), (unsigned)board_led2_get(),
        (unsigned)pet_ui_face(), (unsigned)pet_ui_oled_online(),
        (unsigned)g_sensor_auto, (unsigned)sensor->light_filtered,
        (unsigned)sensor->mpu_online, (unsigned)g_tail_auto,
        (unsigned)servo_angle(), (unsigned)servo_is_enabled(),
        (unsigned long)board_millis());
#elif APP_STAGE >= 3
    const SensorData *sensor = sensors_data();
    (void)snprintf(text, sizeof(text),
        "OK STAGE=%u LED1=%u LED2=%u FACE=%u OLED=%u SENSOR_AUTO=%u LIGHT=%u MPU=%u UPTIME_MS=%lu\r\n",
        (unsigned)APP_STAGE,
        (unsigned)board_led1_get(), (unsigned)board_led2_get(),
        (unsigned)pet_ui_face(), (unsigned)pet_ui_oled_online(),
        (unsigned)g_sensor_auto, (unsigned)sensor->light_filtered,
        (unsigned)sensor->mpu_online, (unsigned long)board_millis());
#elif APP_STAGE >= 2
    (void)snprintf(text, sizeof(text),
        "OK STAGE=%u LED1=%u LED2=%u AUTO_LED=%u BUZZER=%u FACE=%u AUTO_FACE=%u OLED=%u UPTIME_MS=%lu\r\n",
        (unsigned)APP_STAGE,
        (unsigned)board_led1_get(),
        (unsigned)board_led2_get(),
        (unsigned)g_led_auto,
        (unsigned)board_buzzer_get(),
        (unsigned)pet_ui_face(),
        (unsigned)pet_ui_auto_enabled(),
        (unsigned)pet_ui_oled_online(),
        (unsigned long)board_millis());
#else
    (void)snprintf(text, sizeof(text),
        "OK STAGE=%u LED1=%u LED2=%u AUTO=%u BUZZER=%u UPTIME_MS=%lu\r\n",
        (unsigned)APP_STAGE,
        (unsigned)board_led1_get(),
        (unsigned)board_led2_get(),
        (unsigned)g_led_auto,
        (unsigned)board_buzzer_get(),
        (unsigned long)board_millis());
#endif
    (void)uart_send(text);
}

#if APP_STAGE >= 3
static void send_sensor_status(void)
{
    const SensorData *s = sensors_data();
    char text[192];
    (void)snprintf(text, sizeof(text),
        "OK LIGHT_RAW=%u LIGHT_FILTER=%u MPU=%u AX=%d AY=%d AZ=%d GX=%d GY=%d GZ=%d AUTO=%u\r\n",
        (unsigned)s->light_raw, (unsigned)s->light_filtered,
        (unsigned)s->mpu_online,
        (int)s->ax, (int)s->ay, (int)s->az,
        (int)s->gx, (int)s->gy, (int)s->gz,
        (unsigned)g_sensor_auto);
    (void)uart_send(text);
}

static void sensor_reaction_task(uint32_t now)
{
    SensorEvent event = sensors_take_event();
    uint32_t duration = 0u;

#if APP_STAGE >= 5
    if (pet_game_is_sleeping()) { return; }
#endif

    if (g_sensor_reaction_active) {
        if (pet_ui_face() != g_sensor_reaction_face) {
            g_sensor_reaction_active = 0u; /* A button/encoder/serial choice wins. */
        } else if ((int32_t)(now - g_sensor_reaction_until) >= 0) {
            g_sensor_reaction_active = 0u;
#if APP_STAGE >= 5
            pet_game_restore_face();
#else
            pet_ui_set_face(PET_FACE_NORMAL);
#endif
        }
    }
    if (!g_sensor_auto || event == SENSOR_EVENT_NONE) { return; }
    pet_ui_set_auto(0u, now);
    switch (event) {
    case SENSOR_EVENT_LIGHT_CHANGE:
        g_sensor_reaction_face = PET_FACE_SURPRISED;
        duration = 550u;
#if APP_STAGE >= 4
        if (g_tail_auto) { servo_center(); }
#endif

        break;
    case SENSOR_EVENT_GENTLE_MOVE:
        g_sensor_reaction_face = PET_FACE_HAPPY;
        duration = 1000u;
#if APP_STAGE >= 4
        if (g_tail_auto) { servo_wag(now); }
#endif
        break;
    case SENSOR_EVENT_SHAKE:
        g_sensor_reaction_face = PET_FACE_DIZZY;
        duration = 1400u;
        start_buzzer_test(now);
#if APP_STAGE >= 4
        if (g_tail_auto) { servo_sway(now); }
#endif
        break;
    case SENSOR_EVENT_QUIET:
        g_sensor_reaction_face = PET_FACE_SLEEPY;
        duration = 2500u;
#if APP_STAGE >= 4
        if (g_tail_auto) { servo_center(); }
#endif
        break;
    default:
        break;
    }
    if (duration != 0u) {
        pet_ui_set_face(g_sensor_reaction_face);
        g_sensor_reaction_until = now + duration;
        g_sensor_reaction_active = 1u;
    }
}
#endif

#if APP_STAGE >= 5
static void send_pet_status(void)
{
    const PetGameState *pet = pet_game_state();
    char text[128];
    (void)snprintf(text, sizeof(text),
        "OK FULLNESS=%u HAPPINESS=%u ENERGY=%u SLEEP=%u VIEW=%u PAGE=%u\r\n",
        (unsigned)pet->fullness, (unsigned)pet->happiness,
        (unsigned)pet->energy, (unsigned)pet->sleeping,
        (unsigned)pet->status_visible, (unsigned)pet->view_page);
    (void)uart_send(text);
}
#endif

#if APP_STAGE >= 4
static int parse_tail_angle(const char *text, uint8_t *angle)
{
    unsigned value = 0u;
    if (*text == '\0') { return 0; }
    while (*text != '\0') {
        if (*text < '0' || *text > '9') { return 0; }
        value = value * 10u + (unsigned)(*text - '0');
        if (value > 255u) { return 0; }
        text++;
    }
    if (value < SERVO_MIN_ANGLE || value > SERVO_MAX_ANGLE) { return 0; }
    *angle = (uint8_t)value;
    return 1;
}
#endif

static void execute_command(const char *line)
{
    if (strcmp(line, "PING") == 0) {
        (void)uart_send("PONG\r\n");
    } else if (strcmp(line, "STATUS") == 0) {
        send_status();
    } else if (strcmp(line, "LED AUTO") == 0) {
        g_led_auto = 1u;
        (void)uart_send("OK LED AUTO\r\n");
    } else if (strcmp(line, "LED1 ON") == 0) {
        g_led_auto = 0u;
        board_led1_set(1u);
        (void)uart_send("OK LED1 ON\r\n");
    } else if (strcmp(line, "LED1 OFF") == 0) {
        g_led_auto = 0u;
        board_led1_set(0u);
        (void)uart_send("OK LED1 OFF\r\n");
    } else if (strcmp(line, "LED2 ON") == 0) {
        g_led_auto = 0u;
        board_led2_set(1u);
        (void)uart_send("OK LED2 ON\r\n");
    } else if (strcmp(line, "LED2 OFF") == 0) {
        g_led_auto = 0u;
        board_led2_set(0u);
        (void)uart_send("OK LED2 OFF\r\n");
    } else if (strcmp(line, "BUZZER TEST") == 0) {
        start_buzzer_test(board_millis());
        (void)uart_send("OK BUZZER TEST\r\n");
    } else if (strcmp(line, "BUZZER ON") == 0) {
        g_buzzer_test = 0u;
        board_buzzer_set(1u);
        (void)uart_send("OK BUZZER ON\r\n");
    } else if (strcmp(line, "BUZZER OFF") == 0) {
        g_buzzer_test = 0u;
        board_buzzer_set(0u);
        (void)uart_send("OK BUZZER OFF\r\n");
#if APP_STAGE >= 2
    } else if (strcmp(line, "FACE AUTO ON") == 0) {
        pet_ui_set_auto(1u, board_millis());
        (void)uart_send("OK FACE AUTO ON\r\n");
    } else if (strcmp(line, "FACE AUTO OFF") == 0) {
        pet_ui_set_auto(0u, board_millis());
        (void)uart_send("OK FACE AUTO OFF\r\n");
    } else if (strcmp(line, "FACE NEXT") == 0) {
        pet_ui_next();
        pet_ui_set_auto(0u, board_millis());
        (void)uart_send("OK FACE NEXT\r\n");
    } else if (strcmp(line, "FACE PREV") == 0) {
        pet_ui_previous();
        pet_ui_set_auto(0u, board_millis());
        (void)uart_send("OK FACE PREV\r\n");
    } else if (strncmp(line, "FACE ", 5u) == 0) {
        const char *name = line + 5;
        int face = -1;
        if (strcmp(name, "NORMAL") == 0) face = PET_FACE_NORMAL;
        else if (strcmp(name, "HAPPY") == 0) face = PET_FACE_HAPPY;
        else if (strcmp(name, "HUNGRY") == 0) face = PET_FACE_HUNGRY;
        else if (strcmp(name, "SLEEPY") == 0) face = PET_FACE_SLEEPY;
        else if (strcmp(name, "SLEEP") == 0) face = PET_FACE_SLEEP;
        else if (strcmp(name, "SURPRISED") == 0) face = PET_FACE_SURPRISED;
        else if (strcmp(name, "ANGRY") == 0) face = PET_FACE_ANGRY;
        else if (strcmp(name, "DIZZY") == 0) face = PET_FACE_DIZZY;
        else if (strcmp(name, "LEFT") == 0) face = PET_FACE_LOOK_LEFT;
        else if (strcmp(name, "RIGHT") == 0) face = PET_FACE_LOOK_RIGHT;
        else if (strcmp(name, "EAT") == 0) face = PET_FACE_EATING;
        else if (strcmp(name, "NAILONG") == 0) face = PET_FACE_NAILONG;
        if (face >= 0) {
            pet_ui_set_face((unsigned)face);
            pet_ui_set_auto(0u, board_millis());
            (void)uart_send("OK FACE SET\r\n");
        } else {
            (void)uart_send("ERR FACE NAME\r\n");
        }
    } else if (strcmp(line, "OLED STATUS") == 0) {
        (void)uart_send(pet_ui_oled_online() ? "OK OLED ONLINE\r\n" : "ERR OLED OFFLINE\r\n");
#endif
#if APP_STAGE >= 3
    } else if (strcmp(line, "SENSOR STATUS") == 0) {
        send_sensor_status();
    } else if (strcmp(line, "SENSOR AUTO ON") == 0) {
        g_sensor_auto = 1u;
        (void)uart_send("OK SENSOR AUTO ON\r\n");
    } else if (strcmp(line, "SENSOR AUTO OFF") == 0) {
        g_sensor_auto = 0u;
        (void)uart_send("OK SENSOR AUTO OFF\r\n");
#endif
#if APP_STAGE >= 4
    } else if (strcmp(line, "TAIL CENTER") == 0) {
        g_tail_auto = 0u;
        servo_center();
        (void)uart_send("OK TAIL CENTER\r\n");
    } else if (strcmp(line, "TAIL WAG") == 0) {
        g_tail_auto = 0u;
        servo_wag(board_millis());
        (void)uart_send("OK TAIL WAG\r\n");
    } else if (strcmp(line, "TAIL SWAY") == 0) {
        g_tail_auto = 0u;
        servo_sway(board_millis());
        (void)uart_send("OK TAIL SWAY\r\n");
    } else if (strcmp(line, "TAIL OFF") == 0) {
        g_tail_auto = 0u;
        servo_enable(0u);
        (void)uart_send("OK TAIL OFF\r\n");
    } else if (strcmp(line, "TAIL AUTO ON") == 0) {
        g_tail_auto = 1u;
        servo_center();
        (void)uart_send("OK TAIL AUTO ON\r\n");
    } else if (strcmp(line, "TAIL AUTO OFF") == 0) {
        g_tail_auto = 0u;
        (void)uart_send("OK TAIL AUTO OFF\r\n");
    } else if (strncmp(line, "TAIL SET ", 9u) == 0) {
        uint8_t angle;
        if (parse_tail_angle(line + 9, &angle)) {
            g_tail_auto = 0u;
            servo_enable(1u);
            servo_set_angle(angle);
            (void)uart_send("OK TAIL SET\r\n");
        } else {
            (void)uart_send("ERR TAIL RANGE 35..145\r\n");
        }
#endif
#if APP_STAGE >= 5
    } else if (strcmp(line, "PET STATUS") == 0) {
        send_pet_status();
    } else if (strcmp(line, "PET FEED") == 0) {
        pet_game_feed(board_millis());
        (void)uart_send("OK PET FEED\r\n");
    } else if (strcmp(line, "PET PLAY") == 0) {
        pet_game_play(board_millis());
        (void)uart_send("OK PET PLAY\r\n");
    } else if (strcmp(line, "PET SLEEP") == 0) {
        if (!pet_game_is_sleeping()) { pet_game_toggle_sleep(board_millis()); }
        (void)uart_send("OK PET SLEEP\r\n");
    } else if (strcmp(line, "PET WAKE") == 0) {
        if (pet_game_is_sleeping()) { pet_game_toggle_sleep(board_millis()); }
        (void)uart_send("OK PET WAKE\r\n");
    } else if (strcmp(line, "PET VIEW") == 0) {
        pet_game_toggle_status();
        (void)uart_send("OK PET VIEW\r\n");
    } else if (strcmp(line, "PET RESET") == 0) {
        pet_game_reset(board_millis());
        (void)uart_send("OK PET RESET\r\n");
#if APP_STAGE >= 6
    } else if (strcmp(line, "DIAG VIEW") == 0) {
        while (pet_game_state()->view_page != 2u) { pet_game_toggle_status(); }
        (void)uart_send("OK DIAG VIEW\r\n");
    } else if (strcmp(line, "DIAG EXIT") == 0) {
        while (pet_game_state()->view_page != 0u) { pet_game_toggle_status(); }
        (void)uart_send("OK DIAG EXIT\r\n");
    } else if (strcmp(line, "I2C SCAN") == 0) {
        send_i2c_scan();
    } else if (strcmp(line, "MPU DIAG") == 0) {
        send_mpu_diagnostic();
#endif
#endif
    } else if (strcmp(line, "HELP") == 0) {
#if APP_STAGE >= 2
        (void)uart_send("PING | STATUS | LED AUTO | LED1 ON/OFF | LED2 ON/OFF | BUZZER TEST/ON/OFF\r\n");
        (void)uart_send("FACE NEXT/PREV | FACE NORMAL/HAPPY/HUNGRY/SLEEPY/SLEEP/SURPRISED/ANGRY/DIZZY/LEFT/RIGHT/EAT/NAILONG\r\n");
        (void)uart_send("FACE AUTO ON/OFF | OLED STATUS\r\n");
#if APP_STAGE >= 3
        (void)uart_send("SENSOR STATUS | SENSOR AUTO ON/OFF\r\n");
#endif
#if APP_STAGE >= 4
        (void)uart_send("TAIL CENTER/WAG/SWAY/OFF | TAIL SET 35..145 | TAIL AUTO ON/OFF\r\n");
#endif
#if APP_STAGE >= 5
        (void)uart_send("PET STATUS/FEED/PLAY/SLEEP/WAKE/VIEW/RESET | K1 FEED K2 PLAY K3 SLEEP K4 VIEW\r\n");
#if APP_STAGE >= 6
        (void)uart_send("DIAG VIEW/EXIT | I2C SCAN | MPU DIAG | K4 CYCLES FACE/PET/DIAGNOSTIC\r\n");
#endif
#endif
#else
        (void)uart_send("PING | STATUS | LED AUTO | LED1 ON/OFF | LED2 ON/OFF | BUZZER TEST/ON/OFF\r\n");
#endif
    } else {
        (void)uart_send("ERR UNKNOWN COMMAND; SEND HELP\r\n");
    }
}

static void serial_task(void)
{
    static char line[LINE_SIZE];
    static uint8_t used;
    static uint8_t dropping;
    uint8_t value;
    unsigned budget = 64u;

    if (uart_rx_overflow()) {
        used = 0u;
        dropping = 1u;
    }
    while (budget-- != 0u && uart_read_byte(&value)) {
        if (value == '\r' || value == '\n') {
            if (dropping) {
                dropping = 0u;
                used = 0u;
                (void)uart_send("ERR INPUT TOO LONG OR RX OVERFLOW\r\n");
            } else if (used != 0u) {
                line[used] = '\0';
                execute_command(line);
                used = 0u;
            }
        } else if (!dropping) {
            if (value < 32u || value > 126u || used >= (LINE_SIZE - 1u)) {
                used = 0u;
                dropping = 1u;
            } else {
                line[used++] = (char)value;
            }
        }
    }
}

int main(void)
{
    uint32_t now;
    uint32_t led_update = 0u;

    board_init();
#if APP_STAGE >= 2
    pet_ui_init(board_millis());
#if APP_STAGE >= 3
    sensors_init(board_millis());
#if APP_STAGE >= 4
    servo_init();
#if APP_STAGE >= 5
    pet_game_init(board_millis());
#if APP_STAGE >= 6
    (void)uart_send("\r\nLIGHT_SPRITE STAGE6 READY\r\n");
#else
    (void)uart_send("\r\nLIGHT_SPRITE STAGE5 READY\r\n");
#endif
#else
    (void)uart_send("\r\nLIGHT_SPRITE STAGE4 READY\r\n");
#endif
#else
    (void)uart_send("\r\nLIGHT_SPRITE STAGE3 READY\r\n");
#endif
#else
    (void)uart_send("\r\nLIGHT_SPRITE STAGE2 READY\r\n");
#endif
#else
    (void)uart_send("\r\nLIGHT_SPRITE STAGE1 READY\r\n");
#endif
    (void)uart_send("UART 115200 8N1; SEND HELP\r\n");
    start_buzzer_test(board_millis());

    for (;;) {
        now = board_millis();
        serial_task();
        update_buzzer_test(now);
#if APP_STAGE >= 2
        pet_ui_task(now);
#endif
#if APP_STAGE >= 5
        pet_game_task(now);
#endif
#if APP_STAGE >= 3
        sensors_task(now);
        sensor_reaction_task(now);
#endif
#if APP_STAGE >= 4
        servo_task(now);
#endif

        if (g_led_auto && (uint32_t)(now - led_update) >= 500u) {
            led_update = now;
            board_led1_set((uint8_t)!board_led1_get());
            board_led2_set((uint8_t)!board_led1_get());
        }
    }
}

