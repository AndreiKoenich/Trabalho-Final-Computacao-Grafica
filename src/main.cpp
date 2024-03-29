//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   LABORATÓRIO 5
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

#include "collisions.h"

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextReFndering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 vF, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 34.6f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 2.5f; // Distância da câmera para a origem

/* NOVAS VARIAVEIS GLOBAIS ABAIXO */

bool iniciar_jogo = false;
bool fim_jogo = false;

float r = g_CameraDistance;
float y = 0.0f;
float z = 0.0f;
float x = 0.0f;

glm::vec4 camera_position_c  = glm::vec4(0.0f,0.55f,4.5f,1.0f); // Ponto "c", centro da câmera
glm::vec4 camera_view_vector = glm::vec4(0.0f,0.0f,0.0f,0.0f); // Vetor "view", sentido para onde a câmera está virada
glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eixo Y global)
glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando

glm::vec4 w = negative_vector(camera_view_vector)/norm(camera_view_vector);/* PREENCHA AQUI o cálculo do vetor w */;
glm::vec4 u = crossproduct(camera_up_vector,w)/norm(crossproduct(camera_up_vector,w)); /* PREENCHA AQUI o cálculo do vetor u */;

int tecla_W_pressionada = 0;
int tecla_A_pressionada = 0;
int tecla_S_pressionada = 0;
int tecla_D_pressionada = 0;

#define VELOCIDADE_CAMERA 2.5
#define ESPESSURA_JOGADOR 0.25

#define LIMITE_ESQUERDA -15.0
#define LIMITE_DIREITA 15.0
#define LIMITE_FUNDO -15.0
#define LIMITE_FRENTE 15.0
#define LIMITE_CIMA 15.0
#define LIMITE_BAIXO -15.0

#define LIMITE_ESQ_ALVO -10.0
#define LIMITE_DIR_ALVO 10.0

#define LIMITE_ESQ_ESFERA -10.0
#define LIMITE_DIR_ESFERA 10.0

#define DIRECAO_ESQUERDA 0
#define DIRECAO_DIREITA 1
#define ESPESSURA_ALVOS 0.50
#define VELOCIDADE_ALVOS 3
#define QUANTIDADE_ALVOS 15
#define MAXIMO_DANO 1

#define TEMPO_ALVO_BEZIER 2
#define NUM_ALVOS_NAOLINEARES 11

#define VELOCIDADE_BALAS 6
#define QUANTIDADE_BALAS 50
#define ALTURA_BALAS 0.8

#define QUANTIDADE_OBJETOS 12

#define TARGET_FPS 60

#define ALTURA_ESFERAS 5.0
#define QUANTIDADE_ESFERAS 4
#define VELOCIDADE_ESFERAS 5
#define RAIO_ESFERAS 0.5

#define PLANE 0
#define ALVO 1
#define ARMA 2
#define BULLET 3
#define SKYBOX 4
#define MIRA 255
#define TELA_INICIO 5
#define TROFEU 6
#define SKYBOX_TROFEU 7
#define CAIXA 8
#define BARREIRAS 9
#define PALETE 10
#define ESFERA 11
/* NOVAS VARIÁVEIS GLOBAIS ABAIXO. */

double t_now;
double t_prev;
double delta_t;

glm::vec4 bbox_minimo_novo = glm::vec4(0.0f,0.0f,0.0f,0.0f);
glm::vec4 bbox_maximo_novo = glm::vec4(0.0f,0.0f,0.0f,0.0f);

/* NOVAS VARIAVEIS GLOBAIS ACIMA */

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

/* NOVAS FUNÇÕES DO TRABALHO FINAL ABAIXO */

void tela_inicio()
{
    glm::mat4 view = Matrix_Identity();
    glm::mat4 projection = Matrix_Identity();
    glm::mat4 model = Matrix_Rotate_X(1.570796237f);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
    glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));
    glUniform1i(g_object_id_uniform, TELA_INICIO);
    DrawVirtualObject("the_plane");
}

void inicializa_alvos(Alvo vetor_alvos[])
{
    vetor_alvos[0].x = 0.0f;
    vetor_alvos[0].y = 0.5f;
    vetor_alvos[0].z = 0.0f;

    vetor_alvos[1].x = -8.0f;
    vetor_alvos[1].y = 0.5f;
    vetor_alvos[1].z = 6.0f;

    vetor_alvos[2].x = -8.0f;
    vetor_alvos[2].y = 0.5f;
    vetor_alvos[2].z = 8.0f;

    vetor_alvos[3].x = 3.2f;
    vetor_alvos[3].y = 0.95f;
    vetor_alvos[3].z = 11.5f;

    vetor_alvos[4].x = 1.2f;
    vetor_alvos[4].y = 0.95f;
    vetor_alvos[4].z = 11.5f;

    vetor_alvos[5].x = -1.2f;
    vetor_alvos[5].y = 0.95f;
    vetor_alvos[5].z = 11.5f;

    vetor_alvos[6].x = -3.2f;
    vetor_alvos[6].y = 0.95f;
    vetor_alvos[6].z = 11.5f;

    vetor_alvos[7].x = -2.8f;
    vetor_alvos[7].y = 0.5f;
    vetor_alvos[7].z = 1.0f;

    vetor_alvos[8].x = -1.0f;
    vetor_alvos[8].y = 0.5f;
    vetor_alvos[8].z = -1.9f;

    vetor_alvos[9].x = 0.1f;
    vetor_alvos[9].y = 0.5f;
    vetor_alvos[9].z = -1.9f;

    vetor_alvos[10].x = 2.4f;
    vetor_alvos[10].y = 0.5f;
    vetor_alvos[10].z = -1.6f;

     for (int i = NUM_ALVOS_NAOLINEARES; i < QUANTIDADE_ALVOS; i++)
    {
        if (i % 2 == 0)
            vetor_alvos[i].direcao = DIRECAO_DIREITA;
        else
            vetor_alvos[i].direcao = DIRECAO_ESQUERDA;
        vetor_alvos[i].x = 0.0f;
        vetor_alvos[i].y = 0.5f;
        vetor_alvos[i].z = -3.0f*(i-NUM_ALVOS_NAOLINEARES);
    }
}

