// TODO(maciej): Investigate why d3d11 is not rendering the thin lines correctly.

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
#define DBGDRAW_USE_DEFAULT_FONT

#include "msh_std.h"
#include "msh_vec_math.h"
#include "stb_truetype.h"
#include "dbgdraw.h"
#include "dbgdraw_d3d11.h"

typedef struct app_state {
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
  int32_t error = 0;
  app_state_t state = {0};
  error = init(&state);
  if( error ) { goto main_return; }
  d3d11_ctx_t* d3d11 = &state.d3d11;

  while (d3d11_process_events(&state.input))
  {
    d3d11_viewport(d3d11, 0, 0, d3d11->render_target_width, d3d11->render_target_height);
    d3d11_clear(d3d11, 0.9f, 0.9f, 0.95f, 1.0f);

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
  
  d3d11_ctx_desc_t d3d11_desc = 
  {
    .win_title = "dbgdraw_d3d11_lines",
    .win_x = 100,
    .win_y = 100,
    .win_w = 640,
    .win_h = 320,
    .sample_count = 4
  };
 
  error = d3d11_init(&state->d3d11, &d3d11_desc); 
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize d3d11!\n");
    return 1;
  }
  
  dd_render_backend_t* backend = calloc(1, sizeof(dd_render_backend_t));
  backend->d3d11 = &state->d3d11;
  state->dd_ctx.render_backend = backend;
  dd_ctx_desc_t desc =
  { 
    .max_vertices = 1024,
    .max_commands = 16
  };
  error = dd_init(&state->dd_ctx, &desc);
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize dbgdraw library!\n");
    return 1;
  }
  return 0;
}

void frame(app_state_t* state) 
{
  assert(state);
  dd_ctx_t* dd_ctx = &state->dd_ctx;
  d3d11_ctx_t* d3d11 = &state->d3d11;

  int32_t w = d3d11->render_target_width;
  int32_t h = d3d11->render_target_height;
  msh_vec3_t cam_pos = msh_vec3(0, 0, 5);
  msh_mat4_t view = msh_look_at(cam_pos, msh_vec3_zeros(), msh_vec3_posy());
  msh_vec4_t viewport = msh_vec4(0.0f, 0.0f, (float)w, (float)h);
  msh_mat4_t proj = msh_ortho(0.0f, (float)w, 0.0f, (float)h, 0.01f, 10.0f);
  dd_new_frame_info_t info = 
  { 
    .view_matrix       = view.data,
    .projection_matrix = proj.data,
    .viewport_size     = viewport.data,
    .vertical_fov      = (float)h,
    .projection_type   = DBGDRAW_ORTHOGRAPHIC
  };
  dd_new_frame(dd_ctx, &info);
  
  dd_ctx->aa_radius = dd_vec2(2.0f, 2.0f);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_STROKE);
  
  float x = 30.0f;
  float y = (float)h/2;
  float line_width = 0.5f;
  for (int32_t i = 0; i < 20 ; ++i)
  {
    dd_set_primitive_size(dd_ctx, line_width);
    dd_line(dd_ctx, msh_vec3(x - 20.0f, y - 100.0f, 0.0f).data, msh_vec3(x+20.0f, y + 100.0f, 0.0f).data); 
    x += 20.0f;
    line_width += 0.5f;
  }
  
  x += 100.0f;
  float radius_a = 10.0f;
  float radius_b = 100.0f;
  float step = (float)DBGDRAW_TWO_PI / 36.0f;
  dd_set_primitive_size(dd_ctx, 1.0f);
  for (float theta = 0; theta < DBGDRAW_TWO_PI-step; theta += step)
  {
    float s = sinf(theta);
    float t = cosf(theta);
    dd_line(dd_ctx, 
            msh_vec3(x + s * radius_a, y + t * radius_a, 0.0).data,
            msh_vec3(x + s * radius_b, y + t * radius_b, 0.0).data);
  }
  
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
