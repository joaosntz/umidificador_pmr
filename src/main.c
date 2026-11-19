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

typedef struct{
    int i;
    int j;
    const char* nome;
} coordMatriz;

void inicializarExtremidades(coordMatriz extr[4]){
    // visando aprimorar o algoritmo da IDW, precisamos definir uma margem de atuação do sensor (nunca num extremo especifico)

    int min_borda = 0; //tamMatriz * 0.02;
    int max_borda = tamMatriz - 1;
    int meio = tamMatriz / 2; // divisao inteira, pega sempre o inteiro mais próximo, faz com que fique no centro

    extr[0] = (coordMatriz){.i = min_borda, .j = min_borda, .nome = "Superior Esquerdo"};
    extr[1] = (coordMatriz){.i = min_borda, .j = max_borda, .nome = "Superior Direito"};
    extr[2] = (coordMatriz){.i = max_borda, .j = min_borda, .nome = "Inferior Esquerdo"};
    extr[3] = (coordMatriz){.i = max_borda, .j = max_borda, .nome = "Inferior Direito"};
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

void calcularDiferencasDHT(const float umidades_dht[4], int umidade_alvo, float diferencas[4]) {
    printf("--- Diferenca por Sensor (Alvo: %d%%) ---\n", umidade_alvo);

    for (int k = 0; k < 4; k++) {
        // Positivo = falta umidade | Negativo = excesso de umidade
        diferencas[k] = (float)umidade_alvo - umidades_dht[k];

        if (diferencas[k] > TOLERANCIA_UMIDADE) {
            printf("DHT%d: %.1f%% | Variar: +%.1f%% -> UMIDIFICAR\n",
                   k + 1, umidades_dht[k], diferencas[k]);
        } else if (diferencas[k] < -TOLERANCIA_UMIDADE) {
            printf("DHT%d: %.1f%% | Variar: %.1f%% -> DESUMIDIFICAR\n",
                   k + 1, umidades_dht[k], diferencas[k]);
        } else {
            printf("DHT%d: %.1f%% | Variar: %+.1f%% -> ESTAVEL\n",
                   k + 1, umidades_dht[k], diferencas[k]);
        }
    }
}

void executar_acao(Acao acao){
    switch (acao){
        case ACAO_NENHUMA:
            break;
        case ACAO_A10:
            printf("[AÇÃO]: Botão numérico pressionado, aguardando unidade.\n");
            break;
        case ACAO_A20:
            printf("[AÇÃO]: Botão numérico pressionado, aguardando confirmação.\n");
            break;
        case ACAO_A30:
            printf("[AÇÃO]: Botão C pressionado - Confirmação da umidade e entra em estado de monitoramento.\n");
            break;
        case ACAO_A35:
            printf("[AÇÃO]: Botão D pressionado - Volta para o estado de digito da unidade.\n");
            break;
        case ACAO_A40:
            printf("[AÇÃO]: Valores estáveis - Desativa os umidificadores e monitora o ambiente.\n");
            break;
        case ACAO_A50:
            printf("[AÇÃO]: Valores de umidade ABAIXO da tolerância - Ativa umidificador.\n");
            break;
        case ACAO_A55:
            printf("[AÇÃO]: Valores de umidade ACIMA da tolerância - Ativa desumidificador.\n");
            break;
        case ACAO_A60:
            printf("[AÇÃO]: Botão reset acionado - Retorna ao estado inicial.\n");
            break;
        default:
            break;
    }
}


// LCD

void atualizar_lcd_umidade(int digitos_lidos, int umidade, bool confirmado) {
    char linha0[32];
    char linha1[32];
    int umidade_segura = umidade % 100;

    if (confirmado) {
        snprintf(linha0, sizeof(linha0), "Monitorando...  ");
        snprintf(linha1, sizeof(linha1), "Atual: --.- %%   ");

        // LCD1: umidade alvo confirmada
        char alvo0[32];
        char alvo1[32];
        snprintf(alvo0, sizeof(alvo0), "Umidade Alvo:   "); // Exatos 16 caracteres
        snprintf(alvo1, sizeof(alvo1), "%02d %%            ", umidade_segura);
        lcd_set_cursor(LCD1_ADDR, 0, 0);
        lcd_send_string(LCD1_ADDR, alvo0);
        lcd_set_cursor(LCD1_ADDR, 1, 0);
        lcd_send_string(LCD1_ADDR, alvo1);
    } else {
        snprintf(linha0, sizeof(linha0), "Digite Umidade: "); // Exatos 16 caracteres
        
        if (digitos_lidos == 0) {
            snprintf(linha1, sizeof(linha1), "Val: [__] %%     ");
        } 
        else if (digitos_lidos == 1) {
            snprintf(linha1, sizeof(linha1), "Val: [%d_] %%     ", umidade_segura % 10);
        } 
        else {
            snprintf(linha1, sizeof(linha1), "Val: [%02d] %% OK? ", umidade_segura);
        }
    }

    lcd_set_cursor(LCD2_ADDR, 0, 0);
    lcd_send_string(LCD2_ADDR, linha0);
    lcd_set_cursor(LCD2_ADDR, 1, 0);
    lcd_send_string(LCD2_ADDR, linha1);
}

// Atualiza apenas a linha "Atual: XX.X %" no LCD2 com a média calculada da matriz
void atualizar_lcd_media_atual(float umidade_media) {
    char linha1[32];

    if (umidade_media < 0.0f) umidade_media = 0.0f;
    if (umidade_media > 100.0f) umidade_media = 100.0f;

    unsigned int inteiro = (unsigned int)umidade_media;
    unsigned int decimal = (unsigned int)((umidade_media - (float)inteiro) * 10.0f + 0.5f);
    if (decimal >= 10) {
        inteiro++;
        decimal = 0;
    }

    // Exatos 16 caracteres: "Atual:  54.3 %  " ou "Atual: 100.0 %  "
    snprintf(linha1, sizeof(linha1), "Atual: %3u.%u %%  ", inteiro % 101, decimal % 10);

    lcd_set_cursor(LCD2_ADDR, 1, 0);
    lcd_send_string(LCD2_ADDR, linha1);
}

// preenchimento da matriz com a umidade interpolada pelos 4 dhts

float simularUmidade(float** matriz, int n, coordMatriz extr[4], float leituras_dht[4]){
    if (matriz == NULL || n <= 0){
        return 0.0f;
    }

    double soma_total = 0.0;
    for (int i = 0; i < n; i++){
        if (matriz[i] == NULL){
            continue;
        }
        for (int j = 0; j < n; j++){
            float soma_pesos = 0.0f;
            float soma_ponderada = 0.0f;
            bool ponto_exato = false;

            for (int k = 0; k < 4; k++){
                int di = i - extr[k].i;
                int dj = j - extr[k].j;
                int dist2 = (di * di) + (dj * dj);

                if (dist2 == 0){
                    matriz[i][j] = leituras_dht[k];
                    ponto_exato = true;
                    break;
                }

                float peso = 1.0f/(float)dist2;
                soma_ponderada += peso * leituras_dht[k];
                soma_pesos += peso;

            }
            if (!ponto_exato && soma_pesos > 0.0f){
                matriz[i][j] = soma_ponderada / soma_pesos;
            }
            soma_total += matriz[i][j];
        }
        if (i % 50 == 0){
            vTaskDelay(1);
        }
    }

    return (float)(soma_total / ((double)n*(double)n));
}




void app_main(void) {
    // INICIALIZAÇÃO DAS FUNÇÕES DOS COMPONENTES DE HARDWARE

    init_pwm_leds();
    init_adc_pots();
    init_i2c_lcd();
    init_keypad();

    // --------------------------------------------------

    // Declaração da matriz da área de atuação
    
    int i = tamMatriz;
    int j = tamMatriz;
    float** matriz_area_atuacao = criarMatriz(i, j);



    // --------------------------------------------------


    // verifica o tamanho da matriz e onde são os extremos (utilizados para calcular a distancia euclidiana)

    coordMatriz extr[4];
    inicializarExtremidades(extr);
    for(int i = 0; i < 4; i++){
        printf("Extremidade %d: %s (i: %d, j: %d)\n", i, extr[i].nome, extr[i].i, extr[i].j);
    }

    // --------------------------------------------------

    lcd_init(LCD1_ADDR);
    lcd_init(LCD2_ADDR);

    
    //atualizar_lcd_umidade(0, 0, false); // mostra "Valor: [__] C" logo ao ligar

    // teclado numérico
    int umidade = 0; // guarda o numero que esta sendo digitado
    int digitos_lidos = 0; // conta quantos digitos foram inseridos (máx 2)
    int umidade_alvo = 25; // umidade de valor inicial
    Estado estado_atual = ESTADO_DEZENA; // estado inicial
    int opcao;
    Evento evento;
    Transicao t;

    int pot_values[10];


    // DHT
    const int pinosDHT[4] = {19, 20, 21, 0};
    float temperaturas_dht[4] = {0};
    float umidades_dht[4] = {0};
    float diferenca_dht[4] = {0};

    int contadorDHT = 40; // inicialização dos contadores em 40 para fazer a primeira leitura na inicialização

    while (1) {
        char tecla = ler_teclado();
        if (estado_atual == ESTADO_DEZENA || estado_atual == ESTADO_UNIDADE || estado_atual == ESTADO_CONFIRMACAO){
            atualizar_lcd_umidade(digitos_lidos, umidade, false);
            if (tecla != '\0'){
                if (tecla >= '0' && tecla <= '9'){
                    if (digitos_lidos < 2){
                        opcao = 0; // força a mudança de estado para ESTADO_UNIDADE
                        evento = (Evento)opcao;
                        t = tabela_transicao[estado_atual][evento];
                        estado_atual = t.proximo_estado;
                        executar_acao(t.acao_a_executar);
                        umidade = (umidade * 10) + (tecla - '0');
                        digitos_lidos++;
                        atualizar_lcd_umidade(digitos_lidos, umidade, false);
                        
                    } else{
                        printf("Limite de 2 digitos atingido (%d)! Pressione 'C' para confirmar ou 'D' para cancelar.\n", umidade);
                    }
                }

                else if (tecla == 'D'){
                    opcao = 1; // força a mudança de estado para ESTADO_DEZENA
                    evento = (Evento)opcao;
                    t = tabela_transicao[estado_atual][evento];
                    estado_atual = t.proximo_estado;
                    executar_acao(t.acao_a_executar);
                    umidade = 0;
                    digitos_lidos = 0;
                    atualizar_lcd_umidade(digitos_lidos, umidade, false);
                }
                
                else if (tecla == 'C'){
                    if (digitos_lidos > 0){
                        opcao = 2; // força a mudança de estado para ESTADO_MONITORANDO
                        evento = (Evento)opcao;
                        t = tabela_transicao[estado_atual][evento];
                        estado_atual = t.proximo_estado;
                        executar_acao(t.acao_a_executar);
                        umidade_alvo = umidade;
                        atualizar_lcd_umidade(digitos_lidos, umidade, true);
                        umidade = 0;
                        digitos_lidos = 0;
                    }
                }

            }
        }
        // --------------------------------------------
        
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


        // CONFIGURAÇÃO DO DHT NO MAIN
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
                float umidade_media = simularUmidade(matriz_area_atuacao, tamMatriz, extr, umidades_dht);
                printf("DHTs: [%.1f%%, %.1f%%, %.1f%%, %.1f%%] | Media Matriz: %.1f%%\n",
                       umidades_dht[0], umidades_dht[1], umidades_dht[2], umidades_dht[3], umidade_media);

                if (estado_atual != ESTADO_DEZENA &&
                    estado_atual != ESTADO_UNIDADE &&
                    estado_atual != ESTADO_CONFIRMACAO) {
                    atualizar_lcd_media_atual(umidade_media);
                    calcularDiferencasDHT(umidades_dht, umidade_alvo, diferenca_dht);
                }
            }
        }


        // --------------------------------------------
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}