void desenha_chao()
{
    // Desenhamos o plano do chão
    glm::mat4 model = Matrix_Translate(0.0f,0.0f,0.0f)*Matrix_Scale(20.0f,5.0f,20.0f);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, PLANE);
    DrawVirtualObject("the_plane");
}

void desenha_alvos(Alvo vetor_alvos[])
{
    glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem
    for (int i = 0; i < QUANTIDADE_ALVOS; i++)
    {
        if (vetor_alvos[i].dano < MAXIMO_DANO)
        {
            model = Matrix_Translate(vetor_alvos[i].x,vetor_alvos[i].y,vetor_alvos[i].z)*Matrix_Scale(0.1f,0.1f,0.01f);
            glm::mat4 model_scale = Matrix_Scale(0.1f,0.1f,0.01f);
            if(i == 0 || (i>=3 && i<=6)){
                model = model * Matrix_Rotate_Y(3.14);
            }
            else if(i == 1 || i== 2){
                model =  model * Matrix_Rotate_Y(1.57) * Matrix_Scale(10.0f, 1.0f,0.8f);
                model_scale = Matrix_Scale(0.1f,0.1f,0.08f);
            }
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ALVO);
            DrawVirtualObject("Cube");


            vetor_alvos[i].bbox_minimo = model_scale*bbox_minimo_novo;
            vetor_alvos[i].bbox_maximo = model_scale*bbox_maximo_novo;

            vetor_alvos[i].bbox_minimo.x += vetor_alvos[i].x;
            vetor_alvos[i].bbox_minimo.y += vetor_alvos[i].y;
            vetor_alvos[i].bbox_minimo.z += vetor_alvos[i].z;

            vetor_alvos[i].bbox_maximo.x += vetor_alvos[i].x;
            vetor_alvos[i].bbox_maximo.y += vetor_alvos[i].y;
            vetor_alvos[i].bbox_maximo.z += vetor_alvos[i].z;

        }
    }
}

void dispara_balas(Bala vetor_balas[])
{
    for (int i = 0; i < QUANTIDADE_BALAS; i++)
        if (vetor_balas[i].desenhar == false)
        {
            vetor_balas[i].desenhar = true;
            vetor_balas[i].x = camera_position_c.x;
            vetor_balas[i].y = camera_position_c.y;
            vetor_balas[i].z = camera_position_c.z;

            vetor_balas[i].direcao.x = camera_view_vector.x;
            vetor_balas[i].direcao.y = camera_view_vector.y;
            vetor_balas[i].direcao.z = camera_view_vector.z;

            glm::vec4 eixo_rotacao = crossproduct(glm::vec4(0.0f,1.0f,0.0f,0.0f),camera_view_vector);
            vetor_balas[i].eixo_rotacao_normalizado = normalize(eixo_rotacao);
            glm::vec4 view_normalizado = normalize(camera_view_vector);
            float cosseno_rotacao = dotproduct(view_normalizado,glm::vec4(0.0f,1.0f,0.0f,0.0f));

            vetor_balas[i].angulo_rotacao = acosf(cosseno_rotacao);
            break;
        }
}

void desenha_balas(Bala vetor_balas[])
{
    glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem
    for (int i = 0; i < QUANTIDADE_BALAS; i++)
    {
        if (vetor_balas[i].desenhar == true)
        {
            vetor_balas[i].x += VELOCIDADE_BALAS*vetor_balas[i].direcao.x*delta_t;
            vetor_balas[i].y += VELOCIDADE_BALAS*vetor_balas[i].direcao.y*delta_t;
            vetor_balas[i].z += VELOCIDADE_BALAS*vetor_balas[i].direcao.z*delta_t;

            model = Matrix_Translate(vetor_balas[i].x,vetor_balas[i].y,vetor_balas[i].z)
            *Matrix_Scale(0.03f,0.03f,0.03f)
            *Matrix_Rotate(vetor_balas[i].angulo_rotacao,vetor_balas[i].eixo_rotacao_normalizado);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, BULLET);
            DrawVirtualObject("Bullet");
        }
    }
}

void desenha_skybox(int id_objeto)
{
    glm::mat4 model;
    model = Matrix_Scale(15.0f,15.0f,15.0f);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, id_objeto);
    glDisable(GL_CULL_FACE);
    DrawVirtualObject("the_sphere");
    glEnable(GL_CULL_FACE);
}

