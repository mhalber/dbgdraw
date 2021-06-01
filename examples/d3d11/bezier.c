#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include "d3d11_app.h"

#define MSH_STD_INCLUDE_LIBC_HEADERS
#define MSH_STD_IMPLEMENTATION
#define MSH_VEC_MATH_IMPLEMENTATION
#define DBGDRAW_VALIDATION_LAYERS

#include "msh_std.h"
#include "msh_vec_math.h"
#include "stb_truetype.h"
#include "dbgdraw.h"
#include "dbgdraw_d3d11.h"

typedef struct app_state
{
  d3d11_ctx_t d3d11;
  dd_ctx_t dd_ctx;
  d3d11_app_input_t* input;
} app_state_t;

int32_t init(app_state_t* state);
void frame(app_state_t* state);
void cleanup(app_state_t* state);

static int32_t CMU_FONT, CMU_FONT_SMALL;

int
main(void)
{
  int32_t error     = 0;
  app_state_t state = {0};
  error             = init(&state);
  if (error) { goto main_return; }
  d3d11_ctx_t* d3d11 = &state.d3d11;

  while (d3d11_process_events(&state.input))
  {
    d3d11_viewport(d3d11,
                   0,
                   0,
                   d3d11->render_target_width,
                   d3d11->render_target_height);
    d3d11_clear(d3d11, 0.9f, 0.9f, 0.9f, 1.0f);

    frame(&state);

    d3d11_present(d3d11);
  }

main_return:
  cleanup(&state);
  return error;
}

int32_t
init(app_state_t* state)
{
  assert(state);

  int32_t error = 0;

  d3d11_ctx_desc_t d3d11_desc = {.win_title    = "dbgdraw_d3d11_bezier",
                                 .win_x        = 100,
                                 .win_y        = 100,
                                 .win_w        = 640,
                                 .win_h        = 320,
                                 .sample_count = 4};

  error = d3d11_init(&state->d3d11, &d3d11_desc);
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize d3d11!\n");
    return 1;
  }

  dd_render_backend_t* backend = calloc(1, sizeof(dd_render_backend_t));
  backend->d3d11               = &state->d3d11;
  state->dd_ctx.render_backend = backend;

  dd_ctx_desc_t desc = {.max_vertices        = 1024 * 200,
                        .max_commands        = 8,
                        .enable_frustum_cull = false,
                        .enable_depth_test   = false,
                        .antialias_radius    = 2.0f};

  error = dd_init(&state->dd_ctx, &desc);
  error = dd_init_font_from_file(&state->dd_ctx,
                                 "examples/fonts/cmunrm.ttf",
                                 "CMU",
                                 32,
                                 512,
                                 512,
                                 &CMU_FONT);
  error = dd_init_font_from_file(&state->dd_ctx,
                                 "examples/fonts/cmunrm.ttf",
                                 "CMU",
                                 20,
                                 512,
                                 512,
                                 &CMU_FONT_SMALL);
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize dbgdraw library!\n");
    return 1;
  }

  return 0;
}

void
draw_line_segments(dd_ctx_t* dd_ctx,
                   dd_vec2_t* pts,
                   int32_t n_pts,
                   dd_color_t color,
                   float width)
{
  dd_set_color(dd_ctx, color);
  dd_set_primitive_size(dd_ctx, width);

  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_STROKE);
  for (int32_t i = 0; i < n_pts - 1; ++i)
  {
    dd_line(dd_ctx,
            dd_vec3(pts[i].x, pts[i].y, 0.0).data,
            dd_vec3(pts[i + 1].x, pts[i + 1].y, 0.0).data);
  }
  dd_end_cmd(dd_ctx);
}

