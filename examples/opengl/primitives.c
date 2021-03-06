#define MSH_STD_INCLUDE_LIBC_HEADERS
#define MSH_STD_IMPLEMENTATION
#define MSH_VEC_MATH_IMPLEMENTATION
#define MSH_CAMERA_IMPLEMENTATION
#define GLFW_INCLUDE_NONE
#define DBGDRAW_USE_DEFAULT_FONT
#define DBGDRAW_VALIDATION_LAYERS

#include "msh_std.h"
#include "msh_vec_math.h"
#include "msh_camera.h"
#include "stb_truetype.h"

#include "dbgdraw.h"
#include "overlay.h"

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
  "Unrecognized OpenGL Version! Please define either DD_USE_OGL_33 or DD_USE_OGL45!"
#endif

typedef struct
{
  GLFWwindow* window;
  msh_camera_t camera;
  dd_ctx_t* primitives;
  dd_ctx_t* overlay;
  dd_color_t colors[9];
} app_state_t;

int32_t init(app_state_t* state);
void frame(app_state_t* state);
void cleanup(app_state_t* state);

typedef struct input
{
  uint8_t keyboard[256];
  uint8_t mouse_buttons[32];
  double cur_xpos;
  double cur_ypos;
  double prev_xpos;
  double prev_ypos;
  float scroll_x;
  float scroll_y;
} input_t;

static input_t input;

void
mouse_button_callback(GLFWwindow* window,
                      int32_t button,
                      int32_t action,
                      int32_t mods)
{
  (void)window;
  (void)mods;
  input.mouse_buttons[button] = (action == GLFW_RELEASE) ? 0 : 1;
}

void
scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  (void)window;
  input.scroll_x = (float)xoffset;
  input.scroll_y = (float)yoffset;
}

void
key_callback(GLFWwindow* window,
             int32_t key,
             int32_t scancode,
             int32_t action,
             int32_t mods)
{
  (void)window;
  (void)mods;
  (void)scancode;
  if (key < 256) { input.keyboard[key] = (action == GLFW_RELEASE) ? 0 : 1; }
}

int32_t
main(void)
{
  int32_t error      = 0;
  app_state_t* state = calloc(1, sizeof(app_state_t));
  GLFWwindow* window = NULL;

  error = init(state);
  if (error) { goto main_return; }

  window = state->window;

  while (!glfwWindowShouldClose(window)) { frame(state); }

main_return:
  cleanup(state);
  return error;
}

int32_t
init(app_state_t* state)
{
  assert(state);

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
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  state->window = glfwCreateWindow(win_width,
                                   win_height,
                                   "dbgdraw_ogl_primitives",
                                   NULL,
                                   NULL);
  if (!state->window)
  {
    fprintf(stderr, "[ERROR] Failed to create window\n");
    return 1;
  }
  glfwSetMouseButtonCallback(state->window, mouse_button_callback);
  glfwSetScrollCallback(state->window, scroll_callback);
  glfwSetKeyCallback(state->window, key_callback);
  glfwMakeContextCurrent(state->window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    fprintf(stderr, "[ERROR] Failed to initialize OpenGL context!\n");
    return 1;
  }

  state->primitives             = calloc(1, sizeof(dd_ctx_t));
  dd_ctx_desc_t desc_primitives = {.max_vertices        = 32,
                                   .max_commands        = 16,
                                   .detail_level        = 2,
                                   .enable_frustum_cull = true,
                                   .enable_depth_test   = true};
  error                         = dd_init(state->primitives, &desc_primitives);
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize dbgdraw library!\n");
    return 1;
  }

  state->overlay             = calloc(1, sizeof(dd_ctx_t));
  dd_ctx_desc_t desc_overlay = {.max_vertices        = 32,
                                .max_commands        = 16,
                                .detail_level        = 2,
                                .enable_frustum_cull = false,
                                .enable_depth_test   = false,
                                .enable_default_font = true};
  error                      = dd_init(state->overlay, &desc_overlay);
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize dbgdraw library!\n");
    return 1;
  }

  msh_camera_init(
    &state->camera,
    &(msh_camera_desc_t) {.eye    = msh_vec3(3.0f, 3.5f, 8.0f),
                          .center = msh_vec3_zeros(),
                          .up     = msh_vec3_posy(),
                          .viewport =
                            msh_vec4(0, 0, (float)win_width, (float)win_height),
                          .fovy      = (float)msh_rad2deg(60.0f),
                          .znear     = 0.01f,
                          .zfar      = 100.0f,
                          .use_ortho = false});

  state->colors[0] = DBGDRAW_BLUE;
  state->colors[1] = DBGDRAW_RED;
  state->colors[2] = DBGDRAW_GREEN;
  state->colors[3] = DBGDRAW_LIGHT_BLUE;
  state->colors[4] = DBGDRAW_LIGHT_RED;
  state->colors[5] = DBGDRAW_LIGHT_GREEN;
  state->colors[6] = DBGDRAW_WHITE;
  state->colors[7] = DBGDRAW_WHITE;
  state->colors[8] = DBGDRAW_WHITE;

  return 0;
}

