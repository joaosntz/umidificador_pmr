#include "hardware_config.h"
#include <driver/gpio.h>
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

int ledc_pins[8] = {LED_B1_PIN, LED_Y1_PIN, LED_B2_PIN, LED_Y2_PIN, LED_B3_PIN, LED_Y3_PIN, LED_B4_PIN, LED_Y4_PIN};
ledc_channel_t ledc_channels[8] = {
    LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3,
    LEDC_CHANNEL_4, LEDC_CHANNEL_5, LEDC_CHANNEL_6, LEDC_CHANNEL_7
};



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

void init_keypad(void) {
    // 1. Configurar as Linhas (R1 a R4) como Saídas
    uint64_t row_pin_mask = (1ULL << KEYPAD_R1) | (1ULL << KEYPAD_R2) | 
                            (1ULL << KEYPAD_R3) | (1ULL << KEYPAD_R4);
    
    gpio_config_t io_conf_out = {
        .pin_bit_mask = row_pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_out);

    // 2. Configurar as Colunas (C1 a C4) como Entradas com Pull-up
    uint64_t col_pin_mask = (1ULL << KEYPAD_C1) | (1ULL << KEYPAD_C2) | 
                            (1ULL << KEYPAD_C3) | (1ULL << KEYPAD_C4);
    
    gpio_config_t io_conf_in = {
        .pin_bit_mask = col_pin_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // Essencial para estabilizar os botões abertos
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_in);
}


// teclado numérico

static const int i_pins[4] = {KEYPAD_R1, KEYPAD_R2, KEYPAD_R3, KEYPAD_R4};
static const int j_pins[4] = {KEYPAD_C1, KEYPAD_C2, KEYPAD_C3, KEYPAD_C4};

static const char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

char ler_teclado(void){
    static char ultima_tecla = '\0';
    char tecla_atual = '\0';

    for (int i = 0; i < 4; i++){
        gpio_set_level(i_pins[i], 0); // ativa a linha i
        esp_rom_delay_us(5);

        for (int j = 0; j < 4; j++){
            if (gpio_get_level(j_pins[j]) == 0){
                tecla_atual = keymap[i][j];
                break;
            }
        }
        gpio_set_level(i_pins[i], 1);
        if (tecla_atual != '\0') {
            break;
        }
    }

    if (tecla_atual != '\0' && ultima_tecla == '\0'){
        ultima_tecla = tecla_atual;
        return tecla_atual;
    } else if (tecla_atual == '\0'){
        ultima_tecla = '\0';
    }

    return '\0';
}



/// LCD (biblioteca importada, deus me livre fazer isso a mão)

static void lcd_send_internal(uint8_t addr, uint8_t data, uint8_t flags) {
    uint8_t up = data & 0xF0;
    uint8_t lo = (data << 4) & 0xF0;
    uint8_t data_arr[4];

    // Bit 3 (0x08) = Backlight ligado | Bit 2 (0x04) = Pulso de Enable | Bit 0 (0x01) = RS (0: Comando, 1: Dado)
    data_arr[0] = up | flags | 0x08 | 0x04;
    data_arr[1] = up | flags | 0x08;
    data_arr[2] = lo | flags | 0x08 | 0x04;
    data_arr[3] = lo | flags | 0x08;

    i2c_master_write_to_device(I2C_MASTER_NUM, addr, data_arr, sizeof(data_arr), pdMS_TO_TICKS(100));
    esp_rom_delay_us(50);
}

void lcd_send_cmd(uint8_t addr, uint8_t cmd) {
    lcd_send_internal(addr, cmd, 0x00); // RS = 0 para comando
}

void lcd_send_data(uint8_t addr, uint8_t data) {
    lcd_send_internal(addr, data, 0x01); // RS = 1 para caractere ASCII
}

// Envia apenas os 4 bits superiores (1 único pulso de Enable) para o handshake inicial
static void lcd_send_nibble_init(uint8_t addr, uint8_t nibble) {
    uint8_t data_arr[2];
    uint8_t val = (nibble & 0xF0) | 0x08; // Mantém Backlight (0x08) ligado, RS = 0

    data_arr[0] = val | 0x04; // Enable = 1
    data_arr[1] = val;        // Enable = 0

    i2c_master_write_to_device(I2C_MASTER_NUM, addr, data_arr, sizeof(data_arr), pdMS_TO_TICKS(100));
    esp_rom_delay_us(100);
}

void lcd_init(uint8_t addr) {
    vTaskDelay(pdMS_TO_TICKS(50)); // Aguarda estabilização de tensão (VCC)

    // Sequência oficial da Hitachi para forçar reset por software em 8-bit:
    lcd_send_nibble_init(addr, 0x30);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_nibble_init(addr, 0x30);
    esp_rom_delay_us(200);
    lcd_send_nibble_init(addr, 0x30);
    esp_rom_delay_us(200);

    // Agora sim: 1 único pulso de 0x20 muda o hardware para o modo 4-bits alinhado!
    lcd_send_nibble_init(addr, 0x20);
    esp_rom_delay_us(200);

    // Daqui em diante o LCD já opera em 4-bits (2 nibbles por comando):
    lcd_send_cmd(addr, 0x28); // 2 linhas, fonte 5x8
    lcd_send_cmd(addr, 0x08); // Display OFF
    lcd_send_cmd(addr, 0x01); // Clear Display
    vTaskDelay(pdMS_TO_TICKS(5)); // O comando Clear exige pelo menos 2ms!
    lcd_send_cmd(addr, 0x06); // Cursor avança para a direita
    lcd_send_cmd(addr, 0x0C); // Display ON, Cursor OFF
}

void lcd_clear(uint8_t addr) {
    lcd_send_cmd(addr, 0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_set_cursor(uint8_t addr, uint8_t row, uint8_t col) {
    uint8_t pos = (row == 0) ? (0x80 + col) : (0xC0 + col);
    lcd_send_cmd(addr, pos);
}

void lcd_send_string(uint8_t addr, const char *str) {
    while (*str) {
        lcd_send_data(addr, (uint8_t)(*str));
        str++;
    }
}