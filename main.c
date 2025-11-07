/*
 * Trabalho de Computação Gráfica - Visualizador OBJ (Parser C Manual)
 *
 * Este programa usa:
 * - FreeGLUT para gerenciamento de janelas e eventos
 * - OpenGL (modo imediato) para renderização
 * - Um parser C manual para carregar arquivos .obj (simplificado)
 */

#include <GL/glut.h> // Para FreeGLUT (janelas, eventos)
#include <GL/glu.h>  // Para GLU (câmera, perspectiva)
#include <GL/gl.h>   // Para OpenGL (funções de renderização)
#include <stdio.h>   // Para printf, fopen, etc.
#include <stdlib.h>  // Para atexit, malloc, free, realloc
#include <string.h>  // Para strrchr, memset, strncmp
#include <math.h>    // Para fabs, fmax

// -----------------------------------------------------------------------------
// Estruturas de Dados Manuais para o OBJ
// -----------------------------------------------------------------------------

// Vetor 3D simples
typedef struct {
    float x, y, z;
} vec3f;

// Índices para um único vértice de uma face (formato v/vt/vn)
// Índices do OBJ começam em 1
typedef struct {
    int v_idx;  // Índice do vértice
    int vn_idx; // Índice da normal
    int vt_idx; // Índice da coordenada de textura (não usado neste exemplo)
} face_vertex_t;

// Face (sempre um triângulo)
typedef struct {
    face_vertex_t v[3];
} face_t;

// --- Dados Globais do Modelo ---
vec3f* g_vertices = NULL;
size_t g_num_vertices = 0;
size_t g_cap_vertices = 0;

vec3f* g_normals = NULL;
size_t g_num_normals = 0;
size_t g_cap_normals = 0;

face_t* g_faces = NULL;
size_t g_num_faces = 0;
size_t g_cap_faces = 0;

// --- Rotação com Mouse ---
int g_isDragging = 0;   // Verdadeiro se o mouse estiver sendo arrastado
int g_lastX = 0, g_lastY = 0; // Última posição do mouse
float g_rotateX = 0.0f; // Rotação acumulada no eixo X
float g_rotateY = 0.0f; // Rotação acumulada no eixo Y

// --- Bounding Box (Caixa Delimitadora) para Câmera ---
float g_center[3] = {0.0f, 0.0f, 0.0f}; // Centro do modelo
float g_size = 1.0f; // Maior dimensão do modelo

// -----------------------------------------------------------------------------
// Funções de Limpeza e Carregamento (Parser Manual)
// -----------------------------------------------------------------------------

/*
 * Libera a memória alocada para os dados do modelo
 */
void cleanup() {
    printf("Limpando dados do OBJ...\n");
    if (g_vertices) free(g_vertices);
    if (g_normals)  free(g_normals);
    if (g_faces)    free(g_faces);
}

// --- Funções Auxiliares de Alocação Dinâmica ---

void add_vertex(float x, float y, float z) {
    if (g_num_vertices >= g_cap_vertices) {
        g_cap_vertices = (g_cap_vertices == 0) ? 128 : g_cap_vertices * 2;
        g_vertices = (vec3f*)realloc(g_vertices, g_cap_vertices * sizeof(vec3f));
        if (!g_vertices) {
            fprintf(stderr, "Falha ao alocar memória para vértices\n");
            exit(1);
        }
    }
    g_vertices[g_num_vertices++] = (vec3f){x, y, z};
}

void add_normal(float nx, float ny, float nz) {
    if (g_num_normals >= g_cap_normals) {
        g_cap_normals = (g_cap_normals == 0) ? 128 : g_cap_normals * 2;
        g_normals = (vec3f*)realloc(g_normals, g_cap_normals * sizeof(vec3f));
        if (!g_normals) {
            fprintf(stderr, "Falha ao alocar memória para normais\n");
            exit(1);
        }
    }
    g_normals[g_num_normals++] = (vec3f){nx, ny, nz};
}

void add_face(face_vertex_t v0, face_vertex_t v1, face_vertex_t v2) {
     if (g_num_faces >= g_cap_faces) {
        g_cap_faces = (g_cap_faces == 0) ? 128 : g_cap_faces * 2;
        g_faces = (face_t*)realloc(g_faces, g_cap_faces * sizeof(face_t));
        if (!g_faces) {
            fprintf(stderr, "Falha ao alocar memória para faces\n");
            exit(1);
        }
    }
    g_faces[g_num_faces].v[0] = v0;
    g_faces[g_num_faces].v[1] = v1;
    g_faces[g_num_faces].v[2] = v2;
    g_num_faces++;
}


/*
 * Função para carregar o .OBJ e calcular a Bounding Box
 */
