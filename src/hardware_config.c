#include "hardware_config.h"

int ledc_pins[8] = {LED_B1_PIN, LED_Y1_PIN, LED_B2_PIN, LED_Y2_PIN, LED_B3_PIN, LED_Y3_PIN, LED_B4_PIN, LED_Y4_PIN};
ledc_channel_t ledc_channels[8] = {
    LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3,
    LEDC_CHANNEL_4, LEDC_CHANNEL_5, LEDC_CHANNEL_6, LEDC_CHANNEL_7
};

int sdm_pins[2] = {LED_B5_PIN, LED_Y5_PIN};
sdm_channel_handle_t sdm_handles[2];

adc_channel_t pot_channels[10] = {
    POT_B1_CHAN, POT_Y1_CHAN, POT_B2_CHAN, POT_Y2_CHAN,
    POT_B3_CHAN, POT_Y3_CHAN, POT_B4_CHAN, POT_Y4_CHAN,
    POT_B5_CHAN, POT_Y5_CHAN
};

adc_oneshot_unit_handle_t adc1_handle;

void init_pwm_leds(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_12_BIT, 
        .freq_hz          = 5000,              
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    for (int i = 0; i < 8; i++) {
        ledc_channel_config_t ledc_conf = {
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = ledc_channels[i],
            .timer_sel      = LEDC_TIMER_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = ledc_pins[i],
            .duty           = 0, 
            .hpoint         = 0
        };
        ledc_channel_config(&ledc_conf);
    }

    sdm_config_t sdm_config = {
        .clk_src = SDM_CLK_SRC_DEFAULT,
        .sample_rate_hz = 1000000,
    };
    for (int i = 0; i < 2; i++) {
        sdm_config.gpio_num = sdm_pins[i];
        sdm_new_channel(&sdm_config, &sdm_handles[i]);
        sdm_channel_enable(sdm_handles[i]);
    }
}

void init_adc_pots(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12, 
    };

    for (int i = 0; i < 10; i++) {
        adc_oneshot_config_channel(adc1_handle, pot_channels[i], &config);
    }
}

void init_i2c_lcd(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}