void desenha_caixas(ObjetoCenario vetor_objetos[]){
    glm::mat4 model;

    model = Matrix_Scale(0.15f, 0.15f, 0.15f);
            PushMatrix(model);
                model = Matrix_Translate(-4.2f, 0.0f, 1.5f) * model;
                PushMatrix(model);
                    model = model*Matrix_Rotate_Y(0.785f);
                    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                    glUniform1i(g_object_id_uniform, CAIXA);
                    DrawVirtualObject("Crate_Plane.005");
                    vetor_objetos[0].bbox_minimo =  Matrix_Scale(0.15f, 0.15f, 0.15f)*bbox_minimo_novo;
                    vetor_objetos[0].bbox_maximo = Matrix_Scale(0.15f, 0.15f, 0.15f) *bbox_maximo_novo;

                    vetor_objetos[0].bbox_minimo.x += (-4.2);
                    vetor_objetos[0].bbox_minimo.y += 0.0f;
                    vetor_objetos[0].bbox_minimo.z += 1.5f;

                    vetor_objetos[0].bbox_maximo.x += (-4.2);
                    vetor_objetos[0].bbox_maximo.y += 0.0f;
                    vetor_objetos[0].bbox_maximo.z += 1.5f;
                   //printf("%f, %f, %f, %f, %f, %f\n", vetor_objetos[0].bbox_minimo.x, vetor_objetos[0].bbox_maximo.x, vetor_objetos[0].bbox_minimo.y, vetor_objetos[0].bbox_maximo.y, vetor_objetos[0].bbox_minimo.z, vetor_objetos[0].bbox_maximo.z);

                PopMatrix(model);
                PushMatrix(model);
                    model = model * Matrix_Translate(0.0f, 5.0f, 0.0f);
                    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                    glUniform1i(g_object_id_uniform, CAIXA);
                    DrawVirtualObject("Crate_Plane.005");
                    vetor_objetos[1].bbox_minimo = Matrix_Scale(0.15f, 0.15f, 0.15f)*bbox_minimo_novo;
                    vetor_objetos[1].bbox_maximo = Matrix_Scale(0.15f, 0.15f, 0.15f)*bbox_maximo_novo;

                    vetor_objetos[1].bbox_minimo.x += (-4.2);
                    vetor_objetos[1].bbox_minimo.y += 0.75f;
                    vetor_objetos[1].bbox_minimo.z += 1.5f;

                    vetor_objetos[1].bbox_maximo.x += (-4.2);
                    vetor_objetos[1].bbox_maximo.y += 0.75f;
                    vetor_objetos[1].bbox_maximo.z += 1.5f;

                PopMatrix(model);
            PopMatrix(model);
            PushMatrix(model);
                model = Matrix_Translate(2.0f, 0.0f, 1.5f) * model * Matrix_Scale(1.0f, 2.0f, 1.0f);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, CAIXA);
                DrawVirtualObject("Crate_Plane.005");
                vetor_objetos[2].bbox_minimo = Matrix_Scale(0.15f, 0.3f, 0.15f)*bbox_minimo_novo;
                vetor_objetos[2].bbox_maximo = Matrix_Scale(0.15f, 0.3f, 0.15f)*bbox_maximo_novo;


                    vetor_objetos[2].bbox_minimo.x += 2.0f;
                    vetor_objetos[2].bbox_minimo.y += 0.0f;
                    vetor_objetos[2].bbox_minimo.z += 1.5f;

                    vetor_objetos[2].bbox_maximo.x += 2.0f;
                    vetor_objetos[2].bbox_maximo.y += 0.0f;
                    vetor_objetos[2].bbox_maximo.z += 1.5f;
                    //printf("%f, %f, %f, %f, %f, %f\n", vetor_objetos[2].bbox_minimo.x, vetor_objetos[2].bbox_maximo.x, vetor_objetos[2].bbox_minimo.y, vetor_objetos[2].bbox_maximo.y, vetor_objetos[2].bbox_minimo.z, vetor_objetos[2].bbox_maximo.z);

            PopMatrix(model);

        PushMatrix(model);
            model = Matrix_Translate(-1.4f, 0.0f, -1.0f) * model * Matrix_Rotate_Y(-0.2);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, CAIXA);
            DrawVirtualObject("Crate_Plane.005");
            vetor_objetos[3].bbox_minimo = Matrix_Scale(0.15f, 0.3f, 0.15f)*bbox_minimo_novo;
            vetor_objetos[3].bbox_maximo = Matrix_Scale(0.15f, 0.3f, 0.15f)*bbox_maximo_novo;


            vetor_objetos[3].bbox_minimo.x += -1.4f;
            vetor_objetos[3].bbox_minimo.y += 0.0f;
            vetor_objetos[3].bbox_minimo.z += -1.0f;

            vetor_objetos[3].bbox_maximo.x += -1.4f;
            vetor_objetos[3].bbox_maximo.y += 0.0f;
            vetor_objetos[3].bbox_maximo.z += -1.0f;
        PopMatrix(model);

        PushMatrix(model);
            model =  Matrix_Translate(2.2f, 0.0f, -0.8f) * model * Matrix_Rotate_Y(-0.4) * Matrix_Scale(3.0f, 0.5f, 1.0f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, CAIXA);
            DrawVirtualObject("Crate_Plane.005");
            vetor_objetos[4].bbox_minimo = Matrix_Scale(0.45f, 0.075f, 0.15f)*bbox_minimo_novo;
            vetor_objetos[4].bbox_maximo = Matrix_Scale(0.45f, 0.075f, 0.15f)*bbox_maximo_novo;



            vetor_objetos[4].bbox_minimo.x += 2.2f;
            vetor_objetos[4].bbox_minimo.y += 0.0f;
            vetor_objetos[4].bbox_minimo.z += -0.8f;

            vetor_objetos[4].bbox_maximo.x += 2.2f;
            vetor_objetos[4].bbox_maximo.y += 0.0f;
            vetor_objetos[4].bbox_maximo.z += -0.8f;
        PopMatrix(model);

        PushMatrix(model);
            model = Matrix_Translate(-7.5f, 0.0f, 6.7f) * model * Matrix_Scale(0.8f, 0.8f, 0.8f) * Matrix_Rotate_Y(0.1);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, CAIXA);
            DrawVirtualObject("Crate_Plane.005");
            vetor_objetos[5].bbox_minimo = Matrix_Scale(0.15f, 0.15f, 0.15f)*bbox_minimo_novo;
            vetor_objetos[5].bbox_maximo = Matrix_Scale(0.15f, 0.15f, 0.15f)*bbox_maximo_novo;

            vetor_objetos[5].bbox_minimo.x += -7.5f;
            vetor_objetos[5].bbox_minimo.y += 0.0f;
            vetor_objetos[5].bbox_minimo.z += 6.7f;

            vetor_objetos[5].bbox_maximo.x += -7.5f;
            vetor_objetos[5].bbox_maximo.y += 0.0f;
            vetor_objetos[5].bbox_maximo.z += 6.7f;
        PopMatrix(model);

        PushMatrix(model);
            model =  Matrix_Translate(-7.5f, 0.0f, 5.2f) * model * Matrix_Scale(0.8f, 0.8f, 0.8f) * Matrix_Rotate_Y(-0.21);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, CAIXA);
            DrawVirtualObject("Crate_Plane.005");
            vetor_objetos[6].bbox_minimo = model*bbox_minimo_novo;
            vetor_objetos[6].bbox_maximo = model*bbox_maximo_novo;

            vetor_objetos[6].bbox_minimo.x += -7.5f;
            vetor_objetos[6].bbox_minimo.y += 0.0f;
            vetor_objetos[6].bbox_minimo.z += 5.2f;

            vetor_objetos[6].bbox_maximo.x += -7.5f;
            vetor_objetos[6].bbox_maximo.y += 0.0f;
            vetor_objetos[6].bbox_maximo.z += 5.2f;
        PopMatrix(model);
}

