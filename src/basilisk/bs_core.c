// GL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

// Basilisk
#include <bs_shaders.h>
#include <bs_core.h>
#include <bs_textures.h>
#include <bs_debug.h>

// STD
#include <string.h>
#include <stddef.h>
#include <windows.h>
#include <lodepng.h>

float screen_quad_vertices[] = {
    // positions   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};

struct bs_Window {
    int width;
    int height;

    // Pointers to all framebuffers
    bs_Framebuffer **framebuffers;
    int allocated_framebuffer_count;
    int framebuffer_count;
} bs_window;

GLFWwindow *window;
bs_fRGBA clear_color = { 0.0, 0.0, 0.0, 0.0 };

bs_Framebuffer std_framebuffer;

// Shaders
bs_Shader fbo_shader;
bs_Shader texture_shader;

bs_Camera std_camera;
bs_Batch *curr_batch;
bs_Tex2D empty_texture;

float elapsed_time = 0.0;

/* --- WINDOW SETTINGS --- */
void bs_initGLFW(bs_WNDSettings settings) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if(settings != bs_WND_DEFAULT) {
        if((settings & bs_WND_TRANSPARENT) == bs_WND_TRANSPARENT) {
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
        }
        if((settings & bs_WND_NO_TITLE_BAR) == bs_WND_NO_TITLE_BAR) {
            glfwWindowHint(GLFW_DECORATED, false);
        }
        if((settings & bs_WND_TOPMOST) == bs_WND_TOPMOST) {
            glfwWindowHint(GLFW_FLOATING, true);
        }
        if((settings & bs_WND_INVISIBLE) == bs_WND_INVISIBLE) {
            glfwWindowHint(GLFW_VISIBLE, false);
        }
        if((settings & bs_WND_UNCLICKABLE) == bs_WND_UNCLICKABLE) {
            glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, true);
        }
    }
}

void bs_createWindow(int width, int height, char* title) {
    bs_window.width = width;
    bs_window.height = height;

    window = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGL();
    glViewport(0, 0, width, height);
}

// TODO: Extract to bs_debug.c
void bs_printHardwareInfo() {
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    printf("%s\n", vendor);
    printf("%s\n", renderer);
}

void bs_setBackgroundColor(bs_fRGBA color) {
    clear_color = (bs_fRGBA){ color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0 };
}

bs_vec2 bs_getWindowDimensions() {
    return (bs_vec2){ bs_window.width, bs_window.height };
}

/* --- INPUTS/CALLBACKS --- */
bool key_states[350];

void bs_onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        key_states[key] = true;
        return;
    }
    if (action == GLFW_RELEASE) {
        key_states[key] = false;
        return;
    }
}

bool bs_isKeyDown(int key) {
    return key_states[key];
}

bool bs_isKeyUp(int key) {
    return !key_states[key];
}

bool bs_isKeyDownOnce(int key) {
    bool result = key_states[key] != false;
    key_states[key] = false;

    return result;
}

bool bs_isKeyUpOnce(int key) {
    bool result = key_states[key] != true;
    key_states[key] = true;

    return result;
}

bs_vec2 bs_getCursorPositionReverseY() {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return (bs_vec2){ xpos, ypos };
}

bs_vec2 bs_getCursorPosition() {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return (bs_vec2){ xpos, bs_window.height - ypos };
}

/* --- MATRICES --- */
void bs_setProjMatrixOrtho(bs_Camera *cam, int left, int right, int bottom, int top) {
    glm_ortho(left, right, bottom, top, 0.1, 100.0, cam->proj);
    glm_translate(cam->view, (vec3){ 0.0, 0.0, -1.0 });
}

void bs_setViewMatrixOrtho(bs_Camera *cam) {
    float cpy_matrix[4][4] = GLM_MAT4_IDENTITY_INIT;
    memcpy(cam->view, cpy_matrix, 4 * 4 * sizeof(float));
}

void bs_createOrthographicProjection(bs_Camera *cam, int left, int right, int bottom, int top) {
    bs_setViewMatrixOrtho(cam);
    bs_setProjMatrixOrtho(cam, 0, right, 0, top);
}

void bs_setMatrixLookat(bs_Camera *cam, bs_vec3 center, bs_vec3 up) {
    glm_lookat((vec3){ cam->pos.x, cam->pos.y, cam->pos.z }, (vec3){ center.x, center.y, center.z }, (vec3){ up.x, up.y, up.z }, cam->view);
}

