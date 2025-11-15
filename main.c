#include <GL/glut.h> 
#include <GL/glu.h> 
#include <GL/gl.h>  
#include <stdio.h>  
#include <stdlib.h> 
#include <string.h> 
#include <math.h>   
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Vetor 3D simples
typedef struct {
    float x, y, z;
} vec3f;
// Vetor 2D pra textura
typedef struct {
    float u, v;
} vec2f;

typedef struct {
    char name[128];      // Nome (newmtl)
    float ambient[4];    // Ka
    float diffuse[4];    // Kd
    float specular[4];   // Ks
    float shininess;     // Ns
    unsigned int texture_id; // ID da textura (map_Kd)
} material_t;

// Índices para um único vértice de uma face (formato v/vt/vn)
// Índices do OBJ começam em 1
typedef struct {
    int v_idx;  // Índice do vértice
    int vn_idx; // Índice da normal
    int vt_idx; // Índice da coordenada de textura (não usado neste exemplo)
} face_vertex_t;

// Face (triângulo)
typedef struct {
    face_vertex_t v[3];
    int material_id;
} face_t;

vec3f* g_vertices = NULL;
size_t g_num_vertices = 0;
size_t g_cap_vertices = 0;

vec3f* g_normals = NULL;
size_t g_num_normals = 0;
size_t g_cap_normals = 0;

face_t* g_faces = NULL;
size_t g_num_faces = 0;
size_t g_cap_faces = 0;

vec2f* g_texcoords = NULL;
size_t g_num_texcoords = 0;
size_t g_cap_texcoords = 0;

material_t* g_materials = NULL;
size_t g_num_materials = 0;
size_t g_cap_materials = 0;


int g_isDragging = 0; 
int g_lastX = 0, g_lastY = 0;
float g_rotateX = 0.0f;
float g_rotateY = 0.0f; 

float g_center[3] = {0.0f, 0.0f, 0.0f}; 
float g_size = 1.0f;

unsigned int g_default_texture = 0;

void cleanup() {
    printf("Limpando dados do OBJ...\n");
    if (g_vertices) free(g_vertices);
    if (g_normals)  free(g_normals);
    if (g_faces)    free(g_faces);
    
    if (g_texcoords) free(g_texcoords);
        if (g_materials) {
            for(size_t i = 0; i < g_num_materials; i++) {
                if (g_materials[i].texture_id > 0) {
                    glDeleteTextures(1, &g_materials[i].texture_id);
                }
            }
            free(g_materials);
        }
}

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

void add_texcoord(float u, float v) {
    if (g_num_texcoords >= g_cap_texcoords) {
        g_cap_texcoords = (g_cap_texcoords == 0) ? 128 : g_cap_texcoords * 2;
        g_texcoords = (vec2f*)realloc(g_texcoords, g_cap_texcoords * sizeof(vec2f));
        if (!g_texcoords) {
            fprintf(stderr, "Falha ao alocar memória para texcoords\n");
            exit(1);
        }
    }
    g_texcoords[g_num_texcoords++] = (vec2f){u, v};
}
void add_face(face_vertex_t v0, face_vertex_t v1, face_vertex_t v2, int material_id) {
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
    g_faces[g_num_faces].material_id = material_id; 
    g_num_faces++;
}
int add_material(const char* name) {
    if (g_num_materials >= g_cap_materials) {
        g_cap_materials = (g_cap_materials == 0) ? 8 : g_cap_materials * 2;
        g_materials = (material_t*)realloc(g_materials, g_cap_materials * sizeof(material_t));
        if (!g_materials) {
            fprintf(stderr, "Falha ao alocar memória para materiais\n");
            exit(1);
        }
    }
    strncpy(g_materials[g_num_materials].name, name, 127);
    g_materials[g_num_materials].name[127] = '\0';
    
    // Define valores padrão
    g_materials[g_num_materials].ambient[0] = 0.2f; g_materials[g_num_materials].ambient[1] = 0.2f; g_materials[g_num_materials].ambient[2] = 0.2f; g_materials[g_num_materials].ambient[3] = 1.0f;
    g_materials[g_num_materials].diffuse[0] = 0.8f; g_materials[g_num_materials].diffuse[1] = 0.8f; g_materials[g_num_materials].diffuse[2] = 0.8f; g_materials[g_num_materials].diffuse[3] = 1.0f;
    g_materials[g_num_materials].specular[0] = 0.0f; g_materials[g_num_materials].specular[1] = 0.0f; g_materials[g_num_materials].specular[2] = 0.0f; g_materials[g_num_materials].specular[3] = 1.0f;
    g_materials[g_num_materials].shininess = 0.0f;
    g_materials[g_num_materials].texture_id = 0; // 0 = sem textura

    return g_num_materials++;
}