void desenha_barreiras(ObjetoCenario vetor_objetos[]){

    glm::mat4 model;

    model = Matrix_Scale(0.007f, 0.007f, 0.007f) * Matrix_Rotate_X(29.85f);
        PushMatrix(model);
            model = Matrix_Translate(-2.7f, 0.0f, 1.6f) * model;
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, BARREIRAS);
            DrawVirtualObject("ConcreteConstructionBarrier");
            vetor_objetos[7].bbox_minimo = Matrix_Scale(0.007f, 0.014f, 0.007f)*bbox_minimo_novo;
            vetor_objetos[7].bbox_maximo = Matrix_Scale(0.007f, 0.014f, 0.007f)*bbox_maximo_novo;

            vetor_objetos[7].bbox_minimo.x += -2.7f;
            vetor_objetos[7].bbox_minimo.y += 0.0f;
            vetor_objetos[7].bbox_minimo.z += 1.2f;

            vetor_objetos[7].bbox_maximo.x += -2.7f;
            vetor_objetos[7].bbox_maximo.y += -0.2f;
            vetor_objetos[7].bbox_maximo.z += 1.2f;

            //printf("%f, %f, %f\n", camera_position_c.x, camera_position_c.y, camera_position_c.z);
            //printf("%f, %f, %f, %f, %f, %f\n", vetor_objetos[7].bbox_minimo.x, vetor_objetos[7].bbox_maximo.x, vetor_objetos[7].bbox_minimo.y, vetor_objetos[7].bbox_maximo.y, vetor_objetos[7].bbox_minimo.z, vetor_objetos[7].bbox_maximo.z);

        PopMatrix(model);
        PushMatrix(model);
            model = model * Matrix_Scale(3.5f, 0.8f, 0.8f);
            model = Matrix_Translate(-1.05f, 0.0f, 3.7f) * model;
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, BARREIRAS);
            DrawVirtualObject("ConcreteConstructionBarrier");
        PopMatrix(model);

        PushMatrix(model);
            model = model * Matrix_Scale(3.5f, 0.8f, 0.8f);
            model = Matrix_Translate(-1.05f, 0.0f, 7.5f) * model;
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, BARREIRAS);
            DrawVirtualObject("ConcreteConstructionBarrier");
        PopMatrix(model);

        PushMatrix(model);
            model = model  * Matrix_Rotate_Z(1.57f) * Matrix_Scale(1.8f, 0.8f, 0.8f);
            model = Matrix_Translate(-4.4f, 0.0f, 5.5f) * model;
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, BARREIRAS);
            DrawVirtualObject("ConcreteConstructionBarrier");
        PopMatrix(model);

        PushMatrix(model);
            model = model  * Matrix_Rotate_Z(1.57f) * Matrix_Scale(1.8f, 0.8f, 0.8f);
            model = Matrix_Translate(2.6f, 0.0f, 5.5f) * model;
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, BARREIRAS);
            DrawVirtualObject("ConcreteConstructionBarrier");
        PopMatrix(model);

        PushMatrix(model);
            model = Matrix_Translate(3.5f, 0.0f, 11.0f) * model * Matrix_Scale(1.0f, 1.0f, 3.0f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, BARREIRAS);
            DrawVirtualObject("ConcreteConstructionBarrier");
            vetor_objetos[8].bbox_minimo = Matrix_Scale(0.007f, 0.021f, 0.007f)*bbox_minimo_novo;
            vetor_objetos[8].bbox_maximo = Matrix_Scale(0.007f, 0.021f, 0.007f)*bbox_maximo_novo;


            vetor_objetos[8].bbox_minimo.x += 3.5f;
            vetor_objetos[8].bbox_minimo.y += 0.0f;
            vetor_objetos[8].bbox_minimo.z += 10.6f;

            vetor_objetos[8].bbox_maximo.x += 3.5f;
            vetor_objetos[8].bbox_maximo.y += 0.5f;
            vetor_objetos[8].bbox_maximo.z += 10.4f;
        PopMatrix(model);


}


