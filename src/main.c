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

    int min_borda = 0; //tamMatriz * 0.02;
    int max_borda = tamMatriz - 1;
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
            printf("[AÇÃO]: Botão C pressionado - Confirmação da temperatura e entra em estado de monitoramento.\n");
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

void atualizar_lcd_temperatura(int digitos_lidos, int temp, bool confirmado) {
    char linha0[32];
    char linha1[32];
    int temp_segura = temp % 100;

    if (confirmado) {
        snprintf(linha0, sizeof(linha0), "Monitorando...  ");
        snprintf(linha1, sizeof(linha1), "Atual: --.- C   ");

        // LCD2: temperatura alvo
        char alvo0[32];
        char alvo1[32];
        snprintf(alvo0, sizeof(alvo0), "Temp. Alvo:     ");
        snprintf(alvo1, sizeof(alvo1), "%02d C            ", temp_segura);
        lcd_set_cursor(LCD1_ADDR, 0, 0);
        lcd_send_string(LCD1_ADDR, alvo0);
        lcd_set_cursor(LCD1_ADDR, 1, 0);
        lcd_send_string(LCD1_ADDR, alvo1);
    } else {
        snprintf(linha0, sizeof(linha0), "Digite a Temp:  ");
        
        if (digitos_lidos == 0) {
            snprintf(linha1, sizeof(linha1), "Val: [__] C     ");
        } 
        else if (digitos_lidos == 1) {
            snprintf(linha1, sizeof(linha1), "Val: [%d_] C     ", temp_segura % 10);
        } 
        else {
            snprintf(linha1, sizeof(linha1), "Val: [%02d] C OK? ", temp_segura);
        }
    }

    lcd_set_cursor(LCD2_ADDR, 0, 0);
    lcd_send_string(LCD2_ADDR, linha0);
    lcd_set_cursor(LCD2_ADDR, 1, 0);
    lcd_send_string(LCD2_ADDR, linha1);
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

    coordMatriz extr[5];
    inicializarExtremidades(extr);
    for(int i = 0; i < 5; i++){
        printf("Extremidade %d: %s (i: %d, j: %d)\n", i, extr[i].nome, extr[i].i, extr[i].j);
    }

    // --------------------------------------------------

    lcd_init(LCD1_ADDR);
    lcd_init(LCD2_ADDR);

    
    //atualizar_lcd_temperatura(0, 0, false); // mostra "Valor: [__] C" logo ao ligar

    // teclado numérico
    int temp = 0; // guarda o numero que esta sendo digitado
    int digitos_lidos = 0; // conta quantos digitos foram inseridos (máx 2)
    int temperatura_alvo = 25; // temperatura de valor inicial
    Estado estado_atual = ESTADO_DEZENA; // estado inicial
    int opcao;
    Evento evento;
    Transicao t;

    int pot_values[10];

    while (1) {
        char tecla = ler_teclado();
        if (estado_atual == ESTADO_DEZENA || estado_atual == ESTADO_UNIDADE || estado_atual == ESTADO_CONFIRMACAO){
            atualizar_lcd_temperatura(digitos_lidos, temp, false);
            if (tecla != '\0'){
                if (tecla >= '0' && tecla <= '9'){
                    if (digitos_lidos < 2){
                        opcao = 0; // força a mudança de estado para ESTADO_UNIDADE
                        evento = (Evento)opcao;
                        t = tabela_transicao[estado_atual][evento];
                        estado_atual = t.proximo_estado;
                        executar_acao(t.acao_a_executar);
                        temp = (temp * 10) + (tecla - '0');
                        digitos_lidos++;
                        atualizar_lcd_temperatura(digitos_lidos, temp, false);
                        
                    } else{
                        printf("Limite de 2 digitos atingido (%d)! Pressione 'C' para confirmar ou 'D' para cancelar.\n", temp);
                    }
                }

                else if (tecla == 'D'){
                    opcao = 1; // força a mudança de estado para ESTADO_DEZENA
                    evento = (Evento)opcao;
                    t = tabela_transicao[estado_atual][evento];
                    estado_atual = t.proximo_estado;
                    executar_acao(t.acao_a_executar);
                    temp = 0;
                    digitos_lidos = 0;
                    atualizar_lcd_temperatura(digitos_lidos, temp, false);
                }
                
                else if (tecla == 'C'){
                    if (digitos_lidos > 0){
                        opcao = 2; // força a mudança de estado para ESTADO_MONITORANDO
                        evento = (Evento)opcao;
                        t = tabela_transicao[estado_atual][evento];
                        estado_atual = t.proximo_estado;
                        executar_acao(t.acao_a_executar);
                        temperatura_alvo = temp;
                        atualizar_lcd_temperatura(digitos_lidos, temp, true);
                        temp = 0;
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
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}