void loadOBJ(const char* filename) {
    printf("Carregando %s com parser C manual...\n", filename);
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Erro: Não foi possível abrir %s\n", filename);
        exit(1);
    }
    
    // Registra a limpeza para ser chamada ao sair
    atexit(cleanup);

    char line[1024];
    
    // Bounding box
    float min_v[3] = {1e9, 1e9, 1e9};
    float max_v[3] = {-1e9, -1e9, -1e9};

    while (fgets(line, sizeof(line), file)) {
        // --- Carregar Vértices ---
        if (strncmp(line, "v ", 2) == 0) {
            float x, y, z;
            if (sscanf(line, "v %f %f %f", &x, &y, &z) == 3) {
                add_vertex(x, y, z);
                // Atualiza bounding box
                if (x < min_v[0]) min_v[0] = x;
                if (x > max_v[0]) max_v[0] = x;
                if (y < min_v[1]) min_v[1] = y;
                if (y > max_v[1]) max_v[1] = y;
                if (z < min_v[2]) min_v[2] = z;
                if (z > max_v[2]) max_v[2] = z;
            }
        // --- Carregar Normais ---
        } else if (strncmp(line, "vn ", 3) == 0) {
            float nx, ny, nz;
            if (sscanf(line, "vn %f %f %f", &nx, &ny, &nz) == 3) {
                add_normal(nx, ny, nz);
            }
        // --- Carregar Faces ---
        } else if (strncmp(line, "f ", 2) == 0) {
            int v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3;
            face_vertex_t fv0, fv1, fv2;

            // Tenta ler formato v/vt/vn
            int matches = sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", 
                                 &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
            if (matches == 9) {
                fv0 = (face_vertex_t){v1, vn1, vt1};
                fv1 = (face_vertex_t){v2, vn2, vt2};
                fv2 = (face_vertex_t){v3, vn3, vt3};
                add_face(fv0, fv1, fv2);
            } else {
                // Tenta ler formato v//vn
                matches = sscanf(line, "f %d//%d %d//%d %d//%d", 
                                 &v1, &vn1, &v2, &vn2, &v3, &vn3);
                if (matches == 6) {
                    fv0 = (face_vertex_t){v1, vn1, 0};
                    fv1 = (face_vertex_t){v2, vn2, 0};
                    fv2 = (face_vertex_t){v3, vn3, 0};
                    add_face(fv0, fv1, fv2);
                } else {
                    // Tenta ler formato v
                    matches = sscanf(line, "f %d %d %d", &v1, &v2, &v3);
                    if (matches == 3) {
                        fv0 = (face_vertex_t){v1, 0, 0};
                        fv1 = (face_vertex_t){v2, 0, 0};
                        fv2 = (face_vertex_t){v3, 0, 0};
                        add_face(fv0, fv1, fv2);
                    } else {
                        // Se falhar, imprime aviso. (Provavelmente um polígono)
                        // printf("Aviso: Ignorando face não-triangular ou formato não suportado: %s", line);
                    }
                }
            }
        }
    }
    fclose(file);

    // --- Finalizar Bounding Box ---
    if (g_num_vertices == 0) {
         fprintf(stderr, "Objeto não tem vértices.\n");
         exit(1);
    }
    
    g_center[0] = (min_v[0] + max_v[0]) / 2.0f;
    g_center[1] = (min_v[1] + max_v[1]) / 2.0f;
    g_center[2] = (min_v[2] + max_v[2]) / 2.0f;

    float size_x = max_v[0] - min_v[0];
    float size_y = max_v[1] - min_v[1];
    float size_z = max_v[2] - min_v[2];

    g_size = fmax(fmax(fabs(size_x), fabs(size_y)), fabs(size_z));
    
    printf("Objeto carregado. Centro: (%.2f, %.2f, %.2f), Tamanho: %.2f\n", 
           g_center[0], g_center[1], g_center[2], g_size);
    
    printf("Vértices: %lu, Normais: %lu, Faces (triângulos): %lu\n",
           g_num_vertices, g_num_normals, g_num_faces);
}

// -----------------------------------------------------------------------------
// Funções de Callback do GLUT
// -----------------------------------------------------------------------------

/*
 * Callback de Renderização (Display)
 */
