#ifndef HARDWARE_CONFIG_H // include guards
#define HARDWARE_CONFIG_H

#include "driver/ledc.h"
#include "driver/i2c.h"

#define LED_B1_PIN 35
#define LED_B2_PIN 36
#define LED_B3_PIN 37
#define LED_B4_PIN 38
#define LED_B5_PIN 39

#define LED_Y1_PIN 40
#define LED_Y2_PIN 41
#define LED_Y3_PIN 42
#define LED_Y4_PIN 47
#define LED_Y5_PIN 48

#define DHT1_PIN 19
#define DHT2_PIN 20
#define DHT3_PIN 21
#define DHT4_PIN 0
#define DHT5_PIN 44

#define KEYPAD_R1 11
#define KEYPAD_R2 12
#define KEYPAD_R3 13
#define KEYPAD_R4 14
#define KEYPAD_C1 15
#define KEYPAD_C2 16
#define KEYPAD_C3 17
#define KEYPAD_C4 18

#define I2C_MASTER_SDA_IO 45
#define I2C_MASTER_SCL_IO 46
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define LCD1_ADDR 0x27 // endereços I2C de 7 bits, serve para diferenciar os LCDs porque utilizam os mesmos fios (SDA - 45 e SCL - 46)
#define LCD2_ADDR 0x3F

extern int ledc_pins[8]; // declara que o array existe, mas não é alocado nesse arquivo
extern ledc_channel_t ledc_channels[8];


// declaaração das funções

void init_pwm_leds(void);
void init_i2c_lcd(void);
void init_keypad(void);
char ler_teclado(void);
void lcd_init(uint8_t addr);
void lcd_clear(uint8_t addr);
void lcd_set_cursor(uint8_t addr, uint8_t row, uint8_t col);
void lcd_send_string(uint8_t addr, const char *str);

#endif 