# README - Próximos Passos (Visualizador OBJ)

Nosso programa base está funcional. [cite\_start]Para finalizar o trabalho, precisamos adicionar suporte para materiais (`.mtl`) e texturas (imagens), conforme solicitado[cite: 3, 6].

## 1\. Carregar Materiais (.MTL)

Atualmente, nosso objeto é renderizado com um material branco padrão. Precisamos ler o arquivo `.mtl` para aplicar as cores e propriedades corretas.

O arquivo `.obj` especifica qual arquivo `.mtl` usar através da tag `mtllib`.

#### 1.1. Modificar `loadOBJ`

1.  **Detectar `mtllib`:** Durante o *parsing* do `.obj`, procure pela linha:
    `mtllib <nome_do_arquivo.mtl>`
2.  **Chamar o Parser de MTL:** Quando encontrar, chame uma nova função que criaremos: `loadMTL(nome_do_arquivo.mtl)`.
3.  **Detectar `usemtl`:** Procure pela linha:
    `usemtl <nome_do_material>`
4.  **Associar Material à Face:** Armazene o ID do material que está sendo usado. Quando você chamar `add_face`, passe este ID de material para que a face saiba qual material ela usa.

#### 1.2. Criar a Função `loadMTL`

Esta nova função irá ler o arquivo `.mtl`.

1.  **Estrutura de Material:** Crie uma `struct` global para armazenar os materiais.

    ```c
    typedef struct {
        char name[128];
        float ambient[4];
        float diffuse[4];
        float specular[4];
        float shininess; // Expoente especular
    } material_t;

    material_t* g_materials = NULL;
    size_t g_num_materials = 0;
    ```

2.  **Lógica do Parser (MTL):**

      * `newmtl <name>`: Inicia um novo material e o adiciona ao array `g_materials`.
      * [cite\_start]`Ka <r> <g> <b>`: Lê a cor **Ambiente** (salva em `ambient`)[cite: 1469].
      * [cite\_start]`Kd <r> <g> <b>`: Lê a cor **Difusa** (salva em `diffuse`)[cite: 1469].
      * [cite\_start]`Ks <r> <g> <b>`: Lê a cor **Especular** (salva em `specular`)[cite: 1469].
      * [cite\_start]`Ns <value>`: Lê o **Brilho** (expoente especular, `shininess`)[cite: 1490, 1517].

#### 1.3. Modificar `myDisplay`

1.  **Aplicar Material:** Antes de desenhar os triângulos (`glBegin`), descubra qual material a face usa.

2.  [cite\_start]**Chamar `glMaterialfv`:** Use as funções do OpenGL para definir as propriedades do material[cite: 1495, 1515, 1517].

    ```c
    // Dentro do loop de renderização (myDisplay)

    // Antes de glBegin(GL_TRIANGLES)
    material_t* mat = &g_materials[id_do_material_da_face];

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   mat->ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   mat->diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  mat->specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat->shininess);
    ```

-----

## 2\. Adicionar Suporte a Texturas

[cite\_start]Esta é a etapa final da renderização[cite: 6]. A textura é definida no arquivo `.mtl` (geralmente com a tag `map_Kd`).

#### 2.1. Modificar Parser de `loadOBJ`

1.  **Ler Coordenadas de Textura (`vt`):** Nosso parser atual ignora o `vt`. Precisamos lê-lo.

    ```c
    // Adicionar uma nova struct e array global
    typedef struct {
        float u, v;
    } vec2f;

    vec2f* g_texcoords = NULL;
    size_t g_num_texcoords = 0;
    // ... (adicionar lógica de alocação similar ao add_vertex)

    // No loop de loadOBJ:
    if (strncmp(line, "vt ", 3) == 0) {
        float u, v;
        if (sscanf(line, "vt %f %f", &u, &v) == 2) {
            add_texcoord(u, v);
        }
    }
    ```

2.  **Armazenar `vt_idx`:** Nosso parser de faces (`f`) já lê o `vt_idx` (no formato `v/vt/vn`), então estamos prontos.

#### 2.2. Modificar Parser de `loadMTL`

1.  **Ler `map_Kd`:** No parser de MTL, procure pela tag:
    `map_Kd <nome_da_imagem.png>`
