#include "collisions.h"

#include <cstdio>
#include <cstdlib>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#define LIMITE_ESQ_ALVO -10.0
#define LIMITE_DIR_ALVO 10.0
#define LIMITE_ESQ_ESFERA -10.0
#define LIMITE_DIR_ESFERA 10.0
#define DIRECAO_ESQUERDA 0
#define DIRECAO_DIREITA 1
#define ESPESSURA_ALVOS 0.50
#define QUANTIDADE_ALVOS 15
#define MAXIMO_DANO 1
#define NUM_ALVOS_NAOLINEARES 11
#define VELOCIDADE_ALVOS 3
#define VELOCIDADE_BALAS 6
#define QUANTIDADE_BALAS 50
#define QUANTIDADE_OBJETOS 12
#define VELOCIDADE_ESFERAS 5
#define RAIO_ESFERAS 0.5
#define ALTURA_BALAS 0.8


/* Função com teste de colisão ponto-cubo, responsável por impedir que um alvo seja desenhado, caso seja acertado por uma bala. */
void destroi_alvos(Bala vetor_balas[], Alvo vetor_alvos[])
{
    for (int i = 0; i < QUANTIDADE_BALAS; i++)
    {
        for (int j = 0; j < QUANTIDADE_ALVOS; j++)
        {
            if (vetor_balas[i].desenhar == true && vetor_alvos[j].dano < MAXIMO_DANO)
            {
                if (vetor_balas[i].x >= vetor_alvos[j].bbox_minimo.x &&
                    vetor_balas[i].x <= vetor_alvos[j].bbox_maximo.x &&
                    vetor_balas[i].y >= vetor_alvos[j].bbox_minimo.y &&
                    vetor_balas[i].y <= vetor_alvos[j].bbox_maximo.y &&
                    vetor_balas[i].z <= vetor_alvos[j].bbox_maximo.z &&
                    vetor_balas[i].z >= vetor_alvos[j].bbox_maximo.z-ESPESSURA_ALVOS)
                    {
                        vetor_alvos[j].dano += 1;
                        vetor_balas[i].desenhar = false;
                    }
            }
        }
    }
}

/* Função com teste de colisão ponto-cubo, responsável por impedir que uma bala seja desenhada, caso atinja um objeto do cenário.*/
void destroi_balas(Bala vetor_balas[], ObjetoCenario vetor_objetos[]){
    for (int i = 0; i < QUANTIDADE_BALAS; i++)
    {
        for (int j = 0; j < QUANTIDADE_OBJETOS; j++)
        {
            if (vetor_balas[i].desenhar == true)
            {
                if (vetor_balas[i].x >= vetor_objetos[j].bbox_minimo.x &&
                    vetor_balas[i].x <= vetor_objetos[j].bbox_maximo.x &&
                    vetor_balas[i].y >= vetor_objetos[j].bbox_minimo.y &&
                    vetor_balas[i].y <= vetor_objetos[j].bbox_maximo.y &&
                    vetor_balas[i].z >= vetor_objetos[j].bbox_minimo.z &&
                    vetor_balas[i].z <= vetor_objetos[j].bbox_maximo.z)
                    {
                        vetor_balas[i].desenhar = false;
                        vetor_balas[i].x = 0.0;
                        vetor_balas[i].y = 0.0;
                        vetor_balas[i].z = 0.0;
                    }
            }
        }
    }
}

/* Função com teste de colisão ponto-esfera, responsável por verificar se uma esfera do cenário deve ser destruída.*/
void destroi_esferas(Bala vetor_balas[], Esfera vetor_esferas[])
{
    for (int i = 0; i < QUANTIDADE_BALAS; i++)
    {
        for (int j = 0; j < QUANTIDADE_ALVOS; j++)
        {
            if (vetor_balas[i].desenhar == true && vetor_esferas[j].dano < MAXIMO_DANO)
            {
                if (vetor_balas[i].x >= vetor_esferas[j].centro_x-RAIO_ESFERAS &&
                    vetor_balas[i].x <= vetor_esferas[j].centro_x+RAIO_ESFERAS &&
                    vetor_balas[i].y >= vetor_esferas[j].centro_y-RAIO_ESFERAS &&
                    vetor_balas[i].y <= vetor_esferas[j].centro_y+RAIO_ESFERAS &&
                    vetor_balas[i].z <= vetor_esferas[j].centro_z+RAIO_ESFERAS &&
                    vetor_balas[i].z >= vetor_esferas[j].centro_z-RAIO_ESFERAS)
                    {
                        vetor_esferas[j].dano += 1;
                        vetor_balas[i].desenhar = false;
                    }
            }
        }
    }
}

/* Quis-se fazer colisão quadrado-plano para um plano em que X é constante (como não se usa o valor y de posição, o teste é quadrado-plano) */
/* Como as funções são separadas entre X e Z, cada função é uma função de colisão LINHA-PLANO*/
bool limita_jogador_plano_x(glm::vec4 bbox_jogador_max, glm::vec4 bbox_jogador_min, float x){
    if(bbox_jogador_max.x >= x && bbox_jogador_min.x <= x){
        return true;
    }
    return false;
}

/* Quis-se fazer colisão quadrado-plano para um plano em que Z é constante (como não se usa o valor y de posição, o teste é quadrado-plano) */
/* Como as funções são separadas entre X e Z, cada função é uma função de colisão LINHA-PLANO*/
bool limita_jogador_plano_z(glm::vec4 bbox_jogador_max, glm::vec4 bbox_jogador_min, float z){
    if(bbox_jogador_max.z >= z && bbox_jogador_min.z <= z){
        return true;
    }
    return false;
}