void desenha_paletes(ObjetoCenario vetor_objetos[]){

        glm::mat4 model;
     /* DESENHO PALETA */
        model = Matrix_Translate(0.0f, 0.0f, -1.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PALETE);
        DrawVirtualObject("PalletPlywoodNew_LOD0");
        vetor_objetos[9].bbox_minimo = bbox_minimo_novo;
        vetor_objetos[9].bbox_maximo = bbox_maximo_novo;


        vetor_objetos[9].bbox_minimo.x += 0.0f;
        vetor_objetos[9].bbox_minimo.y += 0.0f;
        vetor_objetos[9].bbox_minimo.z += -1.0f;

            vetor_objetos[9].bbox_maximo.x += 0.0f;
            vetor_objetos[9].bbox_maximo.y += 0.0f;
            vetor_objetos[9].bbox_maximo.z += -1.0f;

        model = Matrix_Translate(0.0f, 0.0f, 12.0f) * Matrix_Scale(4.0f, 0.8f, 1.5f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PALETE);
        DrawVirtualObject("PalletPlywoodNew_LOD0");
        vetor_objetos[10].bbox_minimo = Matrix_Scale(4.0f, 0.8f, 1.5f)*bbox_minimo_novo;
        vetor_objetos[10].bbox_maximo = Matrix_Scale(4.0f, 0.8f, 1.5f)*bbox_maximo_novo;

        vetor_objetos[10].bbox_minimo.x += 0.0f;
        vetor_objetos[10].bbox_minimo.y += 0.0f;
        vetor_objetos[10].bbox_minimo.z += 12.0f;

            vetor_objetos[10].bbox_maximo.x += 0.0f;
            vetor_objetos[10].bbox_maximo.y += 0.0f;
            vetor_objetos[10].bbox_maximo.z += 12.0f;

        model = Matrix_Translate(-7.5f, 0.6f, 6.0f) * Matrix_Scale(0.3f, 0.4f, 1.0f) * Matrix_Rotate_Y(1.57f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PALETE);
        DrawVirtualObject("PalletPlywoodNew_LOD0");
        vetor_objetos[11].bbox_minimo = Matrix_Scale(0.3f, 0.4f, 1.0f)*bbox_minimo_novo;
        vetor_objetos[11].bbox_maximo = Matrix_Scale(0.3f, 0.4f, 1.0f)*bbox_maximo_novo;

        vetor_objetos[11].bbox_minimo.x += -7.5f;
        vetor_objetos[11].bbox_minimo.y += 0.6f;
        vetor_objetos[11].bbox_minimo.z += 6.0f;

        vetor_objetos[11].bbox_maximo.x += -7.5f;
        vetor_objetos[11].bbox_maximo.y += 0.6f;
        vetor_objetos[11].bbox_maximo.z += 6.0f;
}



void desenha_hud()
{
    glm::mat4 model, view, projection;
    glDisable(GL_DEPTH_TEST);
    /* DESENHO DA ARMA */
    model = Matrix_Translate(0.75f,-0.70f,0.0f)*Matrix_Scale(-0.12f,0.12f,0.12f)*Matrix_Rotate_X(-0.1f)*Matrix_Rotate_Y(-15.3f);
    view = Matrix_Identity();
    projection = Matrix_Identity();
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
    glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));
    glUniform1i(g_object_id_uniform, ARMA);
    DrawVirtualObject("Cube_Cube.001");
    /* DESENHO DA MIRA */
    glUniform1i(g_object_id_uniform, MIRA);
    view = Matrix_Identity();
    projection = Matrix_Identity();
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
    glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));
    /* PARTE ESQUERDA DA MIRA */
    model = Matrix_Translate(-0.02f,0.0f,0.0f)*Matrix_Scale(-0.012f,0.007f,0.1f)*Matrix_Rotate_X(-1.570796237f)*Matrix_Rotate_Y(-1.570796237f);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    DrawVirtualObject("the_plane");
    /* PARTE DIREITA DA MIRA */
    model = Matrix_Translate(0.02f,0.0f,0.0f)*Matrix_Scale(-0.012f,0.007f,0.1f)*Matrix_Rotate_X(-1.570796237f)*Matrix_Rotate_Y(-1.570796237f);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    DrawVirtualObject("the_plane");
    /* PARTE SUPERIOR DA MIRA */
    model = Matrix_Translate(0.0f,0.038f,0.0f)*Matrix_Scale(-0.0038f,0.022f,0.1f)*Matrix_Rotate_X(-1.570796237f)*Matrix_Rotate_Y(-1.570796237f);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    DrawVirtualObject("the_plane");
    /* PARTE INFERIOR DA MIRA */
    model = Matrix_Translate(0.0f,-0.038f,0.0f)*Matrix_Scale(-0.0038f,0.022f,0.1f)*Matrix_Rotate_X(-1.570796237f)*Matrix_Rotate_Y(-1.570796237f);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    DrawVirtualObject("the_plane");
    glEnable(GL_DEPTH_TEST);
}

void inicializa_esferas(Esfera vetor_esferas[])
{
    for (int i = 0; i < QUANTIDADE_ALVOS; i++)
    {
        if (i % 2 == 0)
            vetor_esferas[i].direcao = DIRECAO_DIREITA;
        else
            vetor_esferas[i].direcao = DIRECAO_ESQUERDA;
        vetor_esferas[i].centro_x = 0.0f;
        vetor_esferas[i].centro_y = ALTURA_ESFERAS;
        vetor_esferas[i].centro_z = -3.0f*i;
    }
}

void desenha_esferas(Esfera vetor_esferas[])
{
    glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem
    for (int i = 0; i < QUANTIDADE_ESFERAS; i++)
    {
        if (vetor_esferas[i].dano < MAXIMO_DANO)
        {
            model = Matrix_Translate(vetor_esferas[i].centro_x,vetor_esferas[i].centro_y,vetor_esferas[i].centro_z)*Matrix_Scale(RAIO_ESFERAS,RAIO_ESFERAS,RAIO_ESFERAS);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ESFERA);
            DrawVirtualObject("the_sphere");
        }
    }
}

void desenha_trofeu()
{
    glm::mat4 model;
    model = Matrix_Translate(0.0f,-0.5f,0.0f)*Matrix_Scale(4.0f,4.0f,4.0f)*Matrix_Rotate_Y(-3.14159265359f);
    glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, TROFEU);
    glDisable(GL_CULL_FACE);
    DrawVirtualObject("Cup");
    glEnable(GL_CULL_FACE);
}