void bs_setMatrixLook(bs_Camera *cam, bs_vec3 dir, bs_vec3 up) {
    glm_look((vec3){ cam->pos.x, cam->pos.y, cam->pos.z }, (vec3){ dir.x, dir.y, dir.z }, (vec3){ up.x, up.y, up.z }, cam->view);
}

void bs_setPerspectiveProjection(bs_Camera *cam, float fovy, float aspect, float nearZ, float farZ) {
    glm_perspective(fovy, aspect, nearZ, farZ, cam->proj);
}

bs_Camera *bs_getStdCamera() {
    return &std_camera;
}

/* --- BATCHING --- */
void bs_changeBatchBufferSize(bs_Batch *batch) {
    
}

void bs_selectBatch(bs_Batch *batch) {
    curr_batch = batch;

    glBindVertexArray(batch->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->EBO);
}

void bs_setBatchShader(bs_Batch *batch, bs_Shader *shader) {
    batch->shader = shader;
}

void bs_pushVertex(float px, float py, float pz, float tx, float ty, float nx, float ny, float nz, bs_RGBA color) {
    bs_Vertex *vertex = &curr_batch->vertices[curr_batch->vertex_draw_count++];

    vertex->position.x = px;
    vertex->position.y = py;
    vertex->position.z = pz;

    vertex->tex_coord.x = tx;
    vertex->tex_coord.y = ty;

    // TODO: Make these optional
    vertex->normal.x = nx;
    vertex->normal.y = ny;
    vertex->normal.z = nz;

    vertex->color = color;
}

// Adds a quad to the vertex array in the chosen batch
void bs_pushQuad(bs_vec2 pos, bs_vec2 dim, bs_RGBA color) {
    bs_Tex2D *curr_texture = bs_getSelectedTexture();

    dim.x += pos.x;
    dim.y += pos.y;
    // dim.x = pos.x + curr_texture->w * scale;
    // dim.y = pos.y + curr_texture->h * scale;

    bs_pushVertex(pos.x, pos.y, 0.0, curr_texture->tex_x , curr_texture->tex_hy, 0.0, 0.0, 0.0, color); // Bottom Left
    bs_pushVertex(dim.x, pos.y, 0.0, curr_texture->tex_wx, curr_texture->tex_hy, 0.0, 0.0, 0.0, color); // Bottom right
    bs_pushVertex(pos.x, dim.y, 0.0, curr_texture->tex_x , curr_texture->tex_y , 0.0, 0.0, 0.0, color); // Top Left
    bs_pushVertex(dim.x, dim.y, 0.0, curr_texture->tex_wx, curr_texture->tex_y , 0.0, 0.0, 0.0, color); // Top Right

    int indices[] = {
        curr_batch->index_draw_unique_count+0, curr_batch->index_draw_unique_count+1, curr_batch->index_draw_unique_count+2,
        curr_batch->index_draw_unique_count+1, curr_batch->index_draw_unique_count+2, curr_batch->index_draw_unique_count+3,
    };

    memcpy(&curr_batch->indices[curr_batch->index_draw_count], indices, 6 * sizeof(int));

    curr_batch->index_draw_unique_count += 4;
    curr_batch->index_draw_count += 6;
}

void bs_pushTriangle(bs_vec2 pos1, bs_vec2 pos2, bs_vec2 pos3, bs_RGBA color) {
    bs_pushVertex(pos1.x, pos1.y, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, color);
    bs_pushVertex(pos2.x, pos2.y, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, color);
    bs_pushVertex(pos3.x, pos3.y, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, color);

    int indices[] = {
        curr_batch->index_draw_unique_count+0, curr_batch->index_draw_unique_count+1, curr_batch->index_draw_unique_count+2,
    };

    memcpy(&curr_batch->indices[curr_batch->index_draw_count], indices, 3 * sizeof(int));

    curr_batch->index_draw_unique_count += 3;
    curr_batch->index_draw_count += 3;
}

void bs_createDynamicBatch(bs_Batch *batch) {
    glGenVertexArrays(1, &batch->VAO);
    glGenBuffers(3, &batch->VBO);
    glGenBuffers(1, &batch->EBO);

    bs_selectBatch(batch);
}

void bs_initStaticBatch(bs_Batch *batch) {
    batch->index_draw_count = batch->index_draw_unique_count = batch->vertex_draw_count = 0;

    batch->indices = malloc(sizeof(int) * 6 * batch->max_vertex_count);
    batch->vertices = malloc(sizeof(bs_Vertex) * 4 * batch->max_vertex_count);

    glGenVertexArrays(1, &batch->VAO);
    glGenBuffers(1, &batch->VBO);
    glGenBuffers(1, &batch->EBO);

    bs_selectBatch(batch);

    glBufferData(GL_ARRAY_BUFFER, sizeof(bs_Vertex) * 4 * batch->max_vertex_count, NULL, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * 6 * batch->max_vertex_count, NULL, GL_STATIC_DRAW);
}

