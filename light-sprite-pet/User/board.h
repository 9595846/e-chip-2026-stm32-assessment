#ifndef PET_BOARD_H
#define PET_BOARD_H

#include <stdint.h>

void board_init(void);
uint32_t board_millis(void);

void board_led1_set(uint8_t on);
void board_led2_set(uint8_t on);
uint8_t board_led1_get(void);
uint8_t board_led2_get(void);

void board_buzzer_set(uint8_t on);
uint8_t board_buzzer_get(void);

/* Pressed keys are returned as bits: K1=bit0, K2=bit1, K3=bit2, K4=bit3. */
uint8_t board_keys_raw(void);
int16_t board_encoder_delta(void);
uint16_t board_light_raw(void);

void bus_init(void);
int bus_write(uint8_t address, const uint8_t *data, unsigned length);
int bus_read_register_byte(uint8_t address, uint8_t reg, uint8_t *value);

int uart_read_byte(uint8_t *value);
int uart_send(const char *text);
int uart_rx_overflow(void);

#endif