void
draw_points(dd_ctx_t* dd_ctx,
            dd_vec2_t* pts,
            int32_t n_pts,
            dd_color_t pt_fill_color,
            dd_color_t pt_stroke_color,
            float radius)
{
  dd_ctx->detail_level = 3;

  dd_set_color(dd_ctx, pt_fill_color);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);
  for (int32_t i = 0; i < n_pts; ++i)
  {
    dd_circle(dd_ctx, dd_vec3(pts[i].x, pts[i].y, 0.0).data, radius);
  }
  dd_end_cmd(dd_ctx);

  dd_set_color(dd_ctx, pt_stroke_color);
  dd_set_primitive_size(dd_ctx, 2.0f);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_STROKE);
  for (int32_t i = 0; i < n_pts; ++i)
  {
    dd_circle(dd_ctx, dd_vec3(pts[i].x, pts[i].y, 0.0).data, radius);
  }
  dd_end_cmd(dd_ctx);

  dd_ctx->detail_level = 2;
}

// Based on
// https://blog.demofox.org/2015/07/05/the-de-casteljeau-algorithm-for-evaluating-bezier-curves/
float
linear_interpolation(float a, float b, float t)
{
  return (1.0f - t) * a + t * b;
}

float
quadratic_interpolation(float a, float b, float c, float t)
{
  float v1 = linear_interpolation(a, b, t);
  float v2 = linear_interpolation(b, c, t);
  return linear_interpolation(v1, v2, t);
}

float
cubic_interpolation(float a, float b, float c, float d, float t)
{
  float v1 = quadratic_interpolation(a, b, c, t);
  float v2 = quadratic_interpolation(b, c, d, t);
  return linear_interpolation(v1, v2, t);
}

float
quartic_interpolation(float a, float b, float c, float d, float e, float t)
{
  float v1 = cubic_interpolation(a, b, c, d, t);
  float v2 = cubic_interpolation(b, c, d, e, t);
  return linear_interpolation(v1, v2, t);
}

dd_vec2_t
linear_interpolation_pts(dd_vec2_t pt_a, dd_vec2_t pt_b, float t)
{
  dd_vec2_t output;
  output.x = linear_interpolation(pt_a.x, pt_b.x, t);
  output.y = linear_interpolation(pt_a.y, pt_b.y, t);
  return output;
}

dd_vec2_t
quadratic_interpolation_pts(dd_vec2_t pt_a,
                            dd_vec2_t pt_b,
                            dd_vec2_t pt_c,
                            float t)
{
  dd_vec2_t pt_1 = linear_interpolation_pts(pt_a, pt_b, t);
  dd_vec2_t pt_2 = linear_interpolation_pts(pt_b, pt_c, t);
  return linear_interpolation_pts(pt_1, pt_2, t);
}

dd_vec2_t
cubic_interpolation_pts(dd_vec2_t pt_a,
                        dd_vec2_t pt_b,
                        dd_vec2_t pt_c,
                        dd_vec2_t pt_d,
                        float t)
{
  dd_vec2_t pt_1 = quadratic_interpolation_pts(pt_a, pt_b, pt_c, t);
  dd_vec2_t pt_2 = quadratic_interpolation_pts(pt_b, pt_c, pt_d, t);
  return linear_interpolation_pts(pt_1, pt_2, t);
}

dd_vec2_t
quartic_interpolation_pts(dd_vec2_t pt_a,
                          dd_vec2_t pt_b,
                          dd_vec2_t pt_c,
                          dd_vec2_t pt_d,
                          dd_vec2_t pt_e,
                          float t)
{
  dd_vec2_t pt_1 = cubic_interpolation_pts(pt_a, pt_b, pt_c, pt_d, t);
  dd_vec2_t pt_2 = cubic_interpolation_pts(pt_b, pt_c, pt_d, pt_e, t);
  return linear_interpolation_pts(pt_1, pt_2, t);
}

