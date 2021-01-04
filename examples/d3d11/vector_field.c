
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

typedef struct app_state {
  d3d11_ctx_t d3d11;
  dd_ctx_t dd_ctx;
  d3d11_app_input_t* input;
  uint64_t start_time;
} app_state_t;

int32_t init(app_state_t* state);
void frame(app_state_t* state);
void cleanup(app_state_t* state);

#define SIZE_X 17
#define SIZE_Y 9
static float field_data[SIZE_Y][SIZE_X] = {{0}};

int32_t
main(void)
{
  int32_t error = 0;
  app_state_t state = {0};
  error = init(&state);
  if( error ) { goto main_return; }
  d3d11_ctx_t* d3d11 = &state.d3d11;

  state.start_time = msh_time_now();
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

int32_t 
init(app_state_t* state)
{
  assert(state);

  int32_t error = 0;
  
  d3d11_ctx_desc_t d3d11_desc = 
  {
    .win_title = "dbgdraw_d3d11_vector_field",
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
    .max_commands = 16,
    .detail_level = 2
  };
  error = dd_init(&state->dd_ctx, &desc);
  if( error )
  {
      fprintf( stderr, "[ERROR] Failed to initialize dbgdraw library!\n" );
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
  msh_mat4_t view = msh_look_at( cam_pos, msh_vec3_zeros(), msh_vec3_posy() );
  msh_vec4_t viewport = msh_vec4( 0.0f, 0.0f, (float)w, (float)h );
  msh_mat4_t proj = msh_ortho( 0.0f, (float)w, 0.0f, (float)h, 0.01f, 10.0f );
  dd_new_frame_info_t info = 
  {
    .view_matrix       = view.data,
    .projection_matrix = proj.data,
    .viewport_size     = viewport.data,
    .vertical_fov      = (float)h,
    .projection_type   = DBGDRAW_ORTHOGRAPHIC
  };
  dd_new_frame( dd_ctx, &info );
  
  dd_ctx->aa_radius = dd_vec2( 2.0f, 2.0f );
  dd_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );
  
  // Fill in the function data
  for (int y = 0; y < SIZE_Y; ++y)
  {
    for (int x = 0; x < SIZE_X; ++x)
    {
      float t = (float)msh_time_diff_sec(msh_time_now(), state->start_time);
      field_data[y][x] = sinf(x/2.0f + t ) + cosf(y/2.0f + t);
    }
  }
  
  float padding = 32.0f;
  float base_x = padding;
  float base_y = padding;
  float delta_x = (w - 2.0f*padding) / (SIZE_X-2.0f);
  float delta_y = (h - 2.0f*padding) / (SIZE_Y-2.0f);
  float pos_a[2] = {0};
  float pos_b[2] = {0};
  float pos_c[2] = {0};
  dd_ctx->aa_radius = dd_vec2(2.0f, 0.0f);
  for (int y = 1; y < SIZE_Y; ++y)
  {
    for (int x = 1; x < SIZE_X; ++x)
    {
      float val_a = field_data[y][x];
      pos_a[0] = base_x + (x-1) * delta_x;
      pos_a[1] = base_y + (y-1) * delta_y;
      
      dd_set_color(dd_ctx, DBGDRAW_BLACK);
      float val_b = field_data[y][x-1];
      float val_c = field_data[y-1][x];
      float dx = val_b - val_a;
      float dy = val_c - val_a;
      pos_b[0] = pos_a[0] + 24.0f*dx;
      pos_b[1] = pos_a[1] + 24.0f*dy;
      pos_c[0] = pos_a[0] + 38.0f*dx;
      pos_c[1] = pos_a[1] + 38.0f*dy;
      
      dd_set_primitive_size(dd_ctx, 4.0f);
      dd_line2d(dd_ctx, pos_a, pos_b);
      
      dd_set_primitive_size(dd_ctx, 8.0f);
      dd_point2d(dd_ctx, pos_b);
      dd_set_primitive_size(dd_ctx, 1.0f);
      dd_point2d(dd_ctx, pos_c);
    }
  }

  dd_end_cmd( dd_ctx );
  
  dd_render( dd_ctx );
}

void cleanup( app_state_t* state )
{
  assert(state);
  dd_term(&state->dd_ctx);
  d3d11_terminate(&state->d3d11);
  free(state->dd_ctx.render_backend);
}
