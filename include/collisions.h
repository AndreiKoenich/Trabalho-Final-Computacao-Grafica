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
#define VELOCIDADE_BALAS 6
#define QUANTIDADE_BALAS 50
#define QUANTIDADE_OBJETOS 12
#define VELOCIDADE_ESFERAS 5
#define RAIO_ESFERAS 0.5
#define VELOCIDADE_ALVOS 3
#define ALTURA_BALAS 0.8

typedef struct
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float velocidade_alvo = VELOCIDADE_ALVOS;
    float espessura = ESPESSURA_ALVOS;

    glm::vec4 bbox_minimo;
    glm::vec4 bbox_maximo;

    int direcao = DIRECAO_DIREITA;
    int dano = 0;

} Alvo;

typedef struct
{
    float centro_x = 0.0f;
    float centro_y = 0.0f;
    float centro_z = 0.0f;
    float velocidade_esfera = VELOCIDADE_ESFERAS;

    int direcao = DIRECAO_DIREITA;
    int dano = 0;

} Esfera;

typedef struct
{
    float x = 0.0;
    float y = ALTURA_BALAS;
    float z = 0.0;
    glm::vec4 direcao = glm::vec4(0.0f,0.0f,0.0f,0.0f);
    glm::vec4 eixo_rotacao_normalizado = glm::vec4(0.0f,0.0f,0.0f,0.0f);
    float angulo_rotacao;
    bool desenhar = false;

} Bala;

typedef struct
{
    float x = 0.0;
    float y = 0.0;
    float z = 0.0;

    glm::vec4 bbox_minimo;
    glm::vec4 bbox_maximo;

} ObjetoCenario;



/* Função com teste de colisão ponto-cubo, responsável por impedir que um alvo seja desenhado, caso seja acertado por uma bala. */
void destroi_alvos(Bala vetor_balas[], Alvo vetor_alvos[]);

/* Função com teste de colisão ponto-cubo, responsável por impedir que uma bala seja desenhada, caso atinja um objeto do cenário.*/
void destroi_balas(Bala vetor_balas[], ObjetoCenario vetor_objetos[]);

/* Função com teste de colisão ponto-esfera, responsável por verificar se uma esfera do cenário deve ser destruída.*/
void destroi_esferas(Bala vetor_balas[], Esfera vetor_esferas[]);

/* Função com teste de colisão cubo-plano para um plano em que X é constante (como não se usa o valor y de posição, o teste na verdade é de quadrado-plano */
/* Foram separadas em 2 funções para X e Z, para que o jogador pudesse "deslizar" no outro sentido caso desse colisão com 1 das paredes */
/* Caso contrário, se ele colodisse com o eixo Z por exemplo, ele não poderia se mover em X */
/* Como então as funções são separadas, cada função é uma função de colisão linha-plano*/
bool limita_jogador_plano_x(glm::vec4 bbox_jogador_max, glm::vec4 bbox_jogador_min, float x);

/* Função com teste de colisão cubo-plano para um plano em que Z é constante (como não se usa o valor y de posição, o teste na verdade é de quadrado-plano) */
/* Como as funções são separadas entre X e Z, cada função é uma função de colisão linha-plano*/
bool limita_jogador_plano_z(glm::vec4 bbox_jogador_max, glm::vec4 bbox_jogador_min, float z);
