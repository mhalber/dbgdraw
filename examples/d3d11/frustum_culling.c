// TODO: determine why this demo looks different between OGL and D3D
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
#define MSH_CAMERA_IMPLEMENTATION
#define DBGDRAW_VALIDATION_LAYERS
#define DBGDRAW_USE_DEFAULT_FONT

#include "msh_std.h"
#include "msh_vec_math.h"
#include "msh_camera.h"
#include "stb_truetype.h"
#include "dbgdraw.h"
#include "dbgdraw_d3d11.h"


typedef struct app_state {
  d3d11_ctx_t d3d11;
  dd_ctx_t dd_ctx;
  d3d11_app_input_t* input;
  msh_camera_t* camera;
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

  while ( d3d11_process_events(&state.input) )
  {
    d3d11_viewport(d3d11, 0, 0, d3d11->render_target_width, d3d11->render_target_height);
    d3d11_clear(d3d11, 0.2f, 0.2f, 0.2f, 1.0f);

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
    .win_title = "dbgdraw_d3d11_frustum_culling",
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
    .max_vertices = 1024*200,
    .max_commands = 16,
    .detail_level = 2,
    .enable_frustum_cull = true,
    .enable_depth_test = true 
  };
  error = dd_init( &state->dd_ctx, &desc);
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize dbgdraw library!\n");
    return 1;
  }

  state->camera = calloc( 1, sizeof(msh_camera_t) );
  msh_camera_init( state->camera, &(msh_camera_desc_t)
                  { .eye = msh_vec3( -6.0f, 2.5f, -6.0f ),
                    .center = msh_vec3_zeros(),
                    .up = msh_vec3_posy(),
                    .viewport = msh_vec4( 0.0f, 0.0f, (float)d3d11_desc.win_w, (float)d3d11_desc.win_h),
                    .fovy = (float)msh_rad2deg( 60.0f ),
                    .znear = 0.01f,
                    .zfar = 100.0f,
                    .use_ortho = false } );
  
  return 0;
}

void frame(app_state_t* state) 
{
  assert(state);
  dd_ctx_t* dd_ctx = &state->dd_ctx;
  d3d11_ctx_t* d3d11 = &state->d3d11;
  d3d11_app_input_t* input = state->input;
  msh_camera_t* cam = state->camera;

  int32_t w = d3d11->render_target_width;
  int32_t h = d3d11->render_target_height;
  
  msh_vec2_t scrn_p0 = msh_vec2( input->mouse.prev_x, input->mouse.prev_y );
  msh_vec2_t scrn_p1 = msh_vec2( input->mouse.cur_x, input->mouse.cur_y );
  if (input->mouse.buttons[D3D11_APP_MOUSE_BUTTON_LEFT])
  {
    msh_camera_rotate( cam, scrn_p0, scrn_p1 );
    msh_camera_update_view( cam );
  }
  
  if (input->mouse.scroll_y)
  {
    msh_camera_zoom( cam, input->mouse.scroll_y );
    msh_camera_update_view( cam );
    if( cam->use_ortho ) { msh_camera_update_proj( cam ); }
    input->mouse.scroll_y = 0.0;
  }
  
  if (input->mouse.buttons[D3D11_APP_MOUSE_BUTTON_RIGHT])
  {
    msh_camera_pan( cam, scrn_p0, scrn_p1 );
    msh_camera_update_view( cam );
  }
  
  if (w != cam->viewport.z || h != cam->viewport.w)
  {
    cam->viewport.z = (float)w;
    cam->viewport.w = (float)h;
    msh_camera_update_proj( cam );
  }
  
  dd_new_frame_info_t info = 
  { 
    .view_matrix       = cam->view.data,
    .projection_matrix = cam->proj.data,
    .viewport_size     = cam->viewport.data,
    .vertical_fov      = cam->fovy,
    .projection_type   = DBGDRAW_ORTHOGRAPHIC
  };
  dd_new_frame( dd_ctx, &info );
  
  static float angle = 0.0f;
  if( angle > MSH_TWO_PI ) { angle -= (float)MSH_TWO_PI; }
  angle += 0.02f;
  float aspect_ratio = (float)w/h;
  msh_mat4_t proj    = msh_perspective( (float)msh_deg2rad(45.0f), aspect_ratio, 0.25f, 1.75f );
  msh_mat4_t view    = msh_mat4_identity();
  view = msh_post_translate( view, msh_vec3( 1.0, 0.0, 0.0 ) );
  view = msh_post_rotate( view, angle, msh_vec3_posy() );
  
  /* Switch the dbgdraw view/projection matrices to the one of a testing frustum */
  memcpy( dd_ctx->proj.data, proj.data, sizeof(dd_ctx->proj));
  memcpy( dd_ctx->view.data, view.data, sizeof(dd_ctx->view));
  dd_extract_frustum_planes( dd_ctx );
  
  /* Draw spheres */
  float radius = 0.21f;
  dd_mode_t modes[2] = {DBGDRAW_MODE_STROKE, DBGDRAW_MODE_FILL};
  dd_color_t pass_colors[2] = {DBGDRAW_LIGHT_GREEN, DBGDRAW_GREEN };
  dd_color_t cull_colors[2] = {DBGDRAW_LIGHT_RED, DBGDRAW_RED };
  for( int32_t i = 0; i <= 1; ++i )
  {
    dd_begin_cmd( dd_ctx, modes[i] );
    for( float x = -3 ; x <= 3; x += 0.5 )
    {
      for( float y = -3 ; y <= 3; y += 0.5 )
      {
        msh_vec3_t o = msh_vec3( x, 0.0, y );
        
        /* Draw green spheres that passed culling, and report if a sphere was culled */
        dd_ctx->frustum_cull = 1;
        dd_set_color( dd_ctx, pass_colors[i] );
        int32_t status = dd_sphere( dd_ctx, o.data, radius );
        
        /* For culled spheres, disable culling and draw red sphere */
        if( status == DBGDRAW_ERR_CULLED )
        {
          dd_ctx->frustum_cull = 0;
          dd_set_color( dd_ctx, cull_colors[i] );
          dd_sphere( dd_ctx, o.data, radius );
        }
      }
    }
    dd_end_cmd( dd_ctx );
  }
  
  /* Switch matrices back to main camera */
  memcpy( dd_ctx->proj.data, cam->proj.data, sizeof(dd_ctx->proj));
  memcpy( dd_ctx->view.data, cam->view.data, sizeof(dd_ctx->view));
  
  /* Draw the view frustum */
  dd_set_color( dd_ctx, DBGDRAW_WHITE );
  dd_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );
  dd_frustum( dd_ctx, view.data, proj.data );
  dd_end_cmd( dd_ctx );
  
  dd_set_color( dd_ctx, dd_rgbaf( 1.0f, 1.0f, 1.0f, 0.4f) );
  dd_begin_cmd( dd_ctx, DBGDRAW_MODE_FILL );
  dd_frustum( dd_ctx, view.data, proj.data );
  dd_end_cmd( dd_ctx );
  
  dd_render( dd_ctx );
}

void cleanup( app_state_t* state )
{
  assert(state);
  dd_term(&state->dd_ctx);
  d3d11_terminate(&state->d3d11);
  free(state->dd_ctx.render_backend);
  free(state->camera);
}
