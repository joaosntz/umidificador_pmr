#ifndef FUNCOES_H
#define FUNCOES_H
/*
TODO:

[ x ] Função de criação de matriz M x N, onde serão alocados os sensores e umidificadores
[ x ] Função para liberar a matriz alocada na finalização do programa (lembrando de definir a matriz = nullptr após liberar para não dar erro de memória)
[  ] Criar função que receba a matriz e o número de sensores e umidificadores, e retorne a matriz com os valores lidos dos sensores
[  ] Criar função que recebe a matriz com os valores lidos dos sensores e consiga fazer a discrepância, criando um mapa de erro entre os sensores

*/

float** criarMatriz(int m, int n);
void liberarMatriz(float** matriz, int m);
void lerSensores(float** matriz, int m, int n);
void mapaErro(float** matriz, int m, int n);

#endif