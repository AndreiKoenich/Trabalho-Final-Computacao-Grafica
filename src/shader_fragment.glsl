#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;
in vec3 cor_v;
// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
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

#define RO 10
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;
uniform sampler2D TextureImage6;
uniform sampler2D TextureImage7;
uniform sampler2D TextureImage8;
uniform sampler2D TextureImage9;
uniform sampler2D TextureImage10;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = (-1*l) + 2*n*dot(n,l);

    // VALOR DO HALF-VECTOR, USADO NO MODELO DE ILUMINAÇÃO BLINN-PHONG
    vec4 h = (v+l)/sqrt(dot(v+l,v+l));

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    float q; // Expoente especular para o modelo de iluminação de Phong

    if (object_id == PLANE)
    {
        Kd = vec3(0.2,0.2,0.2);
        Ks = vec3(0.3,0.3,0.3);
        Ka = vec3(0.0,0.0,0.0);
        q = 20.0;
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage0, vec2(U,V)).rgb;
    }

    else if (object_id == ALVO)
    {
        //Ka = vec3(1.000000, 1.000000, 1.000000);
       // Kd = vec3(0.001214, 0.000728, 0.001943);
       // Ks = vec3(0.500000, 0.500000, 0.500000);
        q = 20.0;
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage1, vec2(U,V)).rgb;
        Kd *= 100;
    }

    else if (object_id == BULLET)
    {
        q = 20.0;
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage2, vec2(U,V)).rgb;
    }

    else if (object_id == ARMA)
    {
        q = 30.0;
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage3, vec2(U,V)).rgb;
        Ks = vec3(0.8,0.8,0.8);
        Ka = Kd/2;
    }

    else if (object_id == MIRA)
    {
        Kd = vec3(0.0,255.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.0,0.0,0.0);
        q = 20.0;
    }

    else if (object_id == SKYBOX)
    {
        q = 0.0;
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;
        bbox_center.w = 1.0;


        vec4 p_linha = bbox_center + normalize(position_model - bbox_center);
        vec4 p_vetor = p_linha - bbox_center;

        float theta = atan(p_vetor.x, p_vetor.z);
        float phi = asin(p_vetor.y);

        U = (theta + M_PI)/(2*M_PI);
        V = (phi + M_PI_2)/M_PI;
        Kd = texture(TextureImage4, vec2(U,V)).rgb;
    }

    else if (object_id == TELA_INICIO)
    {
        Kd = vec3(0.2,0.2,0.2);
        Ks = vec3(0.3,0.3,0.3);
        Ka = vec3(0.0,0.0,0.0);
        q = 20.0;
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage5, vec2(U,V)).rgb;
        Kd *= 100;
    }

    else if (object_id == TROFEU)
    {
        Kd = vec3(0.05,0.05,0.05);
        Ks = vec3(0.5,0.5,0.5);
        Ka = vec3(0.502,0.502,0.502);
        q = 10.0;
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage6, vec2(U,V)).rgb;
    }

    else if (object_id == SKYBOX_TROFEU)
    {
        q = 0.0;
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;
        bbox_center.w = 1.0;

        vec4 p_linha = bbox_center + normalize(position_model - bbox_center);
        vec4 p_vetor = p_linha - bbox_center;

        float theta = atan(p_vetor.x, p_vetor.z);
        float phi = asin(p_vetor.y);

        U = (theta + M_PI)/(2*M_PI);
        V = (phi + M_PI_2)/M_PI;
        Kd = texture(TextureImage7, vec2(U,V)).rgb;
    }
     else if ( object_id == CAIXA )
    {
        q = 20.0;
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage8, vec2(U,V)).rgb;
    }
    else if ( object_id == BARREIRAS )
    {
        q = 20.0;
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage9, vec2(U,V)).rgb;
    }
    else if ( object_id == PALETE )
    {
        q = 20.0;
        U = texcoords.x;
        V = texcoords.y;
        Kd = texture(TextureImage10, vec2(U,V)).rgb;
    }

    else if (object_id == ESFERA)
    {
        // PREENCHA AQUI
        // Propriedades espectrais da esfera
        Kd = vec3(0.8,0.4,0.08);
        Ks = vec3(0.8,0.8,0.8);
        Ka = Kd/2;
        q = 20.0;
    }

    // Espectro da fonte de iluminação
    vec3 I = vec3(1.0,1.0,1.0); // PREENCHA AQUI o espectro da fonte de luz

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.2,0.2,0.2); // PREENCHA AQUI o espectro da luz ambiente

    // Equação de Iluminação
    float lambert = max(0,dot(n,l));

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec3 lambert_diffuse_term = Kd*I*max(0, dot(n,l)); // PREENCHA AQUI o termo difuso de Lambert

    // Termo ambiente
    vec3 ambient_term = Ka*Ia; // PREENCHA AQUI o termo ambiente

    // Termo especular utilizando o modelo de iluminação de Phong
    vec3 phong_specular_term  = Ks*I*pow(max(0, dot(r, v)), q); // PREENCHA AQUI o termo especular de Phong

    vec3 blinn_phong_specular_term  = Ks*I*pow(dot(n,h),q); // TERMO ESPECULAR DE BLINN-PHONG

    if(object_id == SKYBOX || object_id == SKYBOX_TROFEU)
        color.rgb = Kd;

    else if (object_id == ESFERA)
        color.rgb = cor_v;
        //color.rgb = lambert_diffuse_term + ambient_term + phong_specular_term;


    else
        color.rgb = Kd * (lambert + 0.01);

    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = 1;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
}

