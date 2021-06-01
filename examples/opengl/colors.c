#define MSH_STD_INCLUDE_LIBC_HEADERS
#define MSH_STD_IMPLEMENTATION
#define MSH_VEC_MATH_IMPLEMENTATION
#define GLFW_INCLUDE_NONE

#include "msh_std.h"
#include "msh_vec_math.h"
#include "stb_truetype.h"
#include "dbgdraw.h"

#include "GLFW/glfw3.h"
#if defined(DD_USE_OGL_33)
#include "glad33.h"
#include "dbgdraw_opengl33.h"
#define DD_GL_VERSION_MAJOR 3
#define DD_GL_VERSION_MINOR 3
#elif defined(DD_USE_OGL_45)
#include "glad45.h"
#include "dbgdraw_opengl45.h"
#define DD_GL_VERSION_MAJOR 4
#define DD_GL_VERSION_MINOR 5
#else
#error                                                                         \
  "Unrecognized OpenGL Version! Please define either DD_USE_OGL_33 or DD_USE_OGL_45!"
#endif

typedef struct
{
  GLFWwindow* window;
  dd_ctx_t* dd_ctx;
} app_state_t;

int32_t init(app_state_t* state);
void frame(app_state_t* state);
void cleanup(app_state_t* state);

int32_t
main()
{
  int32_t error      = 0;
  app_state_t* state = calloc(1, sizeof(app_state_t));

  error = init(state);
  if (error) { goto main_return; }

  GLFWwindow* window = state->window;

  while (!glfwWindowShouldClose(window))
  {
    frame(state);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

main_return:
  cleanup(state);
  return error;
}

static int32_t TRUENO_FONT;

int32_t
init(app_state_t* state)
{
  int32_t error = 0;

  error = !(glfwInit());
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize GLFW library!\n");
    return 1;
  }

  int32_t win_width = 640, win_height = 320;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, DD_GL_VERSION_MAJOR);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, DD_GL_VERSION_MINOR);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  state->window =
    glfwCreateWindow(win_width, win_height, "dbgdraw_ogl_colors", NULL, NULL);
  if (!state->window)
  {
    fprintf(stderr, "[ERROR] Failed to create window\n");
    return 1;
  }

  glfwMakeContextCurrent(state->window);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    fprintf(stderr, "[ERROR] Failed to initialize OpenGL context!\n");
    return 1;
  }

  state->dd_ctx      = calloc(1, sizeof(dd_ctx_t));
  dd_ctx_desc_t desc = {.max_vertices = 1024, .max_commands = 16};
  error              = dd_init(state->dd_ctx, &desc);
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize dbgdraw library!\n");
    return 1;
  }
  error = dd_init_font_from_file(state->dd_ctx,
                                 "examples/fonts/TruenoLt.otf",
                                 "TRUENO",
                                 26,
                                 512,
                                 512,
                                 &TRUENO_FONT);
  if (error) { exit(EXIT_FAILURE); }

  return 0;
}

void
draw_color_tile(dd_ctx_t* dd_ctx,
                float x,
                float y,
                float r,
                dd_color_t main_color,
                dd_color_t variant_color,
                const char* name)
{
  float p = 8.0f;
  dd_set_font(dd_ctx, TRUENO_FONT);
  float font_size = (float)dd_ctx->fonts[dd_ctx->active_font_idx].size;

  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);

  dd_color_t text_color = main_color;
  text_color.r          = msh_max(text_color.r - 127, 0);
  text_color.g          = msh_max(text_color.g - 127, 0);
  text_color.b          = msh_max(text_color.b - 127, 0);

  dd_set_color(dd_ctx, variant_color);
  dd_quad(dd_ctx,
          msh_vec3(x - p, y - (font_size + p), -1).data,
          msh_vec3(x + r + p, y - (font_size + p), -1).data,
          msh_vec3(x + r + p, y + r + p, -1).data,
          msh_vec3(x - p, y + r + p, -1).data);

  dd_set_color(dd_ctx, main_color);
  dd_quad(dd_ctx,
          msh_vec3(x, y, 0.0f).data,
          msh_vec3(x + r, y, 0.0f).data,
          msh_vec3(x + r, y + r, 0.0f).data,
          msh_vec3(x, y + r, 0.0f).data);

  dd_end_cmd(dd_ctx);

  dd_set_shading_type(dd_ctx, DBGDRAW_SHADING_TEXT);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);
  dd_set_color(dd_ctx, text_color);
  dd_text_info_t ti = {.vert_align = DBGDRAW_TEXT_CENTER};
  dd_text_line(dd_ctx,
               msh_vec3((float)x, y - font_size - p / 2.0f, 0.0f).data,
               name,
               &ti);
  dd_end_cmd(dd_ctx);
  dd_set_shading_type(dd_ctx, DBGDRAW_SHADING_NONE);
}

