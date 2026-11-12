#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "estados.h"
#include "funcoes.h"
#include "driver/ledc.h"
#include "driver/sdm.h"
#include "esp_adc/adc_oneshot.h"
#include "hardware_config.h"

float** criarMatriz(int m, int n){
    // Função para criar uma matriz M x N
    float** matriz = (float**)malloc(m * sizeof(float*));
    if (matriz == nullptr) {
        printf("\nFalha ao alocar memória para a matriz\n");
        return nullptr;
    }
    for (int i = 0; i < m; i++){
        matriz[i] = (float*)malloc(n * sizeof(float));
        if (matriz[i] == nullptr) {
            printf("\nFalha ao alocar memória para a linha %d da matriz\n", i);
            // Libera as linhas já alocadas antes de retornar
            for (int j = 0; j < i; j++) {
                free(matriz[j]);
            }
            free(matriz);
            return nullptr;
        }
    }
    return matriz;
}

void liberarMatriz(float** matriz, int m){
    // Função para liberar a memória alocada para a matriz
    if (matriz == nullptr) {
        printf("\nMatriz nula, nada a liberar\n");
        return;
    }
    for (int i = 0; i < m; i++){
        free(matriz[i]);
    }
    free(matriz);
}

void app_main(void) {
    // INICIALIZAÇÃO DAS FUNÇÕES DOS COMPONENTES DE HARDWARE

    init_pwm_leds();
    init_adc_pots();
    init_i2c_lcd();

    // --------------------------------------------------


    int pot_values[10];

    while (1) {
        for (int i = 0; i < 10; i++) {
            adc_oneshot_read(adc1_handle, pot_channels[i], &pot_values[i]);
            
            if (i < 8) {
                ledc_set_duty(LEDC_LOW_SPEED_MODE, ledc_channels[i], pot_values[i]);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, ledc_channels[i]);
            } else {
                int32_t densidade_calculada = (pot_values[i] * 255 / 4095) - 128;
                int8_t sdm_duty = (int8_t)densidade_calculada;
                
                sdm_channel_set_pulse_density(sdm_handles[i - 8], sdm_duty);
                printf("LED Centro [%d] - Leitura ADC: %d | Sinal SDM: %d\n", i, pot_values[i], sdm_duty);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}