void myDisplay(void) {
    // Limpa o buffer de cor e o buffer de profundidade
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Mudar para a matriz de Modelview
    glMatrixMode(GL_MODELVIEW);
    // Reinicia a matriz (anula transformações prévias)
    glLoadIdentity();

    // --- 1. Configuração da Câmera ---
    gluLookAt(g_center[0], g_center[1], g_center[2] + g_size * 2.0, // Eye (Posição)
              g_center[0], g_center[1], g_center[2],          // Ctr (Para onde olha)
              0.0, 1.0, 0.0);                         // Up (Vetor "para cima")

    // --- 2. Posição da Luz ---
    GLfloat light_pos[] = {g_center[0], g_center[1] + g_size, g_center[2] + g_size, 1.0};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);    
    
    // --- 3. Transformações do Objeto ---
    glRotatef(g_rotateX, 1.0f, 0.0f, 0.0f); // Rotação em X
    glRotatef(g_rotateY, 0.0f, 1.0f, 0.0f); // Rotação em Y
    glTranslatef(-g_center[0], -g_center[1], -g_center[2]);

    // --- 4. Renderização do Objeto ---
    
    // Define um material branco padrão, já que não lemos o .mtl
    GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);

    // Inicia o desenho dos triângulos
    glBegin(GL_TRIANGLES);
    
    for (size_t i = 0; i < g_num_faces; i++) {
        face_t* f = &g_faces[i];
        
        // Desenha os 3 vértices do triângulo
        for (int v = 0; v < 3; v++) {
            face_vertex_t* fv = &f->v[v];
            
            // Índices do OBJ são 1-based, arrays em C são 0-based
            int v_idx = fv->v_idx - 1;
            int vn_idx = fv->vn_idx - 1;

            // Define a Normal (se existir)
            if (vn_idx >= 0 && vn_idx < g_num_normals) {
                vec3f* n = &g_normals[vn_idx];
                glNormal3f(n->x, n->y, n->z);
            }

            // Define o Vértice (se existir)
            if (v_idx >= 0 && v_idx < g_num_vertices) {
                vec3f* vert = &g_vertices[v_idx];
                glVertex3f(vert->x, vert->y, vert->z);
            }
        }
    }
    glEnd(); // Fim dos triângulos

    // Troca os buffers
    glutSwapBuffers();
}

/*
 * Callback de Redimensionamento (Reshape)
 */
void myReshape(int w, int h) {
    // Define a viewport para cobrir a janela inteira
    glViewport(0, 0, w, h);

    // Mudar para a matriz de Projeção
    glMatrixMode(GL_PROJECTION);
    // Reinicia a matriz
    glLoadIdentity();

    if (h == 0) h = 1; // Evita divisão por zero
    float aspect = (float)w / (float)h;

    // Configura a projeção em perspectiva
    gluPerspective(60.0, aspect, 0.1, g_size * 100.0); 

    // Volta para a matriz de Modelview
    glMatrixMode(GL_MODELVIEW);
}

/*
 * Callback de clique do Mouse
 */
void myMouse(int button, int state, int x, int y) { 
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            g_isDragging = 1;
            g_lastX = x;
            g_lastY = y;
        } else if (state == GLUT_UP) {
            g_isDragging = 0;
        }
    }
}

/*
 * Callback de movimento do Mouse (arrastar)
 */
void myMotion(int x, int y) { 
    if (g_isDragging) {
        int deltaX = x - g_lastX;
        int deltaY = y - g_lastY;

        // Acumula a rotação
        g_rotateX += deltaY * 0.5f; // Rotação no eixo X (movimento vertical do mouse)
        g_rotateY += deltaX * 0.5f; // Rotação no eixo Y (movimento horizontal do mouse)

        g_lastX = x;
        g_lastY = y;

        // Pede ao GLUT para redesenhar a cena
        glutPostRedisplay();
    }
}

// -----------------------------------------------------------------------------
// Função Principal
// -----------------------------------------------------------------------------
int main(int argc, char** argv) {
    
    glutInit(&argc, argv);
    
    if (argc < 2) {
        printf("Erro: Forneça o caminho para o arquivo .obj\n");
        printf("Uso: %s <arquivo.obj>\n", argv[0]);
        return 1;
    }

    // Configura o modo de display:
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Configura o tamanho e posição da janela
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    
    // Cria a janela
    glutCreateWindow("Visualizador OBJ - Parser C Manual"); 

    // --- Carrega o Modelo ---
    loadOBJ(argv[1]);

    // --- Configurações de Estado do OpenGL ---
    glEnable(GL_DEPTH_TEST); 
    glEnable(GL_LIGHTING);   
    glEnable(GL_LIGHT0);    
    glShadeModel(GL_SMOOTH); //

    // Define propriedades da Luz 0
    GLfloat light_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient); 
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    
    // Define propriedades de material (brilho especular)
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 50.0 };
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    
    // Habilita o uso de glMaterialfv para definir a cor
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);


    // Define uma cor de fundo (cinza escuro)
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // --- Registra as Callbacks ---
    glutDisplayFunc(myDisplay); 
    glutReshapeFunc(myReshape);
    glutMouseFunc(myMouse);   
    glutMotionFunc(myMotion);   


    glutMainLoop();

    return 0; 
}