void
frame(app_state_t* state)
{
  GLFWwindow* window = state->window;
  dd_ctx_t* dd_ctx   = state->dd_ctx;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.3f, 0.3f, 0.45f, 1.0f);

  int32_t w, h;
  glfwGetWindowSize(window, &w, &h);

  msh_vec3_t cam_pos  = msh_vec3(0, 0, 5);
  msh_mat4_t view     = msh_look_at(cam_pos, msh_vec3_zeros(), msh_vec3_posy());
  msh_vec4_t viewport = msh_vec4(0.0f, 0.0f, (float)w, (float)h);
  msh_mat4_t proj     = msh_ortho(0.0f, (float)w, 0.0f, (float)h, 0.01f, 10.0f);
  dd_new_frame_info_t info = {.view_matrix       = view.data,
                              .projection_matrix = proj.data,
                              .viewport_size     = viewport.data,
                              .vertical_fov      = (float)h,
                              .projection_type   = DBGDRAW_ORTHOGRAPHIC};
  dd_new_frame(dd_ctx, &info);

  float size    = 96.0f;
  float spacing = 26.0f;
  float x       = spacing;
  float y       = h - (size + spacing);
  draw_color_tile(dd_ctx, x, y, size, DBGDRAW_RED, DBGDRAW_LIGHT_RED, "Red");
  x += size + spacing;
  draw_color_tile(dd_ctx,
                  x,
                  y,
                  size,
                  DBGDRAW_GREEN,
                  DBGDRAW_LIGHT_GREEN,
                  "Green");
  x += size + spacing;
  draw_color_tile(dd_ctx, x, y, size, DBGDRAW_BLUE, DBGDRAW_LIGHT_BLUE, "Blue");
  x += size + spacing;
  draw_color_tile(dd_ctx, x, y, size, DBGDRAW_CYAN, DBGDRAW_LIGHT_CYAN, "Cyan");
  x += size + spacing;
  draw_color_tile(dd_ctx,
                  x,
                  y,
                  size,
                  DBGDRAW_MAGENTA,
                  DBGDRAW_LIGHT_MAGENTA,
                  "Magenta");
  x = spacing;
  y -= (size + 2 * spacing);

  draw_color_tile(dd_ctx,
                  x,
                  y,
                  size,
                  DBGDRAW_YELLOW,
                  DBGDRAW_LIGHT_YELLOW,
                  "Yellow");
  x += size + spacing;
  draw_color_tile(dd_ctx,
                  x,
                  y,
                  size,
                  DBGDRAW_ORANGE,
                  DBGDRAW_LIGHT_ORANGE,
                  "Orange");
  x += size + spacing;
  draw_color_tile(dd_ctx,
                  x,
                  y,
                  size,
                  DBGDRAW_PURPLE,
                  DBGDRAW_LIGHT_PURPLE,
                  "Purple");
  x += size + spacing;
  draw_color_tile(dd_ctx, x, y, size, DBGDRAW_LIME, DBGDRAW_LIGHT_LIME, "Lime");
  x += size + spacing;
  draw_color_tile(dd_ctx,
                  x,
                  y,
                  size,
                  DBGDRAW_BROWN,
                  DBGDRAW_LIGHT_BROWN,
                  "Brown");
  x = spacing;
  y -= (size + spacing);

  dd_render(dd_ctx);
}

void
cleanup(app_state_t* state)
{
  dd_term(state->dd_ctx);
  glfwTerminate();
  free(state);
}