void bs_createBatch(bs_Batch *batch, int vertex_amount, int batch_type, int attrib_type) {
    batch->max_vertex_count = vertex_amount;
    batch->camera = &std_camera;
    batch->draw_mode = BS_TRIANGLES;

    // Handle unique batch properties
    if(batch_type == BS_STATIC_BATCH) {
        bs_initStaticBatch(batch);
    }
    if(batch_type == BS_DYNAMIC_BATCH) {

    }

    if(attrib_type == BS_POSITION_COLOR) {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(bs_Vertex), (void*)offsetof(bs_Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(bs_Vertex), (void*)offsetof(bs_Vertex, tex_coord));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(bs_Vertex), (void*)offsetof(bs_Vertex, normal));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(bs_Vertex), (void*)offsetof(bs_Vertex, color));
    }
    if(attrib_type == BS_POSITION_TEXTURE) {

    }

    bs_setBatchShader(batch, &texture_shader);
}

// Pushes all vertices to VRAM
void bs_pushBatch() {
    curr_batch->draw_count_non_resetting = curr_batch->index_draw_count;

    // Batch should already be bound at this point so binding it again is a waste of resources
    glBufferSubData(GL_ARRAY_BUFFER, 0, curr_batch->vertex_draw_count * sizeof(bs_Vertex), curr_batch->vertices);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, curr_batch->index_draw_count * sizeof(int), curr_batch->indices);
}

void bs_freeBatchData() {
    free(curr_batch->vertices);
    free(curr_batch->indices);
}

void bs_renderBatch(int start_index, int draw_count) {
    // Batch should still be bound here
    bs_switchShader(curr_batch->shader);
    bs_setTimeUniform(curr_batch->shader, elapsed_time);
    bs_setViewMatrixUniform(curr_batch->shader, curr_batch->camera);
    bs_setProjMatrixUniform(curr_batch->shader, curr_batch->camera);

    glDrawElements(curr_batch->draw_mode, draw_count, GL_UNSIGNED_INT, (void*)(start_index * 6 * sizeof(GLuint)));


    // Reset draw counts
    curr_batch->vertex_draw_count = 0;
    curr_batch->index_draw_count = 0;
    curr_batch->index_draw_unique_count = 0;
}

int bs_getBatchSize(bs_Batch *batch) {
    return batch->draw_count_non_resetting;
}

/* --- FRAMEBUFFERS --- */
void bs_createFramebufferTexImage(bs_Framebuffer *framebuffer) {
    glGenTextures(1, &framebuffer->texture_color_buffer);
    glBindTexture(GL_TEXTURE_2D, framebuffer->texture_color_buffer);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer->render_width, framebuffer->render_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Attach it to currently bound framebuffer object
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->texture_color_buffer, 0);
}

