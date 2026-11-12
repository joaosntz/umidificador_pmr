#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "estados.h"
#include "funcoes.h"

#define LED_PIN GPIO_NUM_2

static const char *TAG = "MAIN";


float** criarMatriz(int m, int n){
    // Função para criar uma matriz M x N
    float** matriz = (float**)malloc(m * sizeof(float*));
    if (matriz == nullptr) {
        ESP_LOGE(TAG, "Falha ao alocar memória para a matriz");
        return nullptr;
    }
    for (int i = 0; i < m; i++){
        matriz[i] = (float*)malloc(n * sizeof(float));
        if (matriz[i] == nullptr) {
            ESP_LOGE(TAG, "Falha ao alocar memória para a linha %d da matriz", i);
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
        ESP_LOGE(TAG, "Matriz nula, nada a liberar");
        return;
    }
    for (int i = 0; i < m; i++){
        free(matriz[i]);
    }
    free(matriz);
}

void app_main(void){
    

}