void
draw_labels(dd_ctx_t* dd_ctx,
            dd_vec2_t* pts,
            char** pts_labels,
            int32_t n_pts,
            dd_color_t text_color,
            float radius)
{
  float offset_x = radius;
  float offset_y = -(25.0f + radius);
  dd_set_color(dd_ctx, text_color);

  dd_set_font(dd_ctx, CMU_FONT);
  dd_set_shading_type(dd_ctx, DBGDRAW_SHADING_TEXT);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);
  char buf[128] = {0};
  for (int32_t i = 0; i < n_pts; ++i)
  {
    char* underscore = strchr(pts_labels[i], '_');
    strncpy(buf, pts_labels[i], (underscore - pts_labels[i]));
    dd_text_line(dd_ctx,
                 dd_vec3(pts[i].x + offset_x, pts[i].y + offset_y, 0.0f).data,
                 buf,
                 NULL);
  }
  dd_end_cmd(dd_ctx);

  offset_x = 15.0f + radius;
  offset_y = -(30.0f + radius);
  dd_set_font(dd_ctx, CMU_FONT_SMALL);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);
  for (int32_t i = 0; i < n_pts; ++i)
  {
    char* underscore = strchr(pts_labels[i], '_');
    dd_text_line(dd_ctx,
                 dd_vec3(pts[i].x + offset_x, pts[i].y + offset_y, 0.0f).data,
                 underscore + 1,
                 NULL);
  }
  dd_end_cmd(dd_ctx);
  dd_set_shading_type(dd_ctx, DBGDRAW_SHADING_NONE);
}

float
sigmoid(float x, float a, float b, float k)
{
  return k / (1.0f + expf(a + b * x));
}

float
smoothstep(float edge0, float edge1, float x)
{
  x = msh_clamp01((x - edge0) / (edge1 - edge0));
  return x * x * x * (x * (x * 6 - 15) + 10);
}

