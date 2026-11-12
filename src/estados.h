#ifndef ESTADOS_H
#define ESTADOS_H
typedef enum{
    ESTADO_DEZENA,
    ESTADO_UNIDADE,
    ESTADO_CONFIRMACAO,
    ESTADO_ATIVOS,
    ESTADO_MONITORANDO,
    NUM_ESTADOS
} Estado;

typedef enum{
    EVENTO_TECLA_NUMERICA,
    EVENTO_TECLA_D,
    EVENTO_TECLA_C,
    EVENTO_UMIDADE_ESTAVEL,
    EVENTO_UMIDADE_FORA,
    EVENTO_RESET,
    NUM_EVENTOS
} Evento;

typedef enum{
    ACAO_NENHUMA, // nao faz nada, apenas muda de estado
    ACAO_A01, // botao numerico foi pressionado e aguarda unidade
    ACAO_A02, // botão numerico foi pressionado e espera confirmação
    ACAO_A03, // botão c pressionado - confirma a temperatura e entra em estado de ativo se necessário ou monitoramento
    ACAO_A04, // valores estaveis - desativa os umidificadores e monitora o ambiente
    ACAO_A05, // valores de umidade fora da tolerancia - ativa umidificador
    ACAO_A06 // botao reset acionado
} Acao;

typedef struct{
    Acao acao_a_executar;
    Estado proximo_estado;
} Transicao;

const Transicao tabela_transicao[NUM_ESTADOS][NUM_EVENTOS] = {
    [ESTADO_DEZENA] = {
        [EVENTO_TECLA_NUMERICA] = {ACAO_A01, ESTADO_UNIDADE},
        [EVENTO_TECLA_D] = {ACAO_NENHUMA, ESTADO_DEZENA},
        [EVENTO_TECLA_C] = {ACAO_NENHUMA, ESTADO_DEZENA},
        [EVENTO_UMIDADE_ESTAVEL] = {ACAO_NENHUMA, ESTADO_DEZENA},
        [EVENTO_UMIDADE_FORA] = {ACAO_NENHUMA, ESTADO_DEZENA},
        [EVENTO_RESET] = {ACAO_NENHUMA, ESTADO_DEZENA}
    },
    [ESTADO_UNIDADE] = {
        [EVENTO_TECLA_NUMERICA] = {ACAO_A02, ESTADO_CONFIRMACAO},
        [EVENTO_TECLA_D] = {ACAO_NENHUMA, ESTADO_UNIDADE},
        [EVENTO_TECLA_C] = {ACAO_NENHUMA, ESTADO_UNIDADE},
        [EVENTO_UMIDADE_ESTAVEL] = {ACAO_NENHUMA, ESTADO_UNIDADE},
        [EVENTO_UMIDADE_FORA] = {ACAO_NENHUMA, ESTADO_UNIDADE},
        [EVENTO_RESET] = {ACAO_A06, ESTADO_DEZENA}
    },
    [ESTADO_CONFIRMACAO] = {
        [EVENTO_TECLA_NUMERICA] = {ACAO_NENHUMA, ESTADO_CONFIRMACAO},
        [EVENTO_TECLA_D] = {ACAO_NENHUMA, ESTADO_UNIDADE},
        [EVENTO_TECLA_C] = {ACAO_A03, ESTADO_ATIVOS},
        [EVENTO_UMIDADE_ESTAVEL] = {ACAO_NENHUMA, ESTADO_CONFIRMACAO},
        [EVENTO_UMIDADE_FORA] = {ACAO_NENHUMA, ESTADO_CONFIRMACAO},
        [EVENTO_RESET] = {ACAO_A06, ESTADO_DEZENA}
    },
    [ESTADO_ATIVOS] = {
        [EVENTO_UMIDADE_ESTAVEL] = {ACAO_A04, ESTADO_MONITORANDO},
        [EVENTO_UMIDADE_FORA] = {ACAO_A05, ESTADO_ATIVOS},
        [EVENTO_RESET] = {ACAO_A06, ESTADO_DEZENA},
    }
};

#endif