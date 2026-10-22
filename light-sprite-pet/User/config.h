#ifndef PET_CONFIG_H
#define PET_CONFIG_H

#ifndef APP_STAGE
#define APP_STAGE 2
#endif

#define CPU_HZ              64000000u
#define UART_BAUDRATE       115200u
#define I2C_SPEED_HZ        100000u
#define OLED_ADDRESS        0x3Cu
#define OLED_COLUMN_OFFSET  0u
#define MPU6050_ADDRESS     0x68u
#define SENSOR_SAMPLE_MS    40u
#define KEY_DEBOUNCE_MS     20u
#define KEY_LONG_MS         800u
#define ENCODER_COUNTS_PER_DETENT 4
#define ENCODER_DIRECTION   1

/* Stage 1 fixed pins. Keep these aligned with the approved wiring document. */
#define LED1_GPIO           GPIOA
#define LED1_PIN            GPIO_Pin_0
#define LED2_GPIO           GPIOB
#define LED2_PIN            GPIO_Pin_0
#define BUZZER_GPIO         GPIOB
#define BUZZER_PIN          GPIO_Pin_14

/* MH-FMD is active-low: 0 = sound, 1 = silent. */
#define BUZZER_ACTIVE_LOW   1

/* Stage 3 thresholds are deliberately conservative and must be checked on the real assembly. */
#define LIGHT_CHANGE_THRESHOLD 350u
#define MPU_GENTLE_THRESHOLD   2500L
#define MPU_SHAKE_THRESHOLD    15000L
#define MPU_GENTLE_ACCEL_DELTA 900L
#define MPU_SHAKE_ACCEL_DELTA  5500L

/* Stage 4 tail limits. Start conservatively; tune only after linkage verification. */
#define SERVO_MIN_ANGLE        35u
#define SERVO_CENTER_ANGLE     90u
#define SERVO_MAX_ANGLE        145u
#define SERVO_MIN_PULSE_US     1100u
#define SERVO_MAX_PULSE_US     1900u

#endif

