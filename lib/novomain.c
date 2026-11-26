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
#include "DHT22.h"

#define TOLERANCIA_UMIDADE 2.0f // variação de 2% em torno do sensor
#define tamMatriz 40
#define lowIntensity 1024 // 25% de potencia
#define mediumIntensity 2048 // 50% de potencia
#define highIntensity 4095 // 100% de potencia

#define TASK_INTERVAL1 500
#define TASK_INTERVAL2 500
#define TASK_INTERVAL3 1000

int codigoEvento = NENHUM_EVENTO;
int eventoInterno = NENHUM_EVENTO;
int estado = ESTADO_DEZENA;
int codigoAcao;
int acao_matrizTransicaoEstados[NUM_ESTADOS][NUM_EVENTOS];
int proximo_estado_matrizTransicaoEstados[NUM_ESTADOS][NUM_EVENTOS];

int umidade = 0; // guarda o numero que esta sendo digitado
int digitos_lidos = 0; // conta quantos digitos foram inseridos (máx 2)
int umidade_alvo = 25; // umidade de valor inicial
Estado estado_atual = ESTADO_DEZENA; // estado inicial
int opcao;
Evento evento;
Transicao t;
// DHT
const int pinosDHT[4] = {19, 20, 21, 0};
float temperaturas_dht[4] = {0};
float umidades_dht[4] = {0};
float diferenca_dht[4] = {0};
int contadorDHT = 40; // inicialização dos contadores em 40 para fazer a primeira leitura na inicialização
float umidades_efetivas[4] = {0};
float variacao_atuador[4] = {0};
char tecla;





















void taskMaqEstados(void *pvParameters);
void taskObterEvento(void *pvParameters);

QueueHandle_t xQueue;
SemaphoreHandle_t xBinarySemaphore;
TaskHandle_t xTaskMaqEstados, xTaskObterEvento;

int executarAcao(int codigoAcao)
{
    int retval;

    retval = NENHUM_EVENTO;
    if (codigoAcao == ACAO_NENHUMA)
        return retval;

    switch(codigoAcao)
    {
    case ACAO_A10:
        printf("[AÇÃO]: Botão numérico pressionado, aguardando unidade.\n");
        umidade = (umidade * 10) + (tecla - '0');
        digitos_lidos++;
        atualizar_lcd_umidade(digitos_lidos, umidade, false);
        break;
    case ACAO_A20:
        printf("[AÇÃO]: Botão numérico pressionado, aguardando confirmação.\n");
        umidade = (umidade * 10) + (tecla - '0');
        digitos_lidos++;
        atualizar_lcd_umidade(digitos_lidos, umidade, false);
        break;
    case ACAO_A30:
        printf("[AÇÃO]: Botão C pressionado - Confirmação da umidade e entra em estado de monitoramento.\n");
        umidade_alvo = umidade;
        atualizar_lcd_umidade(digitos_lidos, umidade, true);
        umidade = 0;
        digitos_lidos = 0;
        contadorDHT++;
        if (contadorDHT >= 40) { // A cada 2 segundos (40 * 50ms)
            contadorDHT = 0;
            bool leitura_atualizada = false; // <-- Criada aqui, zerada a cada ciclo
            for (int k = 0; k < 4; k++) {
                setDHTgpio(pinosDHT[k]);
                int ret = readDHT();
                if (ret == DHT_OK) {
                    temperaturas_dht[k] = getTemperature();
                    umidades_dht[k] = getHumidity();
                    leitura_atualizada = true; // <-- Marca que pelo menos 1 DHT leu direito
                } else {
                    printf("Erro ao ler DHT %d (GPIO %d): ", k + 1, pinosDHT[k]);
                    errorHandler(ret);
                }
            }
            // Só interpola a matriz e atualiza o LCD se houve leitura válida
            if (leitura_atualizada) {
                for (int k = 0; k < 4; k++){
                    umidades_efetivas[k] = umidades_dht[k] + variacao_atuador[k];
                    if(umidades_efetivas[k] < 0.0f) umidades_efetivas[k] = 0.0f;
                    if (umidades_efetivas[k] > 100.0f) umidades_efetivas[k] = 100.0f;
                }
                float umidade_media = simularUmidade(matriz_area_atuacao, tamMatriz, extr, umidades_efetivas);
                printf("DHTs: [%.1f%%, %.1f%%, %.1f%%, %.1f%%] | Media Matriz: %.1f%%\n",
                        umidades_efetivas[0], umidades_efetivas[1], umidades_efetivas[2], umidades_efetivas[3], umidade_media);
                atualizar_lcd_media_atual(umidade_media);
                calcularDiferencasDHT(umidades_efetivas, umidade_alvo, diferenca_dht);
                // verifica se todas as zonas de leitura estao ok
                bool todas_ok = true;
                for (int k = 0; k < 4; k++){
                    if (diferenca_dht[k] > TOLERANCIA_UMIDADE || diferenca_dht[k] < -TOLERANCIA_UMIDADE){
                        todas_ok = false;
                        break;
                    }
                }
        break;
    case ACAO_A35:
        printf("[AÇÃO]: Botão D pressionado - Volta para o estado de digito da unidade.\n");
        umidade = 0;
        digitos_lidos = 0;
        atualizar_lcd_umidade(digitos_lidos, umidade, false);
        break;
    case ACAO_A40:
        printf("[AÇÃO]: Valores estáveis - Desativa os umidificadores e monitora o ambiente.\n");
        lcd_set_cursor(LCD2_ADDR, 0, 0);
        lcd_send_string(LCD2_ADDR, "Monitorando...  ")
        break;
    case ACAO_A50:
       printf("[AÇÃO]: Valores de umidade ABAIXO da tolerância - Ativa umidificador.\n");
        lcd_set_cursor(LCD2_ADDR, 0, 0);
        lcd_send_string(LCD2_ADDR, "Umidificando... ");
        atualizarAtuadores(estado_atual, diferenca_dht, variacao_atuador);
        break;
    case ACAO_A55:
        printf("[AÇÃO]: Valores de umidade ACIMA da tolerância - Ativa desumidificador.\n");
        lcd_set_cursor(LCD2_ADDR, 0, 0);
        lcd_send_string(LCD2_ADDR, "Desumidificando ");
        atualizarAtuadores(estado_atual, diferenca_dht, variacao_atuador);
        break;
    case ACAO_A60:
        printf("[AÇÃO]: Botão reset acionado - Retorna ao estado inicial.\n");
        umidade = 0;
        digitos_lidos = 0;
        contadorDHT = 40;
        desligarAtuadores();
        lcd_set_cursor(LCD1_ADDR, 0, 0);
        lcd_send_string(LCD1_ADDR, "                ");
        lcd_set_cursor(LCD1_ADDR, 1, 0);
        lcd_send_string(LCD1_ADDR, "                ");
        atualizar_lcd_umidade(digitos_lidos, umidade, false);
        break;

    } // switch

    return retval;
} // executarAcao









