#ifndef PET_SENSORS_H
#define PET_SENSORS_H

#include <stdint.h>

typedef enum {
    SENSOR_EVENT_NONE = 0,
    SENSOR_EVENT_LIGHT_CHANGE,
    SENSOR_EVENT_GENTLE_MOVE,
    SENSOR_EVENT_SHAKE,
    SENSOR_EVENT_QUIET
} SensorEvent;

typedef struct {
    uint16_t light_raw;
    uint16_t light_filtered;
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    uint8_t mpu_online;
} SensorData;

void sensors_init(uint32_t now);
void sensors_task(uint32_t now);
const SensorData *sensors_data(void);
SensorEvent sensors_take_event(void);

#endif

