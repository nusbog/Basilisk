#ifndef BS_CORE_H
#define BS_CORE_H

#include <bs_shaders.h>
#include <glad/glad.h>

typedef enum {
	bs_WND_DEFAULT = 0,

	bs_WND_TRANSPARENT = 1,
	bs_WND_NO_TITLE_BAR = 2,
	bs_WND_TOPMOST = 4,
	bs_WND_INVISIBLE = 8,
	bs_WND_UNCLICKABLE = 16,
} bs_WNDSettings;

typedef struct {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} bs_RGBA;

typedef struct {
	float r;
	float g;
	float b;
	float a;
} bs_fRGBA;

typedef struct {
	float x;
	float y;
} bs_vec2;

typedef struct {
	float x;
	float y;
	float z;
} bs_vec3;

typedef struct {
	float x;
	float y;
	float z;
	float w;
} bs_vec4;

typedef struct {
	float view[4][4];
	float proj[4][4];
	bs_vec3 pos;
} bs_Camera;

typedef struct {
	bs_vec3 position;
	bs_vec2 tex_coord;
	bs_vec3 normal;
	bs_RGBA color;
} bs_Vertex;

typedef struct {
	int render_width;
	int render_height;

	bs_Shader *shader;
	void (*render)();

	unsigned int VAO, VBO;

	unsigned int FBO, RBO;
	unsigned int texture_color_buffer;
} bs_Framebuffer;

// Contains all objects queued to render the next frame (unless using multiple batches)
typedef struct {
	bs_Shader *shader;
	bs_Vertex *vertices;
	bs_Camera *camera;

	int *indices;
	int draw_mode;
	int max_vertex_count;
	int draw_count_non_resetting;
	int vertex_draw_count;
	// Amounf of indices (6 per quad)
	int index_draw_count;
	// Amount of differing indices (4 per quad)
	int index_draw_unique_count;

	unsigned int VAO, VBO, EBO;
} bs_Batch;

/* --- RENDERING --- */
void bs_createFramebuffer(bs_Framebuffer *framebuffer, int render_width, int render_height, void (*render)(), bs_Shader *shader);
void bs_setFramebufferShader(bs_Framebuffer *framebuffer, bs_Shader *shader);

void bs_pushVertex(float px, float py, float pz, float tx, float ty, float nx, float ny, float nz, bs_RGBA color);
void bs_pushQuad(bs_vec2 pos, bs_vec2 dim, bs_RGBA color);
void bs_pushTriangle(bs_vec2 pos1, bs_vec2 pos2, bs_vec2 pos3, bs_RGBA color);
void bs_pushCube();

void bs_createBatch(bs_Batch *batch, int vertex_amount, int batch_type, int attrib_type);

void bs_setBatchShader(bs_Batch *batch, bs_Shader *shader);
void bs_selectBatch(bs_Batch *batch);

void bs_pushBatch();
void bs_freeBatchData();
void bs_renderBatch(int start_index, int draw_count);
int bs_getBatchSize(bs_Batch *batch);

/* --- WINDOW INITIALIZATION --- */
void bs_init(int width, int height, char *title, bs_WNDSettings settings);
void bs_startRender(void (*render)());
void bs_setBackgroundColor(bs_fRGBA color);
bs_vec2 bs_getWindowDimensions();

/* --- INPUTS/CALLBACKS --- */
bool bs_isKeyDown(int key);
bool bs_isKeyUp(int key);
bool bs_isKeyDownOnce(int key);
bool bs_isKeyUpOnce(int key);
bs_vec2 bs_getCursorPositionReverseY();
bs_vec2 bs_getCursorPosition();

/* MATRICES */
void bs_setMatrices(bs_Shader *shader);
bs_Camera *bs_getStdCamera();
void bs_setProjMatrixOrtho(bs_Camera *cam, int left, int right, int bottom, int top);
void bs_setViewMatrixOrtho(bs_Camera *cam);
void bs_createOrthographicProjection(bs_Camera *cam, int left, int right, int bottom, int top);
void bs_setMatrixLookat(bs_Camera *cam, bs_vec3 center, bs_vec3 up);
void bs_setMatrixLook(bs_Camera *cam, bs_vec3 dir, bs_vec3 up);
void bs_setPerspectiveProjection(bs_Camera *cam, float fovy, float aspect, float nearZ, float farZ);

/* --- CONSTANTS --- */
// OpenGL Filtering settings
#define BS_NEAREST 0x2600
#define BS_NEAREST_MIPMAP_LINEAR 0x2702
#define BS_NEAREST_MIPMAP_NEAREST 0x2700

#define BS_LINEAR 0x2601
#define BS_LINEAR_MIPMAP_LINEAR 0x2703
#define BS_LINEAR_MIPMAP_NEAREST 0x2701

#define BS_MAX_TEXTURES 16

// BATCH TYPES
#define BS_STATIC_BATCH 0
#define BS_DYNAMIC_BATCH 1

// BATCH ATTRIBUTE TYPES
#define BS_POSITION_TEXTURE 0
#define BS_POSITION_COLOR 1
#define BS_POSITION_TEXTURE_NORMAL 2
#define BS_POSITION_COLOR_NORMAL 3

// RENDER MODES
#define BS_POINTS 0x0000
#define BS_LINES 0x0001
#define BS_LINE_LOOP 0x0002
#define BS_LINE_STRIP 0x0003
#define BS_TRIANGLES 0x0004
#define BS_TRIANGLE_STRIP 0x0005
#define BS_TRIANGLE_FAN 0x0006
#define BS_LINES_ADJACENCY 0x000A
#define BS_LINE_STRIP_ADJACENCY 0x000B
#define BS_TRIANGLES_ADJACENCY 0x000C
#define BS_TRIANGLE_STRIP_ADJACENCY 0x000D