void
frame(app_state_t* state)
{
  uint64_t dt1, dt2;
  dt1 = msh_time_now();

  GLFWwindow* window   = state->window;
  dd_ctx_t* primitives = state->primitives;
  dd_ctx_t* overlay    = state->overlay;
  msh_camera_t* cam    = &state->camera;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

  int32_t win_width, win_height;
  glfwGetWindowSize(window, &win_width, &win_height);

  input.prev_xpos = input.cur_xpos;
  input.prev_ypos = input.cur_ypos;
  glfwGetCursorPos(window, &input.cur_xpos, &input.cur_ypos);
  msh_vec2_t scrn_p0 = msh_vec2((float)input.prev_xpos, (float)input.prev_ypos);
  msh_vec2_t scrn_p1 = msh_vec2((float)input.cur_xpos, (float)input.cur_ypos);

  if (input.mouse_buttons[GLFW_MOUSE_BUTTON_1])
  {
    msh_camera_rotate(cam, scrn_p0, scrn_p1);
    msh_camera_update_view(cam);
  }

  if (input.scroll_y)
  {
    msh_camera_zoom(cam, input.scroll_y);
    msh_camera_update_view(cam);
    if (cam->use_ortho) { msh_camera_update_proj(cam); }
    input.scroll_y = 0.0;
  }

  if (input.mouse_buttons[GLFW_MOUSE_BUTTON_2])
  {
    msh_camera_pan(cam, scrn_p0, scrn_p1);
    msh_camera_update_view(cam);
  }

  if (win_width != cam->viewport.z || win_height != cam->viewport.w)
  {
    cam->viewport.z = (float)win_width;
    cam->viewport.w = (float)win_height;
    msh_camera_update_proj(cam);
    glViewport((GLint)cam->viewport.x,
               (GLint)cam->viewport.y,
               (GLint)cam->viewport.z,
               (GLint)cam->viewport.w);
  }

  static int32_t show_lines   = 0;
  static int32_t show_solid   = 1;
  static int32_t show_points  = 0;
  static int32_t shading_mode = (int32_t)DBGDRAW_SHADING_NONE;
  static int32_t show_overlay = 1;
  static uint8_t detail_lvl   = 2;
  if (input.keyboard[GLFW_KEY_1])
  {
    show_points                = !show_points;
    input.keyboard[GLFW_KEY_1] = 0;
  }
  if (input.keyboard[GLFW_KEY_2])
  {
    show_lines                 = !show_lines;
    input.keyboard[GLFW_KEY_2] = 0;
  }
  if (input.keyboard[GLFW_KEY_3])
  {
    show_solid                 = !show_solid;
    input.keyboard[GLFW_KEY_3] = 0;
  }
  if (input.keyboard[GLFW_KEY_4])
  {
    shading_mode               = !shading_mode;
    input.keyboard[GLFW_KEY_4] = 0;
  }
  if (input.keyboard[GLFW_KEY_5])
  {
    show_overlay               = !show_overlay;
    input.keyboard[GLFW_KEY_5] = 0;
  }

  if (input.keyboard[GLFW_KEY_EQUAL])
  {
    detail_lvl++;
    input.keyboard[GLFW_KEY_EQUAL] = 0;
  }
  if (input.keyboard[GLFW_KEY_MINUS])
  {
    detail_lvl--;
    input.keyboard[GLFW_KEY_MINUS] = 0;
  }

  detail_lvl = msh_clamp(detail_lvl, 0, 4);

  dd_new_frame_info_t info = {.view_matrix       = cam->view.data,
                              .projection_matrix = cam->proj.data,
                              .viewport_size     = cam->viewport.data,
                              .vertical_fov      = cam->fovy,
                              .projection_type   = DBGDRAW_PERSPECTIVE};
  dd_new_frame(primitives, &info);

  primitives->enable_depth_test = true;
  primitives->detail_level      = detail_lvl;

#define N_TIMES 100
  static float times[N_TIMES] = {0};
  static int32_t time_idx     = 0;

  msh_vec3_t min_pt  = msh_vec3(-0.5f, -0.5f, -0.5f);
  msh_vec3_t max_pt  = msh_vec3(0.5f, 0.5f, 0.5f);
  msh_vec3_t p0      = msh_vec3(0.0f, -0.5f, 0.0f);
  msh_vec3_t p1      = msh_vec3(0.0f, 0.5f, 0.0f);
  msh_vec3_t cur_loc = msh_vec3(0.0f, 0.0f, 0.0f);
  msh_mat4_t proj =
    msh_perspective((float)msh_deg2rad(45.0f), 4.0f / 3.0f, 0.5f, 1.5f);
  msh_mat4_t view = msh_mat4_identity();
  dt1             = msh_time_now();

  dd_color_t* color = NULL;
  dd_set_shading_type(primitives, shading_mode);
  for (int32_t draw_mode = DBGDRAW_MODE_FILL; draw_mode < DBGDRAW_MODE_COUNT;
       ++draw_mode)
  {
    switch (draw_mode)
    {
      case DBGDRAW_MODE_POINT:
        if (!show_points) { continue; }
        dd_set_primitive_size(primitives, 5.0f);
        color = state->colors + 6;
        break;
      case DBGDRAW_MODE_STROKE:
        if (!show_lines) { continue; }
        dd_set_primitive_size(primitives, 3.0f);
        color = state->colors + 3;
        break;
      case DBGDRAW_MODE_FILL:
        if (!show_solid) { continue; }
        color = state->colors;
        break;
    }

    dd_begin_cmd(primitives, draw_mode);
    cur_loc = msh_vec3(3, 0, 0);
    dd_set_color(primitives, *color);
    msh_vec3_t v0 = msh_vec3_add(cur_loc, min_pt);
    msh_vec3_t v1 = msh_vec3_add(cur_loc, max_pt);
    dd_aabb(primitives, v0.data, v1.data);

    cur_loc = msh_vec3(1, 0, 0);
    dd_set_color(primitives, *color);
    msh_mat3_t m = msh_mat3_identity();
    m.col[0]     = msh_vec3_normalize(msh_vec3(1.5, 0.0, 0.5));
    m.col[2]     = msh_vec3_normalize(msh_vec3_cross(m.col[0], m.col[1]));
    m.col[0]     = msh_vec3_scalar_mul(m.col[0], 0.25);
    m.col[1]     = msh_vec3_scalar_mul(m.col[1], 0.5);
    m.col[2]     = msh_vec3_scalar_mul(m.col[2], 0.5);
    dd_obb(primitives, cur_loc.data, m.data);

    cur_loc = msh_vec3(-1, 0, 0);
    view    = msh_look_at(msh_vec3_add(cur_loc, msh_vec3(0.0f, 0.0f, 1.0f)),
                       cur_loc,
                       msh_vec3(0.0f, 1.0f, 0.0f));
    dd_set_color(primitives, *color);
    dd_frustum(primitives, view.data, proj.data);

    cur_loc = msh_vec3(-3, 0, 0);
    dd_set_color(primitives, *color);
    dd_quad(primitives,
            msh_vec3_add(cur_loc, msh_vec3(-0.5, -0.5, 0.0)).data,
            msh_vec3_add(cur_loc, msh_vec3(0.5, -0.5, 0.0)).data,
            msh_vec3_add(cur_loc, msh_vec3(0.5, 0.5, 0.0)).data,
            msh_vec3_add(cur_loc, msh_vec3(-0.5, 0.5, 0.0)).data);
    color++;

    cur_loc = msh_vec3(3, 0, 2);
    dd_set_color(primitives, *color);
    dd_circle(primitives, cur_loc.data, 0.5f);

    cur_loc = msh_vec3(1, 0, 2);
    dd_set_color(primitives, *color);
    dd_arc(primitives, cur_loc.data, 0.5f, (float)MSH_TWO_PI * 0.8);

    cur_loc = msh_vec3(-1, 0, 2);
    dd_set_color(primitives, *color);
    dd_sphere(primitives, cur_loc.data, 0.5f);

    cur_loc = msh_vec3(-3, 0, 2);
    dd_set_color(primitives, *color);
    dd_torus(primitives, cur_loc.data, 0.5f, 0.1f);
    color++;

    cur_loc = msh_vec3(3, 0, -2);
    dd_set_color(primitives, *color);
    dd_cylinder(primitives,
                msh_vec3_add(cur_loc, p0).data,
                msh_vec3_add(cur_loc, p1).data,
                0.5f);

    cur_loc = msh_vec3(1, 0, -2);
    dd_set_color(primitives, *color);
    dd_cone(primitives,
            msh_vec3_add(cur_loc, p0).data,
            msh_vec3_add(cur_loc, p1).data,
            0.5f);

    cur_loc = msh_vec3(-1, 0, -2);
    dd_set_color(primitives, *color);
    dd_conical_frustum(primitives,
                       msh_vec3_add(cur_loc, p0).data,
                       msh_vec3_add(cur_loc, p1).data,
                       0.5f,
                       0.25f);

    cur_loc = msh_vec3(-3, 0, -2);
    dd_set_color(primitives, *color);
    dd_arrow(primitives,
             msh_vec3_add(cur_loc, p0).data,
             msh_vec3_add(cur_loc, p1).data,
             0.3f,
             0.45f,
             0.25f);

    dd_end_cmd(primitives);
  }

  dd_render(primitives);

  if (show_overlay)
  {
    msh_vec3_t cam_pos = msh_vec3(0, 0, 5);
    proj =
      msh_ortho(0.0f, (float)win_width, 0.0f, (float)win_height, 0.01f, 100.0f);
    view             = msh_look_at(cam_pos, msh_vec3_zeros(), msh_vec3_posy());
    info.view_matrix = view.data;
    info.projection_matrix = proj.data;
    info.vertical_fov      = (float)win_height;
    info.projection_type   = DBGDRAW_ORTHOGRAPHIC;
    dd_new_frame(overlay, &info);

    int32_t x = win_width - 260;
    int32_t y = 10;
    draw_frame_timer(overlay, x, y, times, N_TIMES, time_idx);

    char legend[256];
    snprintf(legend,
             256,
             "1   - Toggle Points\n"
             "2   - Toggle Lines\n"
             "3   - Toggle Flat\n"
             "4   - Toggle Shading\n"
             "5   - Toggle Overlay\n"
             "+/- - Increase/decrease detail");

    x = 10;
    y = win_height - 10;

    draw_legend(overlay, legend, x, y);

    dd_render(overlay);
  }

  glfwSwapBuffers(window);
  glfwPollEvents();

  dt2 = msh_time_now();

  times[time_idx] = (float)msh_time_diff_ms(dt2, dt1);
  time_idx        = (time_idx + 1) % N_TIMES;
}

void
cleanup(app_state_t* state)
{
  dd_term(state->primitives);
  dd_term(state->overlay);
  glfwTerminate();
  free(state);
}