bool verifica_fim(Alvo vetor_alvos[], Esfera vetor_esferas[])
{
    int i = 0;
    for(i = 0; i < QUANTIDADE_ALVOS; i++)
        if (vetor_alvos[i].dano < MAXIMO_DANO)
            break;

    if (i != QUANTIDADE_ALVOS)
        return false;

     for(i = 0; i < QUANTIDADE_ESFERAS; i++)
        if (vetor_esferas[i].dano < MAXIMO_DANO)
            break;

     if (i != QUANTIDADE_ESFERAS)
        return false;

    return true;
}

/* Função para controlar a movimentação dos alvos no cenário, com teste de colisão
envolvendo os pontos extremos do cenário e o ponto no qual o alvo se encontra. */
void controla_alvos(Alvo vetor_alvos[], double t, glm::vec4 vetor_bezier_alvo, double delta_t)
{
    vetor_alvos[0].x = vetor_bezier_alvo.x;
    vetor_alvos[0].z = vetor_bezier_alvo.z;

    for (int i = NUM_ALVOS_NAOLINEARES; i < QUANTIDADE_ALVOS; i++)
    {
        if (vetor_alvos[i].x >= LIMITE_DIR_ALVO) /* Inverte o sentido da direção do alvo para a esquerda, caso ele atinja o limite direito. */
            vetor_alvos[i].direcao = DIRECAO_ESQUERDA;

        else if (vetor_alvos[i].x <= LIMITE_ESQ_ALVO) /* Inverte o sentido da direção do alvo para a direita, caso ele atinja o limite esquerdo. */
            vetor_alvos[i].direcao = DIRECAO_DIREITA;

        if (vetor_alvos[i].direcao == DIRECAO_DIREITA)
            vetor_alvos[i].x += vetor_alvos[i].velocidade_alvo*delta_t;

        else if (vetor_alvos[i].direcao == DIRECAO_ESQUERDA)
            vetor_alvos[i].x -= vetor_alvos[i].velocidade_alvo*delta_t;
    }
}

/* Função para controlar a movimentação das esferas no cenário, com teste de colisão
envolvendo os pontos extremos do cenário e o ponto no qual a esfera se encontra. */
void controla_esferas(Esfera vetor_esferas[], double delta_t)
{
    for (int i = 0; i < QUANTIDADE_ALVOS; i++)
    {
        if (vetor_esferas[i].centro_x >= LIMITE_DIR_ESFERA) /* Inverte o sentido da direção da esfera para a esquerda, caso ela atinja o limite direito. */
            vetor_esferas[i].direcao = DIRECAO_ESQUERDA;

        else if (vetor_esferas[i].centro_x <= LIMITE_ESQ_ESFERA) /* Inverte o sentido da direção da esfera para a direita, caso ela atinja o limite esquerdo. */
            vetor_esferas[i].direcao = DIRECAO_DIREITA;

        if (vetor_esferas[i].direcao == DIRECAO_DIREITA)
            vetor_esferas[i].centro_x += vetor_esferas[i].velocidade_esfera*delta_t;

        else if (vetor_esferas[i].direcao == DIRECAO_ESQUERDA)
            vetor_esferas[i].centro_x -= vetor_esferas[i].velocidade_esfera*delta_t;
    }
}

void controla_balas(Bala vetor_balas[])
{
    for (int i = 0; i < QUANTIDADE_BALAS; i++)
    {
        if (vetor_balas[i].z >= LIMITE_FRENTE ||
            vetor_balas[i].z <= LIMITE_FUNDO ||
            vetor_balas[i].x <= LIMITE_ESQUERDA ||
            vetor_balas[i].x >= LIMITE_DIREITA ||
            vetor_balas[i].y >= LIMITE_CIMA ||
            vetor_balas[i].y <= LIMITE_BAIXO)
            {
                vetor_balas[i].desenhar = false;
                vetor_balas[i].x = 0.0;
                vetor_balas[i].y = 0.0;
                vetor_balas[i].z = 0.0;
            }
    }
}

