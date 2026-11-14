#ifndef ESTADOS_H
#define ESTADOS_H
typedef enum{
    ESTADO_DEZENA,
    ESTADO_UNIDADE,
    ESTADO_CONFIRMACAO,
    ESTADO_U_ATIVOS,
    ESTADO_D_ATIVOS,
    ESTADO_MONITORANDO,
    NUM_ESTADOS
} Estado;

typedef enum{
    EVENTO_TECLA_NUMERICA = 0,
    EVENTO_TECLA_D, // 1
    EVENTO_TECLA_C, // 2
    EVENTO_UMIDADE_ESTAVEL, // 3
    EVENTO_UMIDADE_ABAIXO, // 4 
    EVENTO_UMIDADE_ACIMA, // 5
    EVENTO_RESET, // 6
    NUM_EVENTOS
} Evento;

typedef enum{
    ACAO_NENHUMA, // nao faz nada, apenas muda de estado
    ACAO_A10, // botao numerico foi pressionado e aguarda unidade
    ACAO_A20, // botão numerico foi pressionado e espera confirmação
    ACAO_A30, // botão c pressionado - confirma a temperatura e entra em estado de ativo se necessário ou monitoramento
    ACAO_A35, // botao d pressionado - volta para o estado de digito da unidade
    ACAO_A40, // valores estaveis - desativa os umidificadores e monitora o ambiente
    ACAO_A50, // valores de umidade ABAIXO da tolerancia - ativa umidificador
    ACAO_A55, // valores de umidade ACIMA da tolerancia - ativa desumidificador
    ACAO_A60 // botao reset acionado
} Acao;

typedef struct{
    Acao acao_a_executar;
    Estado proximo_estado;
} Transicao;

const Transicao tabela_transicao[NUM_ESTADOS][NUM_EVENTOS] = {
    [ESTADO_DEZENA] = {
        [EVENTO_TECLA_NUMERICA] = {ACAO_A10, ESTADO_UNIDADE},
        [EVENTO_TECLA_D] = {ACAO_NENHUMA, ESTADO_DEZENA},
        [EVENTO_TECLA_C] = {ACAO_NENHUMA, ESTADO_DEZENA},
        [EVENTO_UMIDADE_ESTAVEL] = {ACAO_NENHUMA, ESTADO_DEZENA},
        [EVENTO_UMIDADE_ABAIXO] = {ACAO_NENHUMA, ESTADO_DEZENA},
        [EVENTO_UMIDADE_ACIMA] = {ACAO_NENHUMA, ESTADO_DEZENA},
        [EVENTO_RESET] = {ACAO_NENHUMA, ESTADO_DEZENA}
    },
    [ESTADO_UNIDADE] = {
        [EVENTO_TECLA_NUMERICA] = {ACAO_A20, ESTADO_CONFIRMACAO},
        [EVENTO_TECLA_D] = {ACAO_NENHUMA, ESTADO_UNIDADE},
        [EVENTO_TECLA_C] = {ACAO_NENHUMA, ESTADO_UNIDADE},
        [EVENTO_UMIDADE_ESTAVEL] = {ACAO_NENHUMA, ESTADO_UNIDADE},
        [EVENTO_UMIDADE_ABAIXO] = {ACAO_NENHUMA, ESTADO_UNIDADE},
        [EVENTO_UMIDADE_ACIMA] = {ACAO_NENHUMA, ESTADO_UNIDADE},
        [EVENTO_RESET] = {ACAO_A60, ESTADO_DEZENA}
    },
    [ESTADO_CONFIRMACAO] = {
        [EVENTO_TECLA_NUMERICA] = {ACAO_NENHUMA, ESTADO_CONFIRMACAO},
        [EVENTO_TECLA_D] = {ACAO_A35, ESTADO_UNIDADE},
        [EVENTO_TECLA_C] = {ACAO_A30, ESTADO_MONITORANDO},
        [EVENTO_UMIDADE_ESTAVEL] = {ACAO_NENHUMA, ESTADO_CONFIRMACAO},
        [EVENTO_UMIDADE_ABAIXO] = {ACAO_NENHUMA, ESTADO_CONFIRMACAO},
        [EVENTO_UMIDADE_ACIMA] = {ACAO_NENHUMA, ESTADO_CONFIRMACAO},
        [EVENTO_RESET] = {ACAO_A60, ESTADO_DEZENA}
    },

    [ESTADO_U_ATIVOS] = {
        [EVENTO_UMIDADE_ESTAVEL] = {ACAO_A40, ESTADO_MONITORANDO},
        [EVENTO_UMIDADE_ABAIXO] = {ACAO_A50, ESTADO_U_ATIVOS},
        [EVENTO_UMIDADE_ACIMA] = {ACAO_A55, ESTADO_D_ATIVOS},
        [EVENTO_RESET] = {ACAO_A60, ESTADO_DEZENA},
    }
};

#endif