2.  **Carregar Imagem:** Quando encontrar, chame uma nova função: `loadTexture(nome_da_imagem.png)`.
3.  **Armazenar ID da Textura:** Armazene o `GLuint` (ID da textura) retornado na sua `struct material_t`.

#### 2.3. Criar a Função `loadTexture`

[cite\_start]O trabalho permite usar bibliotecas para carregar imagens[cite: 6]. Recomendo usar **`stb_image.h`** (uma biblioteca de cabeçalho único, assim como `tinyobjloader-c`), pois é muito mais simples que `libdevil`.

1.  Baixe `stb_image.h` e inclua-o.
2.  Sua função `loadTexture` deve:
      * Carregar a imagem: `unsigned char *data = stbi_load(filename, &width, &height, &channels, 0);`
      * [cite\_start]Gerar uma textura OpenGL: `glGenTextures(1, &textureID);` [cite: 1569]
      * [cite\_start]Enviar a imagem para a GPU: `glBindTexture(GL_TEXTURE_2D, textureID);` [cite: 1569]
      * [cite\_start]`glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);` [cite: 1570, 1572, 1574]
      * Definir parâmetros (wrapping/filtering).
      * Liberar a imagem da CPU: `stbi_image_free(data);`
      * Retornar o `textureID`.

#### 2.4. Modificar `myDisplay`

1.  **Habilitar/Desabilitar Texturas:**

      * [cite\_start]Em `main`, chame `glEnable(GL_TEXTURE_2D);` uma vez[cite: 1569].
      * Dentro do loop de `myDisplay`, antes de desenhar:
          * Se o material da face *tem* um `textureID`: `glBindTexture(GL_TEXTURE_2D, mat->textureID);`
          * Se o material da face *não tem* textura: `glBindTexture(GL_TEXTURE_2D, 0);` (desvincula).

2.  **Aplicar Coordenadas (`glTexCoord`)**:

      * [cite\_start]Dentro do `glBegin(GL_TRIANGLES)`, *antes* de cada `glVertex3f`, você deve passar a coordenada de textura[cite: 1577].

    <!-- end list -->

    ```c
    // Dentro do loop de vértices (v = 0 a 2)
    face_vertex_t* fv = &f->v[v];
    int vt_idx = fv->vt_idx - 1;

    if (vt_idx >= 0 && vt_idx < g_num_texcoords) {
        vec2f* tc = &g_texcoords[vt_idx];
        glTexCoord2f(tc->u, tc->v); // Define a coordenada de textura
    }

    // ... (glNormal3f e glVertex3f vêm aqui) ...
    ```

-----

## 3\. Testar e Escrever o Relatório

Depois que as texturas estiverem funcionando, a parte de código estará concluída. [cite\_start]A etapa final é testar e escrever seu relatório[cite: 9].

1.  [cite\_start]**Testar Modelos:** Baixe pelo menos 3 modelos da lista sugerida[cite: 18].
      * [cite\_start]**Bule de Chá (Teapot)**[cite: 28]: Ótimo para testar materiais (sem textura).
      * [cite\_start]**Coelho de Stanford (Bunny)**[cite: 20]: Ótimo para testar malhas complexas (sem materiais ou texturas).
      * [cite\_start]**Buda Feliz (Buddha)** [cite: 19] [cite\_start]ou **Dragão Chinês (Dragon)**[cite: 22]: Também são bons testes de malha.
      * [cite\_start]**Cornell Box**[cite: 21]: Pode falhar no nosso parser se tiver faces com 4 vértices (quads), a menos que você baixe uma versão triangulada.
2.  [cite\_start]**Escrever o Relatório (1 página):** [cite: 9]
      * [cite\_start]Nome e Matrícula[cite: 10].
      * [cite\_start]Instruções de compilação (ex: `gcc -o seu_prog main.c -lglut -lGLU -lGL -lm`)[cite: 11].
      * [cite\_start]Instruções de execução (ex: `./seu_prog modelos/bule.obj`)[cite: 11].
      * [cite\_start]Liste os 3+ modelos que você testou com sucesso[cite: 12].
      * [cite\_start]Liste outras bibliotecas (ex: `stb_image.h` se você usou)[cite: 13].
      * [cite\_start]Informações relevantes (ex: "O parser só suporta faces triangulares.")[cite: 14].