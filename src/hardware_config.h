#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include "driver/ledc.h"
#include "driver/sdm.h"
#include "esp_adc/adc_oneshot.h"
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

#define POT_B1_CHAN ADC_CHANNEL_0 
#define POT_Y1_CHAN ADC_CHANNEL_1 
#define POT_B2_CHAN ADC_CHANNEL_2 
#define POT_Y2_CHAN ADC_CHANNEL_3 
#define POT_B3_CHAN ADC_CHANNEL_4 
#define POT_Y3_CHAN ADC_CHANNEL_5 
#define POT_B4_CHAN ADC_CHANNEL_6 
#define POT_Y4_CHAN ADC_CHANNEL_7 
#define POT_B5_CHAN ADC_CHANNEL_8 
#define POT_Y5_CHAN ADC_CHANNEL_9 

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
#define LCD1_ADDR 0x27
#define LCD2_ADDR 0x3F

extern adc_channel_t pot_channels[10];
extern adc_oneshot_unit_handle_t adc1_handle;
extern int ledc_pins[8];
extern ledc_channel_t ledc_channels[8];
extern int sdm_pins[2];
extern sdm_channel_handle_t sdm_handles[2];

void init_pwm_leds(void);
void init_adc_pots(void);
void init_i2c_lcd(void);

#endif 