// RENDER MODE SIZES
#define BS_VERTEX 1
#define BS_LINE 3
#define BS_TRIANGLE 3
#define BS_QUAD 6

//TODO: CAPS
// #define bs_KEY_UNKNOWN   -1
#define bs_KEY_SPACE   32
#define bs_KEY_APOSTROPHE   39 /* ' */
#define bs_KEY_COMMA   44 /* , */
#define bs_KEY_MINUS   45 /* - */
#define bs_KEY_PERIOD   46 /* . */
#define bs_KEY_SLASH   47 /* / */
#define bs_KEY_0   48
#define bs_KEY_1   49
#define bs_KEY_2   50
#define bs_KEY_3   51
#define bs_KEY_4   52
#define bs_KEY_5   53
#define bs_KEY_6   54
#define bs_KEY_7   55
#define bs_KEY_8   56
#define bs_KEY_9   57
#define bs_KEY_SEMICOLON   59 /* ; */
#define bs_KEY_EQUAL   61 /* = */
#define bs_KEY_A   65
#define bs_KEY_B   66
#define bs_KEY_C   67
#define bs_KEY_D   68
#define bs_KEY_E   69
#define bs_KEY_F   70
#define bs_KEY_G   71
#define bs_KEY_H   72
#define bs_KEY_I   73
#define bs_KEY_J   74
#define bs_KEY_K   75
#define bs_KEY_L   76
#define bs_KEY_M   77
#define bs_KEY_N   78
#define bs_KEY_O   79
#define bs_KEY_P   80
#define bs_KEY_Q   81
#define bs_KEY_R   82
#define bs_KEY_S   83
#define bs_KEY_T   84
#define bs_KEY_U   85
#define bs_KEY_V   86
#define bs_KEY_W   87
#define bs_KEY_X   88
#define bs_KEY_Y   89
#define bs_KEY_Z   90
#define bs_KEY_LEFT_BRACKET   91 /* [ */
#define bs_KEY_BACKSLASH   92 /* \ */
#define bs_KEY_RIGHT_BRACKET   93 /* ] */
#define bs_KEY_GRAVE_ACCENT   96 /* ` */
#define bs_KEY_WORLD_1   161 /* non-US #1 */
#define bs_KEY_WORLD_2   162 /* non-US #2 */
#define bs_KEY_ESCAPE   256
#define bs_KEY_ENTER   257
#define bs_KEY_TAB   258
#define bs_KEY_BACKSPACE   259
#define bs_KEY_INSERT   260
#define bs_KEY_DELETE   261
#define bs_KEY_RIGHT   262
#define bs_KEY_LEFT   263
#define bs_KEY_DOWN   264
#define bs_KEY_UP   265
#define bs_KEY_PAGE_UP   266
#define bs_KEY_PAGE_DOWN   267
#define bs_KEY_HOME   268
#define bs_KEY_END   269
#define bs_KEY_CAPS_LOCK   280
#define bs_KEY_SCROLL_LOCK   281
#define bs_KEY_NUM_LOCK   282
#define bs_KEY_PRINT_SCREEN   283
#define bs_KEY_PAUSE   284
#define bs_KEY_F1   290
#define bs_KEY_F2   291
#define bs_KEY_F3   292
#define bs_KEY_F4   293
#define bs_KEY_F5   294
#define bs_KEY_F6   295
#define bs_KEY_F7   296
#define bs_KEY_F8   297
#define bs_KEY_F9   298
#define bs_KEY_F10   299
#define bs_KEY_F11   300
#define bs_KEY_F12   301
#define bs_KEY_F13   302
#define bs_KEY_F14   303
#define bs_KEY_F15   304
#define bs_KEY_F16   305
#define bs_KEY_F17   306
#define bs_KEY_F18   307
#define bs_KEY_F19   308
#define bs_KEY_F20   309
#define bs_KEY_F21   310
#define bs_KEY_F22   311
#define bs_KEY_F23   312
#define bs_KEY_F24   313
#define bs_KEY_F25   314
#define bs_KEY_KP_0   320
#define bs_KEY_KP_1   321
#define bs_KEY_KP_2   322
#define bs_KEY_KP_3   323
#define bs_KEY_KP_4   324
#define bs_KEY_KP_5   325
#define bs_KEY_KP_6   326
#define bs_KEY_KP_7   327
#define bs_KEY_KP_8   328
#define bs_KEY_KP_9   329
#define bs_KEY_KP_DECIMAL   330
#define bs_KEY_KP_DIVIDE   331
#define bs_KEY_KP_MULTIPLY   332
#define bs_KEY_KP_SUBTRACT   333
#define bs_KEY_KP_ADD   334
#define bs_KEY_KP_ENTER   335
#define bs_KEY_KP_EQUAL   336
#define bs_KEY_LEFT_SHIFT   340
#define bs_KEY_LEFT_CONTROL   341
#define bs_KEY_LEFT_ALT   342
#define bs_KEY_LEFT_SUPER   343
#define bs_KEY_RIGHT_SHIFT   344
#define bs_KEY_RIGHT_CONTROL   345
#define bs_KEY_RIGHT_ALT   346
#define bs_KEY_RIGHT_SUPER   347
#define bs_KEY_MENU   348
#define bs_KEY_LAST   bs_KEY_MENU

#endif /* BS_CORE_H */