// Procura um material pelo nome e retorna seu índice
int find_material(const char* name) {
    for (size_t i = 0; i < g_num_materials; i++) {
        if (strncmp(g_materials[i].name, name, 128) == 0) {
            return (int)i;
        }
    }
    return -1; // Não encontrado
}


// Cria uma textura branca padrão para objetos sem textura
unsigned int createDefaultTexture() {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Define parâmetros de wrapping e filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Cria uma textura gradiente vermelho-azul 256x256
    #define GRADIENT_SIZE 256
    unsigned char* gradient = (unsigned char*)malloc(GRADIENT_SIZE * GRADIENT_SIZE * 4);
    
    for (int y = 0; y < GRADIENT_SIZE; y++) {
        for (int x = 0; x < GRADIENT_SIZE; x++) {
            int idx = (y * GRADIENT_SIZE + x) * 4;
            // Gradiente horizontal: vermelho à esquerda, azul à direita
            gradient[idx + 0] = 255 - (x * 255 / GRADIENT_SIZE); // R: 255 -> 0
            gradient[idx + 1] = 0;                                 // G: 0
            gradient[idx + 2] = (x * 255 / GRADIENT_SIZE);        // B: 0 -> 255
            gradient[idx + 3] = 255;                               // A: 255
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GRADIENT_SIZE, GRADIENT_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, gradient);
    free(gradient);
    
    printf("Textura gradiente vermelho-azul padrão criada (ID: %d)\n", textureID);
    return textureID;
}

unsigned int loadTexture(const char* filename) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Define parâmetros de wrapping e filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Carrega a imagem
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(1); // Inverte a imagem no eixo Y
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        printf("Textura carregada: %s (ID: %d, %dx%d, %d canais)\n", filename, textureID, width, height, nrChannels);
    } else {
        fprintf(stderr, "Falha ao carregar textura: %s\n", filename);
        stbi_image_free(data);
        return 0; // Retorna 0 em caso de falha
    }
    
    stbi_image_free(data);
    return textureID;
}
void loadMTL(const char* filename, const char* base_dir) {
    char mtl_path[1024];
    snprintf(mtl_path, sizeof(mtl_path), "%s/%s", base_dir, filename);

    FILE* file = fopen(mtl_path, "r");
    if (!file) {
        fprintf(stderr, "Aviso: Não foi possível abrir o arquivo .mtl: %s\n", mtl_path);
        return;
    }
    
    printf("Carregando materiais de %s...\n", mtl_path);

    char line[1024];
    int current_material = -1;

    while (fgets(line, sizeof(line), file)) {
        // Remove leading whitespace (tabs and spaces)
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        if (strncmp(trimmed, "newmtl ", 7) == 0) {
            char name[128];
            sscanf(trimmed, "newmtl %127s", name);
            current_material = add_material(name);
        }
        else if (current_material == -1) {
            continue; // Ignora se nenhum material estiver ativo
        }
        // Cor Ambiente
        else if (strncmp(trimmed, "Ka ", 3) == 0) {
            sscanf(trimmed, "Ka %f %f %f", 
                   &g_materials[current_material].ambient[0],
                   &g_materials[current_material].ambient[1],
                   &g_materials[current_material].ambient[2]);
            g_materials[current_material].ambient[3] = 1.0f;
        }
        // Cor Difusa
        else if (strncmp(trimmed, "Kd ", 3) == 0) {
            sscanf(trimmed, "Kd %f %f %f", 
                   &g_materials[current_material].diffuse[0],
                   &g_materials[current_material].diffuse[1],
                   &g_materials[current_material].diffuse[2]);
            g_materials[current_material].diffuse[3] = 1.0f;
        }
        // Cor Especular
        else if (strncmp(trimmed, "Ks ", 3) == 0) {
            sscanf(trimmed, "Ks %f %f %f", 
                   &g_materials[current_material].specular[0],
                   &g_materials[current_material].specular[1],
                   &g_materials[current_material].specular[2]);
            g_materials[current_material].specular[3] = 1.0f;
        }
        // Brilho (Shininess)
        else if (strncmp(trimmed, "Ns ", 3) == 0) {
            sscanf(trimmed, "Ns %f", &g_materials[current_material].shininess);
        }
        // Mapa de Textura (Difusa ou Ambiente)
        else if ((strncmp(trimmed, "map_Kd ", 7) == 0 || strncmp(trimmed, "map_Ka ", 7) == 0) 
                 && g_materials[current_material].texture_id == 0) {
            char tex_name[1024];
            if (strncmp(trimmed, "map_Kd ", 7) == 0) {
                sscanf(trimmed, "map_Kd %1023s", tex_name);
            } else {
                sscanf(trimmed, "map_Ka %1023s", tex_name);
            }
            
            char tex_path[2048];
            snprintf(tex_path, sizeof(tex_path), "%s/%s", base_dir, tex_name);
            
            g_materials[current_material].texture_id = loadTexture(tex_path);
        }
    }
    fclose(file);
}
/*
 * Função para carregar o .OBJ e calcular a Bounding Box
 */
 void loadOBJ(const char* filename) {
     // --- NOVO: Extrai o diretório base do arquivo .obj ---
     char base_dir[1024] = ".";
     char* path_copy = strdup(filename); // Duplica para poder modificar
     char* last_slash = strrchr(path_copy, '/');
     if (!last_slash) last_slash = strrchr(path_copy, '\\');
     
     if (last_slash) {
         *last_slash = '\0'; // Corta a string no último separador
         strncpy(base_dir, path_copy, sizeof(base_dir) - 1);
     }
     free(path_copy);
 
     printf("Carregando %s (Base dir: %s)...\n", filename, base_dir);
     
     FILE* file = fopen(filename, "r");
     if (!file) { // --- MODIFICADO: Checagem de arquivo movida para cima ---
         fprintf(stderr, "Erro: Não foi possível abrir %s\n", filename);
         exit(1);
     }
     
     atexit(cleanup);
 
     char line[1024];
     // Bounding box
     float min_v[3] = {1e9, 1e9, 1e9};
     float max_v[3] = {-1e9, -1e9, -1e9};
     
     int current_material_id = -1;
 
     while (fgets(line, sizeof(line), file)) {
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
         // --- NOVO: Carregar Coordenadas de Textura ---
         } else if (strncmp(line, "vt ", 3) == 0) {
             float u, v;
             if (sscanf(line, "vt %f %f", &u, &v) == 2) {
                 add_texcoord(u, v);
             }
         // --- NOVO: Carregar Biblioteca de Material ---
         } else if (strncmp(line, "mtllib ", 7) == 0) {
             char mtl_filename[1024];
             sscanf(line, "mtllib %1023s", mtl_filename);
             loadMTL(mtl_filename, base_dir); // Chama o parser de MTL
         // --- NOVO: Usar Material ---
         } else if (strncmp(line, "usemtl ", 7) == 0) {
             char mtl_name[128];
             sscanf(line, "usemtl %127s", mtl_name);
             current_material_id = find_material(mtl_name);
             if (current_material_id == -1) {
                  printf("Aviso: Material '%s' não encontrado.\n", mtl_name);
             }
         // --- Carregar Faces ---
         } else if (strncmp(line, "f ", 2) == 0) {
             int v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3;
             face_vertex_t fv0, fv1, fv2;
             
             // Zera os índices de textura/normal (para o formato "f v1 v2 v3")
             vt1 = vn1 = vt2 = vn2 = vt3 = vn3 = 0;
 
             // --- MODIFICADO: Parser de face mais robusto ---
             // Tenta ler formato v/vt/vn
             sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", 
                                  &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
             
             fv0 = (face_vertex_t){v1, vn1, vt1};
             fv1 = (face_vertex_t){v2, vn2, vt2};
             fv2 = (face_vertex_t){v3, vn3, vt3};
             // Adiciona a face com o ID do material atual
             add_face(fv0, fv1, fv2, current_material_id);
         }
     }
     fclose(file);
     
     if (g_num_vertices == 0) { // --- NOVO: Checagem se vértices foram lidos ---
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
     
     printf("Objeto carregado. Centro: (%.2f, %.2f, %.2f), Tamanho: %.2f\n", g_center[0], g_center[1], g_center[2], g_size);
     printf("Vértices: %lu, Normais: %lu, TexCoords: %lu, Faces: %lu, Materiais: %lu\n",
            g_num_vertices, g_num_normals, g_num_texcoords, g_num_faces, g_num_materials);
 }


 void myDisplay(void) {
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();
     gluLookAt(g_center[0], g_center[1], g_center[2] + g_size * 2.0, g_center[0], g_center[1], g_center[2], 0.0, 1.0, 0.0);                        
     
     GLfloat light_pos[] = {g_center[0], g_center[1] + g_size, g_center[2] + g_size, 1.0};
     glLightfv(GL_LIGHT0, GL_POSITION, light_pos);    
     
     glRotatef(g_rotateX, 1.0f, 0.0f, 0.0f);
     glRotatef(g_rotateY, 0.0f, 1.0f, 0.0f);
     glTranslatef(-g_center[0], -g_center[1], -g_center[2]);
     
     // Renderiza faces agrupadas por material (mais eficiente)
     for (int mat_id = -1; mat_id < (int)g_num_materials; mat_id++) {
         // Configura o material antes de desenhar
         if (mat_id >= 0 && mat_id < (int)g_num_materials) {
             material_t* mat = &g_materials[mat_id];
             glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   mat->ambient);
             glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   mat->diffuse);
             glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  mat->specular);
             glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat->shininess);
             glBindTexture(GL_TEXTURE_2D, mat->texture_id);
         } else {
             // Material padrão para faces sem material
             GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f};
             GLfloat black[] = {0.0f, 0.0f, 0.0f, 1.0f};
             glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, black);
             glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
             glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
             glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
             glBindTexture(GL_TEXTURE_2D, g_default_texture); // Usa textura padrão
         }
         
         // Desenha todas as faces com este material
         glBegin(GL_TRIANGLES);
         for (size_t i = 0; i < g_num_faces; i++) {
             face_t* f = &g_faces[i];
             
             // Pula se não for o material atual
             if (f->material_id != mat_id) continue;
             
             // Desenha os 3 vértices do triângulo
             for (int v = 0; v < 3; v++) {
                 face_vertex_t* fv = &f->v[v];
                 
                 int v_idx = fv->v_idx - 1;
                 int vn_idx = fv->vn_idx - 1;
                 int vt_idx = fv->vt_idx - 1;
     
                 // Define Coordenada de Textura (se existir)
                 if (vt_idx >= 0 && vt_idx < (int)g_num_texcoords) {
                     vec2f* tc = &g_texcoords[vt_idx];
                     glTexCoord2f(tc->u, tc->v); 
                 }
     
                 // Define a Normal (se existir)
                 if (vn_idx >= 0 && vn_idx < (int)g_num_normals) {
                     vec3f* n = &g_normals[vn_idx];
                     glNormal3f(n->x, n->y, n->z);
                 }
     
                 // Define o Vértice (se existir)
                 if (v_idx >= 0 && v_idx < (int)g_num_vertices) {
                     vec3f* vert = &g_vertices[v_idx];
                     glVertex3f(vert->x, vert->y, vert->z);
                 }
             }
         }
         glEnd();
     }
 
     glutSwapBuffers();
 }

void myReshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;

    gluPerspective(60.0, aspect, 0.1, g_size * 100.0); 
    glMatrixMode(GL_MODELVIEW);
}

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

void myMotion(int x, int y) { 
    if (g_isDragging) {
        int deltaX = x - g_lastX;
        int deltaY = y - g_lastY;

        g_rotateX += deltaY * 0.5f;
        g_rotateY += deltaX * 0.5f;

        g_lastX = x;
        g_lastY = y;

        glutPostRedisplay();
    }
}

int main(int argc, char** argv) {
    
    glutInit(&argc, argv);
    
    if (argc < 2) {
        printf("Erro: Forneça o caminho para o arquivo .obj\n");
        printf("Uso: %s <arquivo.obj>\n", argv[0]);
        return 1;
    }

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Trabalho Computacao grafica"); 

    loadOBJ(argv[1]);
    
    // Cria textura gradiente padrão para objetos sem textura (como o bunny)
    g_default_texture = createDefaultTexture();
    for (size_t i = 0; i < g_num_materials; i++) {
        if (g_materials[i].texture_id == 0) {
            g_materials[i].texture_id = g_default_texture;
        }
    }

    glEnable(GL_DEPTH_TEST); 
    glEnable(GL_LIGHTING);   
    glEnable(GL_LIGHT0);    
    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);

    GLfloat light_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient); 
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    
    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 50.0 };
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);


    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glutDisplayFunc(myDisplay); 
    glutReshapeFunc(myReshape);
    glutMouseFunc(myMouse);   
    glutMotionFunc(myMotion);   

    glutMainLoop();

    return 0; 
}