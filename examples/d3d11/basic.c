
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
#define DBGDRAW_USE_DEFAULT_FONT

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

int main(void)
{
  int32_t error = 0;
  app_state_t state = {0};
  error = init(&state);
  if( error ) { goto main_return; }
  d3d11_ctx_t* d3d11 = &state.d3d11;

  while ( d3d11_process_events(&state.input) )
  {
    d3d11_viewport(d3d11, 0, 0, d3d11->render_target_width, d3d11->render_target_height);
    d3d11_clear(d3d11, 0.9f, 0.9f, 0.9f, 1.0f);

    frame(&state);

    d3d11_present(d3d11);
  }

  main_return:
  cleanup(&state);
  return error;
}

int32_t init(app_state_t* state)
{
  assert(state);

  int32_t error = 0;
  
  d3d11_ctx_desc_t d3d11_desc = 
  {
    .win_title = "dbgdraw_d3d11_basic",
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

 dd_ctx_desc_t desc = 
  { 
    .max_vertices = 32,
    .max_commands = 16,
    .detail_level = 2,
    .enable_frustum_cull = false,
    .enable_depth_test = true,
    .enable_default_font = false,
    .line_antialias_radius = 2.0f
  };
  
  dd_render_backend_t* backend = calloc(1, sizeof(dd_render_backend_t));
  backend->d3d11 = &state->d3d11;
  state->dd_ctx.render_backend = backend;
  error = dd_init( &state->dd_ctx, &desc);
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
  float fovy = 1.0472f; /* approx. 60 deg in radians */
  
  static float angle = 0.0f;
  if (angle > DBGDRAW_TWO_PI) { angle -= (float)DBGDRAW_TWO_PI; }
  angle += 0.02f;
  
  msh_vec3_t cam_pos = msh_vec3(0.8f, 2.6f, 3.0f);
  msh_mat4_t view = msh_look_at(cam_pos, msh_vec3_zeros(), msh_vec3_posy());
  msh_vec4_t viewport = msh_vec4(0.0f, 0.0f, (float)w, (float)h);
  msh_mat4_t proj = msh_perspective(fovy, (float)w/h, 0.1f, 100.0f);
  msh_mat4_t model = msh_mat4_identity();
  model = msh_post_rotate(model, angle, msh_vec3_posy());
  
  dd_new_frame_info_t info = { 
    .view_matrix       = view.data,
    .projection_matrix = proj.data,
    .viewport_size     = viewport.data,
    .vertical_fov      = fovy,
    .projection_type   = DBGDRAW_PERSPECTIVE };
  dd_new_frame(dd_ctx, &info);
  
  msh_vec3_t x0 = msh_vec3_negx(); msh_vec3_t x1 = msh_vec3_posx();
  msh_vec3_t y0 = msh_vec3_negy(); msh_vec3_t y1 = msh_vec3_posy();
  msh_vec3_t z0 = msh_vec3_negz(); msh_vec3_t z1 = msh_vec3_posz();
  
  dd_set_primitive_size(dd_ctx, 1.0f);
  dd_set_transform(dd_ctx, model.data);

  dd_set_primitive_size(dd_ctx, 2.0f);
  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_STROKE);
  dd_set_color(dd_ctx, DBGDRAW_RED);
  dd_line(dd_ctx, x0.data, x1.data);
  dd_set_color(dd_ctx, DBGDRAW_GREEN);
  dd_line(dd_ctx, y0.data, y1.data);
  dd_set_color(dd_ctx, DBGDRAW_BLUE);
  dd_line(dd_ctx, z0.data, z1.data);
  dd_set_color(dd_ctx, DBGDRAW_GRAY);
  dd_aabb(dd_ctx, msh_vec3(-1.1f, -1.1f, -1.1f).data, msh_vec3(1.1f, 1.1f, 1.1f).data);
  dd_end_cmd(dd_ctx);

  dd_render(dd_ctx);
}

void cleanup( app_state_t* state )
{
  assert(state);
  dd_term(&state->dd_ctx);
  d3d11_terminate(&state->d3d11);
  free(state->dd_ctx.render_backend);
}