void
frame(app_state_t* state)
{
  assert(state);
  dd_ctx_t* dd_ctx   = &state->dd_ctx;
  d3d11_ctx_t* d3d11 = &state->d3d11;

  int32_t w = d3d11->render_target_width;
  int32_t h = d3d11->render_target_height;

  msh_vec3_t cam_pos  = msh_vec3(0.0f, 0.0f, 5.0f);
  msh_mat4_t view     = msh_look_at(cam_pos, msh_vec3_zeros(), msh_vec3_posy());
  msh_vec4_t viewport = msh_vec4(0.0f, 0.0f, (float)w, (float)h);
  msh_mat4_t proj = msh_ortho(0.0f, (float)w, 0.0f, (float)h, -100.0f, 100.0f);

  dd_new_frame_info_t info = {.view_matrix       = view.data,
                              .projection_matrix = proj.data,
                              .viewport_size     = viewport.data,
                              .vertical_fov      = (float)h,
                              .projection_type   = DBGDRAW_ORTHOGRAPHIC};
  dd_new_frame(dd_ctx, &info);

  static float at      = 0.0f;
  static float delta_t = 0.01f;

#define NPTS 5
  dd_vec2_t control_pts[NPTS] = {dd_vec2(50.0f, 70.0f),
                                 dd_vec2(180.0f, 270.0f),
                                 dd_vec2(240.0f, 280.0f),
                                 dd_vec2(420.0f, 40.0f),
                                 dd_vec2(590.0f, 280.0f)};

  char* control_pts_labels[NPTS] = {"P_0", "P_1", "P_2", "P_3", "P_4"};

  float t = smoothstep(0, 1, at);
  dd_vec2_t average_pts_a[NPTS - 1];
  for (int32_t i = 0; i < NPTS - 1; ++i)
  {
    average_pts_a[i] =
      linear_interpolation_pts(control_pts[i], control_pts[i + 1], t);
  }

  dd_vec2_t average_pts_b[NPTS - 2];
  for (int32_t i = 0; i < NPTS - 2; ++i)
  {
    average_pts_b[i] =
      linear_interpolation_pts(average_pts_a[i], average_pts_a[i + 1], t);
  }

  dd_vec2_t average_pts_c[NPTS - 3];
  for (int32_t i = 0; i < NPTS - 3; ++i)
  {
    average_pts_c[i] =
      linear_interpolation_pts(average_pts_b[i], average_pts_b[i + 1], t);
  }

  dd_vec2_t average_pts_d[NPTS - 4];
  for (int32_t i = 0; i < NPTS - 4; ++i)
  {
    average_pts_d[i] =
      linear_interpolation_pts(average_pts_c[i], average_pts_c[i + 1], t);
  }

  if (at > 1.0f)
  {
    at = 1.0f;
    delta_t *= -1;
  }
  if (at < 0.0f)
  {
    at = 0.0f;
    delta_t *= -1;
  }
  at += delta_t;

  const int32_t bezier_pts_cap = 64;
  const int32_t bezier_pts_len =
    2 + (int32_t)(((bezier_pts_cap - 1) * (t * 1000)) / 1000.0);
  static dd_vec2_t* bezier_pts = NULL;
  if (!bezier_pts) bezier_pts = malloc(sizeof(dd_vec2_t) * bezier_pts_cap);

  bezier_pts[0] = control_pts[0];
  for (int32_t i = 1; i < bezier_pts_len; ++i)
  {
    float ct      = (i) * (1.0f / (bezier_pts_cap - 1));
    bezier_pts[i] = quartic_interpolation_pts(control_pts[0],
                                              control_pts[1],
                                              control_pts[2],
                                              control_pts[3],
                                              control_pts[4],
                                              ct);
  }
  int32_t idx = 1;
  if (bezier_pts_len > 1) idx = bezier_pts_len - 1;
  bezier_pts[idx] = quartic_interpolation_pts(control_pts[0],
                                              control_pts[1],
                                              control_pts[2],
                                              control_pts[3],
                                              control_pts[4],
                                              t);

  float r = 2.0f;
  draw_line_segments(dd_ctx,
                     bezier_pts,
                     bezier_pts_len,
                     DBGDRAW_LIGHT_PURPLE,
                     r * 2.0f);

  r = 1.25;
  draw_line_segments(dd_ctx, control_pts, NPTS, DBGDRAW_GRAY, r * 2.5f);
  draw_line_segments(dd_ctx, average_pts_a, NPTS - 1, DBGDRAW_GREEN, r * 3.5f);
  draw_line_segments(dd_ctx, average_pts_b, NPTS - 2, DBGDRAW_RED, r * 3.5f);
  draw_line_segments(dd_ctx, average_pts_c, NPTS - 3, DBGDRAW_BLUE, r * 3.5f);
  draw_line_segments(dd_ctx, average_pts_d, NPTS - 4, DBGDRAW_PURPLE, r * 3.5f);

  draw_points(dd_ctx,
              control_pts,
              NPTS,
              DBGDRAW_LIGHT_GRAY,
              DBGDRAW_GRAY,
              r * 3.0f);
  draw_points(dd_ctx,
              average_pts_a,
              NPTS - 1,
              DBGDRAW_LIGHT_GREEN,
              DBGDRAW_GREEN,
              r * 3.0f);
  draw_points(dd_ctx,
              average_pts_b,
              NPTS - 2,
              DBGDRAW_LIGHT_RED,
              DBGDRAW_RED,
              r * 3.0f);
  draw_points(dd_ctx,
              average_pts_c,
              NPTS - 3,
              DBGDRAW_LIGHT_BLUE,
              DBGDRAW_BLUE,
              r * 3.0f);
  draw_points(dd_ctx,
              average_pts_d,
              NPTS - 4,
              DBGDRAW_LIGHT_PURPLE,
              DBGDRAW_PURPLE,
              r * 3.0f);

  draw_labels(dd_ctx,
              control_pts,
              control_pts_labels,
              NPTS,
              DBGDRAW_BLACK,
              5.0f);

  dd_render(dd_ctx);
}

void
cleanup(app_state_t* state)
{
  assert(state);
  dd_term(&state->dd_ctx);
  d3d11_terminate(&state->d3d11);
  free(state->dd_ctx.render_backend);
}