void check_bbox(ObjetoCenario vetor_objetos[]){
    for (int i = 0; i < QUANTIDADE_OBJETOS; i++)
        {
            float aux;
            if(vetor_objetos[i].bbox_minimo.x > vetor_objetos[i].bbox_maximo.x){
                aux = vetor_objetos[i].bbox_maximo.x;
                vetor_objetos[i].bbox_maximo.x = vetor_objetos[i].bbox_minimo.x;
                vetor_objetos[i].bbox_minimo.x = aux;
            }
            if(vetor_objetos[i].bbox_minimo.y > vetor_objetos[i].bbox_maximo.y){
                aux = vetor_objetos[i].bbox_maximo.y;
                vetor_objetos[i].bbox_maximo.y = vetor_objetos[i].bbox_minimo.y;
                vetor_objetos[i].bbox_minimo.y = aux;
            }
            if(vetor_objetos[i].bbox_minimo.z > vetor_objetos[i].bbox_maximo.z){
                aux = vetor_objetos[i].bbox_maximo.z;
                vetor_objetos[i].bbox_maximo.z = vetor_objetos[i].bbox_minimo.z;
                vetor_objetos[i].bbox_minimo.z = aux;
            }

        }


}

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "INF01047 - Trabalho Final - Tiro ao alvo", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/textura_piso.jpg");    // TextureImage0
    LoadTextureImage("../../data/textB1!.png"); // TextureImage1
    LoadTextureImage("../../data/textura_bala.jpg"); // TextureImage2
    LoadTextureImage("../../data/glocktexture.png"); // TextureImage3
    LoadTextureImage("../../data/skybox.jpeg"); // TextureImage4
    LoadTextureImage("../../data/telainicio.jpg"); // TextureImage5
    LoadTextureImage("../../data/trofeu_textura.png"); // TextureImage6
    LoadTextureImage("../../data/skybox_trofeu.jpg"); // TextureImage7
    LoadTextureImage("../../data/WoodenCrate_Crate_BaseColor.png"); // TextureImage8
    LoadTextureImage("../../data/barreira.jpg"); // TextureImage9
    LoadTextureImage("../../data/PalletPlywood_Base_Color.png"); // TextureImage10

    // Construímos a representação de objetos geométricos através de malhas de triângulos

    ObjModel piso("../../data/piso.obj");
    ComputeNormals(&piso);
    BuildTrianglesAndAddToVirtualScene(&piso);

    ObjModel target("../../data/poligono1.obj");
    ComputeNormals(&target);
    BuildTrianglesAndAddToVirtualScene(&target);

    ObjModel bullet("../../data/bullet.obj");
    ComputeNormals(&bullet);
    BuildTrianglesAndAddToVirtualScene(&bullet);

    ObjModel arma("../../data/glock.obj");
    ComputeNormals(&arma);
    BuildTrianglesAndAddToVirtualScene(&arma);

    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel trofeu("../../data/Cup.obj");
    ComputeNormals(&trofeu);
    BuildTrianglesAndAddToVirtualScene(&trofeu);

    ObjModel caixa("../../data/WoodenCrate.obj");
    ComputeNormals(&caixa);
    BuildTrianglesAndAddToVirtualScene(&caixa);

    ObjModel barreira("../../data/barreira.obj");
    ComputeNormals(&barreira);
    BuildTrianglesAndAddToVirtualScene(&barreira);

    ObjModel palete("../../data/PalletPlywoodNew_GameReady_LODs.obj");
    ComputeNormals(&palete);
    BuildTrianglesAndAddToVirtualScene(&palete);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    /* TRABALHO FINAL - NOVAS VARIÁVEIS E CONFIGURAÇÕES USADAS NO LAÇO DEFINIDAS ABAIXO. */

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    Alvo vetor_alvos[QUANTIDADE_ALVOS];
    inicializa_alvos(vetor_alvos);

    ObjetoCenario vetor_objetos[QUANTIDADE_OBJETOS];

    Esfera vetor_esferas[QUANTIDADE_ESFERAS];
    inicializa_esferas(vetor_esferas);

    Bala vetor_balas[QUANTIDADE_BALAS];
    bool disparar = false;

    t_prev = glfwGetTime();

    double tempo_ant = glfwGetTime();
    double tempo;
    double delta_tempo;
    float tempo_alvo;
    bool direcao = true;

    glm::vec4 ponto1   = glm::vec4(-5.0f,0.0f,8.0f,1.0f);
    glm::vec4 ponto2   = glm::vec4(9.0f,0.0f,10.0f,1.0f);
    glm::vec4 ponto3   = glm::vec4(-10.0f,0.0f,10.0f,1.0f);
    glm::vec4 ponto4   = glm::vec4(4.0f,0.0f,8.0f,1.0f);

    glm::vec4 vetor12;
    glm::vec4 vetor23;
    glm::vec4 vetor34;
    glm::vec4 vetor123;
    glm::vec4 vetor234;
    glm::vec4 vetor_bezier_alvo;

    double tempo_fps = glfwGetTime();

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        tempo = glfwGetTime();
        delta_tempo = tempo - tempo_ant;
        if(delta_tempo > TEMPO_ALVO_BEZIER){
            tempo_ant = glfwGetTime();
            direcao = !direcao;
            delta_tempo = tempo - tempo_ant;
        }
        if(!direcao){
            delta_tempo = TEMPO_ALVO_BEZIER - delta_tempo;
        }

        tempo_alvo = (float)(delta_tempo/TEMPO_ALVO_BEZIER);

        vetor12 = ponto1 + tempo_alvo*(ponto2 - ponto1);
        vetor23 = ponto2 + tempo_alvo*(ponto3 - ponto2);
        vetor34 = ponto3 + tempo_alvo*(ponto4 - ponto3);

        vetor123 = vetor12 + tempo_alvo*(vetor23 - vetor12);
        vetor234 = vetor23 + tempo_alvo*(vetor34 - vetor23);

        vetor_bezier_alvo = vetor123 + tempo_alvo*(vetor234 - vetor123);

        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        r = g_CameraDistance;
        y = r*sin(g_CameraPhi);
        z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        x = r*cos(g_CameraPhi)*sin(g_CameraTheta);
        camera_view_vector = glm::vec4(x,y,z,0.0f); // Vetor "view", sentido para onde a câmera está virada
        camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
        camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando

        if (fim_jogo)
        {
            camera_position_c  = glm::vec4(x,y,z,1.0f); // Ponto "c", centro da câmera
            camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
            camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        }

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -60.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        /* NOVAS CHAMADAS DE FUNÇÕES DO TRABALHO FINAL ABAIXO. */

        if (!iniciar_jogo && !fim_jogo)
            tela_inicio();

        if (iniciar_jogo && !fim_jogo)
        {
            desenha_chao();

            t_now = glfwGetTime();
            delta_t = t_now-t_prev;
            t_prev = t_now;

            controla_esferas(vetor_esferas, delta_t);
            controla_alvos(vetor_alvos, tempo_alvo, vetor_bezier_alvo, delta_t);
            desenha_alvos(vetor_alvos);
            desenha_esferas(vetor_esferas);
            desenha_caixas(vetor_objetos);
            desenha_barreiras(vetor_objetos);
            desenha_paletes(vetor_objetos);
            check_bbox(vetor_objetos);
            controla_balas(vetor_balas);

            if(g_LeftMouseButtonPressed && disparar == 0)
                disparar = true;
            if(!g_LeftMouseButtonPressed && disparar == true)
            {
                disparar = false;
                dispara_balas(vetor_balas);
            }

            desenha_balas(vetor_balas);
            destroi_balas(vetor_balas, vetor_objetos);
            destroi_alvos(vetor_balas,vetor_alvos);
            destroi_esferas(vetor_balas,vetor_esferas);
            desenha_skybox(SKYBOX);
            desenha_hud();

            fim_jogo = verifica_fim(vetor_alvos,vetor_esferas);
        }

        if (fim_jogo)
        {
            desenha_trofeu();
            desenha_skybox(SKYBOX_TROFEU);
        }


        /* NOVAS CHAMADAS DE FUNÇÕES DO TRABALHO FINAL ACIMA. */

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();

        glm::vec4 nova_pos = camera_position_c;
        glm::vec4 w_normalizado = w;
        w_normalizado.y = 0.0f;
        w_normalizado = normalize(w_normalizado);

        if (iniciar_jogo && !fim_jogo)
        {
            if (tecla_W_pressionada == 1)
            {
                nova_pos.x += delta_t*(-1*w_normalizado.x*VELOCIDADE_CAMERA);
                //camera_position_c.y += (-1*w.y*VELOCIDADE_CAMERA);
                nova_pos.z += delta_t*(-1*w_normalizado.z*VELOCIDADE_CAMERA);
            }

            if (tecla_S_pressionada == 1)
            {
                nova_pos.x += delta_t*(w_normalizado.x*VELOCIDADE_CAMERA);
                //camera_position_c.y += (w.y*VELOCIDADE_CAMERA);
                nova_pos.z += delta_t*(w_normalizado.z*VELOCIDADE_CAMERA);
            }

            if (tecla_D_pressionada == 1)
            {
                nova_pos.x += delta_t*(u.x*VELOCIDADE_CAMERA);
                //camera_position_c.y += (u.y*VELOCIDADE_CAMERA);
                nova_pos.z += delta_t*(u.z*VELOCIDADE_CAMERA);
            }

            if (tecla_A_pressionada == 1)
            {
                nova_pos.x += delta_t*(-1*u.x*VELOCIDADE_CAMERA);
                //camera_position_c.y += (-1*u.y*VELOCIDADE_CAMERA);
                nova_pos.z += delta_t*(-1*u.z*VELOCIDADE_CAMERA);
            }
            glm::vec4 bbox_jogador_max = nova_pos;
            bbox_jogador_max.x += ESPESSURA_JOGADOR;
            bbox_jogador_max.z += ESPESSURA_JOGADOR;
            glm::vec4 bbox_jogador_min = nova_pos;
            bbox_jogador_min.x -= ESPESSURA_JOGADOR;
            bbox_jogador_min.z -= ESPESSURA_JOGADOR;
            bool parede1 = limita_jogador_plano_x(bbox_jogador_max, bbox_jogador_min, -4.4f);
            bool parede2 = limita_jogador_plano_x(bbox_jogador_max, bbox_jogador_min, 2.3f);
            bool parede3 = limita_jogador_plano_z(bbox_jogador_max, bbox_jogador_min, 3.9f);
            bool parede4 = limita_jogador_plano_z(bbox_jogador_max, bbox_jogador_min, 7.0f);
            if(!parede1 && !parede2){
                camera_position_c.x = nova_pos.x;
            }
            if(!parede3 && !parede4){
                camera_position_c.z = nova_pos.z;
            }
        }
        while (glfwGetTime() < tempo_fps + 1.0/TARGET_FPS) {
        // TODO: Put the thread to sleep, yield, or simply do nothing
        }
        tempo_fps += 1.0/TARGET_FPS;

    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;

    /* NOVA ADICAO ABAIXO. */

    bbox_minimo_novo.x = bbox_min.x;
    bbox_minimo_novo.y = bbox_min.y;
    bbox_minimo_novo.z = bbox_min.z;
    bbox_minimo_novo.w = 0.0f;

    bbox_maximo_novo.x = bbox_max.x;
    bbox_maximo_novo.y = bbox_max.y;
    bbox_maximo_novo.z = bbox_max.z;
    bbox_maximo_novo.w = 0.0f;

    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage9"), 9);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage10"), 10);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

   // if (!g_LeftMouseButtonPressed)
      //  return;

    if (!fim_jogo)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   -= 0.01f*dy;

        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f/2;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    else if (g_LeftMouseButtonPressed && fim_jogo)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f/2;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // =================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // =================

    w = negative_vector(camera_view_vector)/norm(camera_view_vector);/* PREENCHA AQUI o cálculo do vetor w */;
    u = crossproduct(camera_up_vector,w)/norm(crossproduct(camera_up_vector,w)); /* PREENCHA AQUI o cálculo do vetor u */;

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    /* SE O USUÁRIO PRESSIONAR ANTER, INICIAMOS O JOGO. */
    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
        iniciar_jogo = true;

    if (key == GLFW_KEY_W)
    {
        if (action == GLFW_PRESS)
            // Usuário apertou a tecla D, então atualizamos o estado para pressionada
            tecla_W_pressionada = 1;

        else if (action == GLFW_RELEASE)
            // Usuário largou a tecla D, então atualizamos o estado para NÃO pressionada
            tecla_W_pressionada = 0;
    }

    if (key == GLFW_KEY_A)
    {
        if (action == GLFW_PRESS)
            tecla_A_pressionada = 1;

        else if (action == GLFW_RELEASE)
            tecla_A_pressionada = 0;
    }

    if (key == GLFW_KEY_S)
    {
        if (action == GLFW_PRESS)
            tecla_S_pressionada = 1;

        else if (action == GLFW_RELEASE)
            tecla_S_pressionada = 0;
    }

    if (key == GLFW_KEY_D)
    {
        if (action == GLFW_PRESS)
            tecla_D_pressionada = 1;

        else if (action == GLFW_RELEASE)
            tecla_D_pressionada = 0;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :
