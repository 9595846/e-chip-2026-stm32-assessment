#include "sensors.h"
#include "board.h"
#include "config.h"
#include "mpu_soft_i2c.h"

#if APP_STAGE >= 3

static SensorData g_data;
static SensorEvent g_event;
static uint32_t g_last_sample;
static uint32_t g_retry_at;
static uint32_t g_event_cooldown;
static uint32_t g_last_motion;
static uint8_t g_quiet_sent;
static int32_t g_light_slow;
static int16_t g_prev_ax, g_prev_ay, g_prev_az;
static uint8_t g_have_mpu_sample;
static uint8_t g_mpu_address;

static int32_t magnitude(int16_t a, int16_t b, int16_t c)
{
    int32_t aa = a < 0 ? -(int32_t)a : (int32_t)a;
    int32_t bb = b < 0 ? -(int32_t)b : (int32_t)b;
    int32_t cc = c < 0 ? -(int32_t)c : (int32_t)c;
    return aa + bb + cc;
}

static int mpu_write(uint8_t reg, uint8_t value)
{
    uint8_t bytes[2];
    bytes[0] = reg; bytes[1] = value;
    return mpu_bus_write(g_mpu_address, bytes, 2u);
}

static int mpu_read_word(uint8_t reg, int16_t *value)
{
    uint8_t high, low;
    if (!mpu_bus_read_register_byte(g_mpu_address, reg, &high) ||
        !mpu_bus_read_register_byte(g_mpu_address, (uint8_t)(reg + 1u), &low)) {
        return 0;
    }
    *value = (int16_t)(((uint16_t)high << 8) | low);
    return 1;
}

static int mpu_start(void)
{
    uint8_t who = 0u;
    g_mpu_address = 0x68u;
    if (!mpu_bus_read_register_byte(g_mpu_address, 0x75u, &who) ||
        (who != 0x68u && who != 0x69u)) {
        g_mpu_address = 0x69u;
        if (!mpu_bus_read_register_byte(g_mpu_address, 0x75u, &who) ||
            (who != 0x68u && who != 0x69u)) {
            return 0;
        }
    }
    if (!mpu_write(0x6Bu, 0x00u)) { return 0; } /* Wake, internal 8 MHz clock. */
    if (!mpu_write(0x1Au, 0x03u)) { return 0; } /* 44 Hz low-pass filter. */
    if (!mpu_write(0x1Bu, 0x08u)) { return 0; } /* Gyro +/-500 deg/s. */
    if (!mpu_write(0x1Cu, 0x08u)) { return 0; } /* Accel +/-4 g. */
    return 1;
}

static int mpu_sample(void)
{
    return mpu_read_word(0x3Bu, &g_data.ax) &&
           mpu_read_word(0x3Du, &g_data.ay) &&
           mpu_read_word(0x3Fu, &g_data.az) &&
           mpu_read_word(0x43u, &g_data.gx) &&
           mpu_read_word(0x45u, &g_data.gy) &&
           mpu_read_word(0x47u, &g_data.gz);
}

static void offer_event(SensorEvent event, uint32_t now, uint32_t cooldown)
{
    if ((int32_t)(now - g_event_cooldown) >= 0 && g_event == SENSOR_EVENT_NONE) {
        g_event = event;
        g_event_cooldown = now + cooldown;
    }
}

void sensors_init(uint32_t now)
{
    uint16_t light = board_light_raw();
    g_data.light_raw = light;
    g_data.light_filtered = light;
    g_light_slow = light;
    g_event = SENSOR_EVENT_NONE;
    g_last_sample = now;
    g_retry_at = now;
    g_event_cooldown = now + 1500u;
    g_last_motion = now;
    g_quiet_sent = 0u;
    g_prev_ax = g_prev_ay = g_prev_az = 0;
    g_have_mpu_sample = 0u;
    g_mpu_address = MPU6050_ADDRESS;
    g_data.mpu_online = 0u;
    mpu_bus_init();
}

void sensors_task(uint32_t now)
{
    uint16_t raw;
    int32_t light_difference;
    int32_t gyro_activity;
    int32_t accel_change;

    if ((uint32_t)(now - g_last_sample) < SENSOR_SAMPLE_MS) { return; }
    g_last_sample = now;
    raw = board_light_raw();
    g_data.light_raw = raw;
    g_data.light_filtered = (uint16_t)(((uint32_t)g_data.light_filtered * 7u + raw) / 8u);
    light_difference = (int32_t)g_data.light_filtered - g_light_slow;
    if (light_difference < 0) { light_difference = -light_difference; }
    if ((uint32_t)light_difference >= LIGHT_CHANGE_THRESHOLD) {
        offer_event(SENSOR_EVENT_LIGHT_CHANGE, now, 2500u);
    }
    g_light_slow += ((int32_t)g_data.light_filtered - g_light_slow) / 64L;

    if (!g_data.mpu_online) {
        if ((int32_t)(now - g_retry_at) >= 0) {
            g_retry_at = now + 1000u;
            g_data.mpu_online = (uint8_t)(mpu_start() ? 1u : 0u);
        }
        return;
    }
    if (!mpu_sample()) {
        g_data.mpu_online = 0u;
        g_retry_at = now + 1000u;
        return;
    }
    if (!g_have_mpu_sample) {
        g_prev_ax = g_data.ax; g_prev_ay = g_data.ay; g_prev_az = g_data.az;
        g_have_mpu_sample = 1u;
        return;
    }
    gyro_activity = magnitude(g_data.gx, g_data.gy, g_data.gz);
    accel_change = magnitude(
        (int16_t)(g_data.ax - g_prev_ax),
        (int16_t)(g_data.ay - g_prev_ay),
        (int16_t)(g_data.az - g_prev_az));
    g_prev_ax = g_data.ax; g_prev_ay = g_data.ay; g_prev_az = g_data.az;

    if (gyro_activity >= MPU_SHAKE_THRESHOLD || accel_change >= MPU_SHAKE_ACCEL_DELTA) {
        g_last_motion = now; g_quiet_sent = 0u;
        offer_event(SENSOR_EVENT_SHAKE, now, 2500u);
    } else if (gyro_activity >= MPU_GENTLE_THRESHOLD || accel_change >= MPU_GENTLE_ACCEL_DELTA) {
        g_last_motion = now; g_quiet_sent = 0u;
        offer_event(SENSOR_EVENT_GENTLE_MOVE, now, 2500u);
    } else if (!g_quiet_sent && (uint32_t)(now - g_last_motion) >= 20000u) {
        g_quiet_sent = 1u;
        offer_event(SENSOR_EVENT_QUIET, now, 5000u);
    }
}

const SensorData *sensors_data(void) { return &g_data; }

SensorEvent sensors_take_event(void)
{
    SensorEvent result = g_event;
    g_event = SENSOR_EVENT_NONE;
    return result;
}

#else
static SensorData g_data;
void sensors_init(uint32_t now) { (void)now; }
void sensors_task(uint32_t now) { (void)now; }
const SensorData *sensors_data(void) { return &g_data; }
SensorEvent sensors_take_event(void) { return SENSOR_EVENT_NONE; }
#endif

