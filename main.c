#include <GL/glut.h> 
#include <GL/glu.h> 
#include <GL/gl.h>  
#include <stdio.h>  
#include <stdlib.h> 
#include <string.h> 
#include <math.h>   
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct {
    float x, y, z;
} vec3f;

typedef struct {
    float u, v;
} vec2f;

typedef struct {
    char name[128];
    unsigned int texture_id;
} material_t;

typedef struct {
    int v_idx;
    int vn_idx;
    int vt_idx;
} face_vertex_t;

typedef struct {
    face_vertex_t v[3];
    int material_id;
} face_t;

vec3f* g_vertices = NULL;
size_t g_num_vertices = 0;

vec3f* g_normals = NULL;
size_t g_num_normals = 0;

vec2f* g_texcoords = NULL;
size_t g_num_texcoords = 0;

face_t* g_faces = NULL;
size_t g_num_faces = 0;

material_t* g_materials = NULL;
size_t g_num_materials = 0;

int g_isDragging = 0; 
int g_lastX = 0, g_lastY = 0;
float g_rotateX = 0.0f;
float g_rotateY = 0.0f; 

float g_center[3] = {0.0f, 0.0f, 0.0f}; 
float g_size = 1.0f;

unsigned int g_default_texture = 0;

void cleanup() {
    if (g_vertices) free(g_vertices);
    if (g_normals) free(g_normals);
    if (g_texcoords) free(g_texcoords);
    if (g_faces) free(g_faces);
    if (g_materials) free(g_materials);
}

void add_vertex(float x, float y, float z) {
    g_vertices = (vec3f*)realloc(g_vertices, (g_num_vertices + 1) * sizeof(vec3f));
    g_vertices[g_num_vertices++] = (vec3f){x, y, z};
}

void add_normal(float nx, float ny, float nz) {
    g_normals = (vec3f*)realloc(g_normals, (g_num_normals + 1) * sizeof(vec3f));
    g_normals[g_num_normals++] = (vec3f){nx, ny, nz};
}

void add_texcoord(float u, float v) {
    g_texcoords = (vec2f*)realloc(g_texcoords, (g_num_texcoords + 1) * sizeof(vec2f));
    g_texcoords[g_num_texcoords++] = (vec2f){u, v};
}

void add_face(face_vertex_t v0, face_vertex_t v1, face_vertex_t v2, int material_id) {
    g_faces = (face_t*)realloc(g_faces, (g_num_faces + 1) * sizeof(face_t));
    g_faces[g_num_faces].v[0] = v0;
    g_faces[g_num_faces].v[1] = v1;
    g_faces[g_num_faces].v[2] = v2;
    g_faces[g_num_faces].material_id = material_id; 
    g_num_faces++;
}

int add_material(const char* name) {
    g_materials = (material_t*)realloc(g_materials, (g_num_materials + 1) * sizeof(material_t));
    strncpy(g_materials[g_num_materials].name, name, 127);
    g_materials[g_num_materials].name[127] = '\0';
    g_materials[g_num_materials].texture_id = 0;
    return g_num_materials++;
}

