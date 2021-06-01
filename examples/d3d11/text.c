
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

#define MSH_VEC_MATH_INCLUDE_LIBC_HEADERS
#define MSH_VEC_MATH_IMPLEMENTATION
#define DBGDRAW_VALIDATION_LAYERS

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

int32_t
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
    d3d11_clear(d3d11, 0.3f, 0.3f, 0.45f, 1.0f);

    frame(&state);

    d3d11_present(d3d11);
  }

main_return:
  cleanup(&state);
  return error;
}

static int32_t TRUENO_FONT, CMU_FONT, ANAKTORIA_FONT;

int32_t
init(app_state_t* state)
{
  assert(state);

  int32_t error = 0;

  d3d11_ctx_desc_t d3d11_desc = {.win_title    = "dbgdraw_d3d11_text",
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

  dd_ctx_desc_t desc = {.max_vertices = 1024 * 50, .max_commands = 32};

  error = dd_init(&state->dd_ctx, &desc);
  error = dd_init_font_from_file(&state->dd_ctx,
                                 "examples/fonts/TruenoLt.otf",
                                 "Trueno",
                                 26,
                                 512,
                                 512,
                                 &TRUENO_FONT);
  error = dd_init_font_from_file(&state->dd_ctx,
                                 "examples/fonts/cmunrm.ttf",
                                 "CMU",
                                 32,
                                 512,
                                 512,
                                 &CMU_FONT);
  error = dd_init_font_from_file(&state->dd_ctx,
                                 "examples/fonts/Anaktoria.ttf",
                                 "ANAKTORIA",
                                 32,
                                 512,
                                 512,
                                 &ANAKTORIA_FONT);
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize dbgdraw library!\n");
    return 1;
  }

  return 0;
}

void
frame(app_state_t* state)
{
  assert(state);
  dd_ctx_t* dd_ctx   = &state->dd_ctx;
  d3d11_ctx_t* d3d11 = &state->d3d11;

  int32_t w           = d3d11->render_target_width;
  int32_t h           = d3d11->render_target_height;
  msh_vec3_t cam_pos  = msh_vec3(0.0f, 0.0f, 5.0f);
  msh_mat4_t view     = msh_look_at(cam_pos, msh_vec3_zeros(), msh_vec3_posy());
  msh_vec4_t viewport = msh_vec4(0.0f, 0.0f, (float)w, (float)h);
  msh_mat4_t proj =
    msh_ortho(-w / 2.0f, w / 2.0f, -h / 2.0f, h / 2.0f, 0.01f, 10.0f);

  dd_new_frame_info_t frame_info = {.view_matrix       = view.data,
                                    .projection_matrix = proj.data,
                                    .viewport_size     = viewport.data,
                                    .vertical_fov      = (float)h,
                                    .projection_type   = DBGDRAW_ORTHOGRAPHIC};
  dd_new_frame(dd_ctx, &frame_info);

  int32_t base_x = -w / 2;
  int32_t base_y = h / 2;
  dd_set_shading_type(dd_ctx, DBGDRAW_SHADING_TEXT);
  dd_set_font(dd_ctx, CMU_FONT);
  dd_set_color(dd_ctx, DBGDRAW_LIGHT_LIME);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);
  dd_text_line(dd_ctx,
               msh_vec3(base_x + 10.0f, base_y - 42.0f, 0.0f).data,
               "The quick brown fox jumps over the lazy dog",
               NULL);
  dd_end_cmd(dd_ctx);

  dd_set_color(dd_ctx, DBGDRAW_LIGHT_CYAN);
  dd_text_info_t info = {0};
  dd_set_font(dd_ctx, ANAKTORIA_FONT);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);
  dd_text_line(dd_ctx,
               msh_vec3(base_x + 10.0f, base_y - 82.0f, 0.0f).data,
               "Sphinx of black quartz, judge my vow.",
               &info);
  dd_end_cmd(dd_ctx);

  dd_set_font(dd_ctx, CMU_FONT);
  dd_set_color(dd_ctx, DBGDRAW_LIGHT_BROWN);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);
  dd_text_line(dd_ctx,
               msh_vec3(base_x + 10.0f, base_y - 122.0f, 0.0f).data,
               "Σωκράτης was a famous philosopher",
               NULL);
  dd_end_cmd(dd_ctx);

  float x = 0.0f;
  float y = 0.0f;
  dd_set_transform(dd_ctx, msh_mat4_identity().data);
  dd_set_font(dd_ctx, TRUENO_FONT);
  dd_set_color(dd_ctx, DBGDRAW_WHITE);

  dd_text_info_t info_bottom = {.vert_align = DBGDRAW_TEXT_BOTTOM};
  dd_text_info_t info_middle = {.vert_align = DBGDRAW_TEXT_MIDDLE};
  dd_text_info_t info_top    = {.vert_align = DBGDRAW_TEXT_TOP};

  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);
  dd_text_line(dd_ctx, msh_vec3(x, y, 0).data, "Baseline", NULL);
  dd_text_line(dd_ctx, msh_vec3(x + 100, y, 0).data, "Bottom", &info_bottom);
  dd_text_line(dd_ctx, msh_vec3(x - 100, y, 0).data, "Middle", &info_middle);
  dd_text_line(dd_ctx, msh_vec3(x - 180, y, 0).data, "Top", &info_top);
  dd_end_cmd(dd_ctx);
  dd_set_shading_type(dd_ctx, DBGDRAW_SHADING_NONE);

  dd_set_color(dd_ctx, DBGDRAW_LIGHT_GRAY);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_STROKE);
  dd_line(dd_ctx,
          msh_vec3(x - 250.0f, y, 0.0f).data,
          msh_vec3(x + 250.0f, y, 0.0f).data);
  dd_end_cmd(dd_ctx);

  dd_text_info_t info_left   = {.horz_align = DBGDRAW_TEXT_LEFT};
  dd_text_info_t info_right  = {.horz_align = DBGDRAW_TEXT_RIGHT};
  dd_text_info_t info_center = {.horz_align = DBGDRAW_TEXT_CENTER};

  x = 0.0f;
  y = -60.0f;

  dd_set_shading_type(dd_ctx, DBGDRAW_SHADING_TEXT);
  dd_set_color(dd_ctx, DBGDRAW_WHITE);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);
  dd_text_line(dd_ctx, msh_vec3(x, y, 0).data, "Left", &info_left);
  dd_text_line(dd_ctx, msh_vec3(x, y - 32, 0).data, "Right", &info_right);
  dd_text_line(dd_ctx, msh_vec3(x, y - 64, 0).data, "Center", &info_center);
  dd_end_cmd(dd_ctx);
  dd_set_shading_type(dd_ctx, DBGDRAW_SHADING_NONE);

  dd_set_color(dd_ctx, DBGDRAW_LIGHT_GRAY);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_STROKE);
  dd_line(dd_ctx,
          msh_vec3(x, y + 30.0f, 0.0f).data,
          msh_vec3(x, y - 80.0f, -5.0f).data);
  dd_end_cmd(dd_ctx);

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
