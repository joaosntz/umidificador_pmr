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


#define tamMatriz 500

typedef struct{
    int i;
    int j;
    const char* nome;
} coordMatriz;

void inicializarExtremidades(coordMatriz extr[5]){
    // visando aprimorar o algoritmo da IDW, precisamos definir uma margem de atuação do sensor (nunca num extremo especifico)

    int min_borda = tamMatriz * 0.02;
    int max_borda = tamMatriz - 1 - (tamMatriz * 0.02);
    int meio = tamMatriz / 2; // divisao inteira, pega sempre o inteiro mais próximo, faz com que fique no centro

    extr[0] = (coordMatriz){.i = min_borda, .j = min_borda, .nome = "Superior Esquerdo"};
    extr[1] = (coordMatriz){.i = min_borda, .j = max_borda, .nome = "Superior Direito"};
    extr[2] = (coordMatriz){.i = max_borda, .j = min_borda, .nome = "Inferior Esquerdo"};
    extr[3] = (coordMatriz){.i = max_borda, .j = max_borda, .nome = "Inferior Direito"};
    extr[4] = (coordMatriz){.i = meio, .j = meio, .nome = "Centro"};
}

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

void lerSensores(float** matriz, int m, int n){
}

void app_main(void) {
    // INICIALIZAÇÃO DAS FUNÇÕES DOS COMPONENTES DE HARDWARE

    init_pwm_leds();
    init_adc_pots();
    init_i2c_lcd();

    // --------------------------------------------------

    // Declaração da matriz da área de atuação
    
    int i = tamMatriz;
    int j = tamMatriz;
    float** matriz_area_atuacao = criarMatriz(i, j);



    // --------------------------------------------------


    // verifica o tamanho da matriz e onde são os extremos (utilizados para calcular a distancia euclidiana)

    coordMatriz extr[5];
    inicializarExtremidades(extr);
    for(int i = 0; i < 5; i++){
        printf("Extremidade %d: %s (i: %d, j: %d)\n", i, extr[i].nome, extr[i].i, extr[i].j);
    }

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
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}