int find_material(const char* name) {
    for (size_t i = 0; i < g_num_materials; i++) {
        if (strcmp(g_materials[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

unsigned int createDefaultTexture() {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    #define GRADIENT_SIZE 256
    unsigned char* gradient = (unsigned char*)malloc(GRADIENT_SIZE * GRADIENT_SIZE * 4);
    
    for (int y = 0; y < GRADIENT_SIZE; y++) {
        for (int x = 0; x < GRADIENT_SIZE; x++) {
            int idx = (y * GRADIENT_SIZE + x) * 4;
            gradient[idx + 0] = 255 - (x * 255 / GRADIENT_SIZE);
            gradient[idx + 1] = 0;
            gradient[idx + 2] = (x * 255 / GRADIENT_SIZE);
            gradient[idx + 3] = 255;
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GRADIENT_SIZE, GRADIENT_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, gradient);
    free(gradient);
    
    return textureID;
}

unsigned int loadTexture(const char* filename) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        printf("Loaded texture: %s (ID: %d)\n", filename, textureID);
        stbi_image_free(data);
        return textureID;
    }
    
    printf("Failed to load texture: %s\n", filename);
    stbi_image_free(data);
    return 0;
}

void loadMTL(const char* filename, const char* base_dir) {
    char mtl_path[1024];
    snprintf(mtl_path, sizeof(mtl_path), "%s/%s", base_dir, filename);

    FILE* file = fopen(mtl_path, "r");
    if (!file) return;

    char line[1024];
    int current_material = -1;

    while (fgets(line, sizeof(line), file)) {
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        if (strncmp(trimmed, "newmtl ", 7) == 0) {
            char name[128];
            sscanf(trimmed, "newmtl %127s", name);
            current_material = add_material(name);
        }
        else if (current_material >= 0 && strncmp(trimmed, "map_Kd ", 7) == 0) {
            char tex_name[1024];
            sscanf(trimmed, "map_Kd %1023s", tex_name);
            
            char tex_path[2048];
            snprintf(tex_path, sizeof(tex_path), "%s/%s", base_dir, tex_name);
            
            printf("Loading texture for material '%s': %s\n", g_materials[current_material].name, tex_path);
            g_materials[current_material].texture_id = loadTexture(tex_path);
        }
    }
    fclose(file);
}

void loadOBJ(const char* filename) {
    char base_dir[1024] = ".";
    char* path_copy = strdup(filename);
    char* last_slash = strrchr(path_copy, '/');
    if (!last_slash) last_slash = strrchr(path_copy, '\\');
    
    if (last_slash) {
        *last_slash = '\0';
        strncpy(base_dir, path_copy, sizeof(base_dir) - 1);
    }
    free(path_copy);

    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Erro: Não foi possível abrir %s\n", filename);
        exit(1);
    }
    
    atexit(cleanup);

    char line[1024];
    float min_v[3] = {1e9, 1e9, 1e9};
    float max_v[3] = {-1e9, -1e9, -1e9};
    int current_material_id = -1;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "v ", 2) == 0) {
            float x, y, z;
            sscanf(line, "v %f %f %f", &x, &y, &z);
            add_vertex(x, y, z);
            
            if (x < min_v[0]) min_v[0] = x;
            if (x > max_v[0]) max_v[0] = x;
            if (y < min_v[1]) min_v[1] = y;
            if (y > max_v[1]) max_v[1] = y;
            if (z < min_v[2]) min_v[2] = z;
            if (z > max_v[2]) max_v[2] = z;
        }
        else if (strncmp(line, "vn ", 3) == 0) {
            float nx, ny, nz;
            sscanf(line, "vn %f %f %f", &nx, &ny, &nz);
            add_normal(nx, ny, nz);
        }
        else if (strncmp(line, "vt ", 3) == 0) {
            float u, v;
            sscanf(line, "vt %f %f", &u, &v);
            add_texcoord(u, v);
        }
        else if (strncmp(line, "mtllib ", 7) == 0) {
            char mtl_filename[1024];
            sscanf(line, "mtllib %1023s", mtl_filename);
            loadMTL(mtl_filename, base_dir);
        }
        else if (strncmp(line, "usemtl ", 7) == 0) {
            char mtl_name[128];
            sscanf(line, "usemtl %127s", mtl_name);
            current_material_id = find_material(mtl_name);
        }
        else if (strncmp(line, "f ", 2) == 0) {
            int v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3;
            sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", 
                   &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
            
            face_vertex_t fv0 = {v1, vn1, vt1};
            face_vertex_t fv1 = {v2, vn2, vt2};
            face_vertex_t fv2 = {v3, vn3, vt3};
            
            add_face(fv0, fv1, fv2, current_material_id);
        }
    }
    fclose(file);

    g_center[0] = (min_v[0] + max_v[0]) / 2.0f;
    g_center[1] = (min_v[1] + max_v[1]) / 2.0f;
    g_center[2] = (min_v[2] + max_v[2]) / 2.0f;

    float size_x = max_v[0] - min_v[0];
    float size_y = max_v[1] - min_v[1];
    float size_z = max_v[2] - min_v[2];

    g_size = fmax(fmax(fabs(size_x), fabs(size_y)), fabs(size_z));
}

void myDisplay(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    gluLookAt(g_center[0], g_center[1], g_center[2] + g_size * 2.0, 
              g_center[0], g_center[1], g_center[2], 
              0.0, 1.0, 0.0);
    
    GLfloat light_pos[] = {g_center[0], g_center[1] + g_size, g_center[2] + g_size, 1.0};
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    
    glRotatef(g_rotateX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_rotateY, 0.0f, 1.0f, 0.0f);
    glTranslatef(-g_center[0], -g_center[1], -g_center[2]);
    
    for (size_t i = 0; i < g_num_faces; i++) {
        face_t* f = &g_faces[i];
        
        if (f->material_id >= 0) {
            glBindTexture(GL_TEXTURE_2D, g_materials[f->material_id].texture_id);
        } else {
            glBindTexture(GL_TEXTURE_2D, g_default_texture);
        }
        
        glBegin(GL_TRIANGLES);
        for (int v = 0; v < 3; v++) {
            face_vertex_t* fv = &f->v[v];
            
            int vt_idx = fv->vt_idx - 1;
            int vn_idx = fv->vn_idx - 1;
            int v_idx = fv->v_idx - 1;

            glTexCoord2f(g_texcoords[vt_idx].u, g_texcoords[vt_idx].v);
            glNormal3f(g_normals[vn_idx].x, g_normals[vn_idx].y, g_normals[vn_idx].z);
            glVertex3f(g_vertices[v_idx].x, g_vertices[v_idx].y, g_vertices[v_idx].z);
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
        printf("Uso: %s <arquivo.obj>\n", argv[0]);
        return 1;
    }

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Trabalho Computacao grafica"); 

    loadOBJ(argv[1]);
    
    g_default_texture = createDefaultTexture();
    printf("\nAssigning default texture to materials without textures:\n");
    for (size_t i = 0; i < g_num_materials; i++) {
        if (g_materials[i].texture_id == 0) {
            printf("  Material '%s' -> default gradient\n", g_materials[i].name);
            g_materials[i].texture_id = g_default_texture;
        } else {
            printf("  Material '%s' -> texture ID %d\n", g_materials[i].name, g_materials[i].texture_id);
        }
    }

    glEnable(GL_DEPTH_TEST); 
    glEnable(GL_LIGHTING);   
    glEnable(GL_LIGHT0);    
    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);

    GLfloat light_ambient[] = {0.2, 0.2, 0.2, 1.0};
    GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
    GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient); 
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glutDisplayFunc(myDisplay); 
    glutReshapeFunc(myReshape);
    glutMouseFunc(myMouse);   
    glutMotionFunc(myMotion);   

    glutMainLoop();

    return 0; 
}