void bs_setFramebufferVertices(bs_Framebuffer *framebuffer) {
    glGenVertexArrays(1, &framebuffer->VAO);
    glGenBuffers(1, &framebuffer->VBO);

    glBindVertexArray(framebuffer->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, framebuffer->VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(screen_quad_vertices), &screen_quad_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void bs_createFramebuffer(bs_Framebuffer *framebuffer, int render_width, int render_height, void (*render)(), bs_Shader *shader) {
    framebuffer->render_width  = render_width;
    framebuffer->render_height = render_height;
    framebuffer->render = render;

    // Allocate 4 more framebuffers if the framebuffer count goes out of range
    if((bs_window.framebuffer_count + 1) > bs_window.allocated_framebuffer_count) {
        bs_window.allocated_framebuffer_count += 4;
        bs_window.framebuffers = realloc(bs_window.framebuffers, bs_window.allocated_framebuffer_count * sizeof(bs_Framebuffer));
    }

    bs_window.framebuffers[bs_window.framebuffer_count++] = framebuffer;

    bs_setFramebufferShader(framebuffer, shader);

    glGenFramebuffers(1, &framebuffer->FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->FBO);

    bs_createFramebufferTexImage(framebuffer);
    bs_setFramebufferVertices(framebuffer);
}

void bs_setFramebufferShader(bs_Framebuffer *framebuffer, bs_Shader *shader) {
    framebuffer->shader = shader;
}

void bs_startFramebufferRender(bs_Framebuffer *framebuffer) {
    // Bind
    glBindVertexArray(framebuffer->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, framebuffer->VBO);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->FBO);

    // Clear any previous drawing
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    framebuffer->render();
}

void bs_endFramebufferRender(bs_Framebuffer *framebuffer) {
    bs_switchShader(framebuffer->shader);
    bs_setTimeUniform(framebuffer->shader, elapsed_time);

    // Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(framebuffer->VAO);
    glDisable(GL_DEPTH_TEST);

    // Render
    glBindTexture(GL_TEXTURE_2D, framebuffer->texture_color_buffer);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void bs_render() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    double previousTime = glfwGetTime();
    int frameCount = 0;

    for(int i = 0; i < 350; i++) {
        key_states[i] = false;
    }

    glfwSetKeyCallback(window, bs_onKey);

    while(!glfwWindowShouldClose(window)) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        elapsed_time += 0.01;

        // Measure speed
        double currentTime = glfwGetTime();
        frameCount++;
        // If a second has passed.
        if (currentTime - previousTime >= 1.0) {
            // Display the frame count here any way you want.
            printf("%d\n", frameCount);

            frameCount = 0;
            previousTime = currentTime;
        }

        // Render all framebuffers
        for(int i = 0; i < bs_window.framebuffer_count; i++) {
            bs_Framebuffer *framebuffer = bs_window.framebuffers[i];
            bs_startFramebufferRender(framebuffer);
            bs_endFramebufferRender(framebuffer);
        }

        GLenum err = glGetError();
        if(err != GL_NO_ERROR) {
            switch(err) {
                case GL_INVALID_ENUM                 : printf("INVALID_ENUM"); break;
                case GL_INVALID_VALUE                : printf("INVALID_VALUE"); break;
                case GL_INVALID_OPERATION            : printf("GL_INVALID_OPERATION"); break;
                case GL_STACK_OVERFLOW               : printf("GL_STACK_OVERFLOW"); break;
                case GL_STACK_UNDERFLOW              : printf("GL_STACK_UNDERFLOW"); break;
                case GL_OUT_OF_MEMORY                : printf("OUT_OF_MEMORY"); break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: printf("INVALID_FRAMEBUFFER_OPERATION"); break;
            }
            printf(" | 0x0%x\n", err);
        }

        glfwPollEvents();
        glfwSwapBuffers(window);
    }
}

void bs_exitGLFW() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void bs_init(int width, int height, char *title, bs_WNDSettings settings) {
    bs_initGLFW(settings);

    // Set default texture to be empty
    bs_Tex2D *tex = bs_getSelectedTexture();
    empty_texture.w = empty_texture.h = 0;
    empty_texture.x = empty_texture.y = 0;
    empty_texture.tex_x = empty_texture.tex_y = 0;
    empty_texture.tex_wx = empty_texture.tex_hy = 0;
    empty_texture.data = NULL;
    bs_selectTexture(&empty_texture);

    // Allocate 4 framebuffers by default, this auto-increments
    bs_window.allocated_framebuffer_count = 4;
    bs_window.framebuffer_count = 0;
    bs_window.framebuffers = malloc(bs_window.allocated_framebuffer_count * sizeof(bs_Framebuffer*));

    bs_createWindow(width, height, title);
    bs_printHardwareInfo();

    std_camera.pos.x = 0.0;
    std_camera.pos.y = 0.0;
    std_camera.pos.z = 200.0;
    bs_setMatrixLookat(&std_camera, (bs_vec3){ 0.0, 0.0, 2.0 }, (bs_vec3){ 0.0, 1.0, 0.0 });
    bs_setPerspectiveProjection(&std_camera, 90.0, (float)width / (float)height, 0.1, 5000.0);

    printf("STDCAM\n");
    for(int y = 0; y < 4; y++) {
        for(int x = 0; x < 4; x++) {
            printf("%f\n", std_camera.proj[x][y]);
            printf("%f\n", std_camera.view[x][y]);
            printf("\n");
        }
    }
}

void bs_startRender(void (*render)()) {
    // Create the default framebuffer
    fbo_shader = bs_loadShader("resources/fbo_shader.vs", "resources/fbo_shader.fs", 0);
    bs_createFramebuffer(&std_framebuffer, bs_window.width, bs_window.height, render, &fbo_shader);

    // Load default shaders
    texture_shader = bs_loadShader("resources/bs_color_shader.vs", "resources/bs_color_shader.fs", 0);

    bs_render();
    bs_exitGLFW();
}