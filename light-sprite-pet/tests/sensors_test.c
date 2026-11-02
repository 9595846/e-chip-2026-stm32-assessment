#include <assert.h>
#include <stdio.h>
#include "sensors.h"
#include "mpu_soft_i2c.h"

static int16_t motion[6];
static int read_count;
static int fail_read = -1;
uint16_t board_light_raw(void) { return 1000u; }
void mpu_bus_init(void) { }
int mpu_bus_write(uint8_t address, const uint8_t *data, unsigned length)
{ (void)address; (void)data; (void)length; return 1; }
int mpu_bus_read_register_byte(uint8_t address, uint8_t reg, uint8_t *value)
{
    unsigned index, offset;
    uint16_t word;
    (void)address;
    if (reg == 0x75u) { *value = 0x68u; return 1; }
    if (read_count++ == fail_read) { return 0; }
    offset = reg >= 0x43u ? reg - 0x43u : reg - 0x3Bu;
    index = offset / 2u + (reg >= 0x43u ? 3u : 0u);
    assert(index < 6u);
    word = (uint16_t)motion[index];
    *value = (uint8_t)((offset & 1u) ? word : word >> 8);
    return 1;
}
static void reset(void)
{
    unsigned i;
    fail_read = -1; read_count = 0;
    for (i = 0; i < 6; i++) { motion[i] = 0; }
    sensors_init(0u);
    sensors_task(40u); /* Connect. */
    assert(sensors_data()->mpu_online);
}
static void sample(uint32_t now)
{ read_count = 0; sensors_task(now); }
int main(void)
{
    int fail;
    reset();
    motion[0] = 32767; sample(80u);
    motion[0] = -32768; sample(1600u);
    assert(sensors_take_event() == SENSOR_EVENT_SHAKE);
    reset();
    motion[0] = -32768; sample(80u);
    motion[0] = 32767; sample(1600u);
    assert(sensors_take_event() == SENSOR_EVENT_SHAKE);
    /* Failure at any byte must preserve the entire last complete frame. */
    for (fail = 0; fail < 12; fail++) {
        SensorData before;
        reset(); motion[0] = 10; motion[1] = 20; motion[2] = 30;
        motion[3] = 40; motion[4] = 50; motion[5] = 60;
        sample(80u); before = *sensors_data();
        motion[0] = 30000; motion[1] = 31000; motion[2] = 32000;
        motion[3] = 10000; motion[4] = 11000; motion[5] = 12000;
        fail_read = fail; sample(1600u);
        assert(!sensors_data()->mpu_online);
        assert(sensors_data()->ax == before.ax && sensors_data()->ay == before.ay);
        assert(sensors_data()->az == before.az && sensors_data()->gx == before.gx);
        assert(sensors_data()->gy == before.gy && sensors_data()->gz == before.gz);
        fail_read = -1;
        sample(2600u); /* Reconnect. */
        assert(sensors_data()->mpu_online);
        motion[3] = motion[4] = motion[5] = 0;
        sample(2640u); /* First frame establishes a fresh baseline. */
        assert(sensors_take_event() == SENSOR_EVENT_NONE);
        sample(2680u);
        assert(sensors_take_event() == SENSOR_EVENT_NONE);
        motion[0] = 0; sample(2720u);
        assert(sensors_take_event() == SENSOR_EVENT_SHAKE);
    }
    puts("sensors: signed extremes and all 12 read-failure/reconnect cases passed");
    return 0;
}
