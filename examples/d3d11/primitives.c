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
#define GLFW_INCLUDE_NONE
#define DBGDRAW_USE_DEFAULT_FONT
#define DBGDRAW_VALIDATION_LAYERS

#include "msh_std.h"
#include "msh_vec_math.h"
#include "msh_camera.h"
#include "stb_truetype.h"

#include "dbgdraw.h"
#include "overlay.h"
#include "dbgdraw_d3d11.h"

typedef struct app_state {
  d3d11_ctx_t d3d11;
  msh_camera_t camera;
  dd_ctx_t primitives;
  dd_ctx_t overlay;
  d3d11_app_input_t* input;
  dd_color_t colors[9];
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
    d3d11_clear(d3d11, 0.2f, 0.2f, 0.2f, 1.0f );

    frame(&state);
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
    .win_title = "dbgdraw_d3d11_primitives",
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
  
  dd_render_backend_t* primitives_backend = calloc(1, sizeof(dd_render_backend_t));
  primitives_backend->d3d11 = &state->d3d11;
  state->primitives.render_backend = primitives_backend;
  dd_render_backend_t* overlay_backend = calloc(1, sizeof(dd_render_backend_t));
  overlay_backend->d3d11 = &state->d3d11;
  state->overlay.render_backend = overlay_backend;

  dd_ctx_desc_t desc_primitives =
  { 
    .max_vertices = 32,
    .max_commands = 16,
    .detail_level = 2,
    .enable_frustum_cull = true,
    .enable_depth_test = true
  };
  error = dd_init(&state->primitives, &desc_primitives);
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize dbgdraw library!\n");
    return 1;
  }

  dd_ctx_desc_t desc_overlay = 
  { 
    .max_vertices = 1024,
    .max_commands = 16,
    .detail_level = 2,
    .enable_frustum_cull = false,
    .enable_depth_test = false,
    .enable_default_font = true
  };
  error = dd_init(&state->overlay, &desc_overlay);
  if (error)
  {
    fprintf( stderr, "[ERROR] Failed to initialize dbgdraw library!\n" );
    return 1;
  }
    
  msh_camera_init(&state->camera, &(msh_camera_desc_t){ 
                      .eye       = msh_vec3(3.0f, 3.5f, 8.0f),
                      .center    = msh_vec3_zeros(),
                      .up        = msh_vec3_posy(),
                      .viewport  = msh_vec4(0, 0, (float)d3d11_desc.win_w, (float)d3d11_desc.win_h),
                      .fovy      = (float)msh_rad2deg(60.0f),
                      .znear     = 0.01f,
                      .zfar      = 100.0f,
                      .use_ortho = false });
  
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

void frame(app_state_t* state) 
{
  assert(state);

  uint64_t dt1, dt2;
  dt1 = msh_time_now();

  dd_ctx_t* primitives = &state->primitives;
  dd_ctx_t* overlay = &state->overlay;
  d3d11_ctx_t* d3d11 = &state->d3d11;
  msh_camera_t* cam = &state->camera;
  d3d11_app_input_t* input = state->input;

  int32_t w = d3d11->render_target_width;
  int32_t h = d3d11->render_target_height;

  msh_vec2_t scrn_p0 = msh_vec2( (float)input->mouse.prev_x, (float)input->mouse.prev_y );
  msh_vec2_t scrn_p1 = msh_vec2( (float)input->mouse.cur_x, (float)input->mouse.cur_y );
  
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
  
  static int32_t show_lines   = 0;
  static int32_t show_solid   = 1;
  static int32_t show_points  = 0;
  static int32_t shading_mode = (int32_t)DBGDRAW_SHADING_NONE;
  static int32_t show_overlay = 1;
  static uint8_t detail_lvl   = 2;
  if( input->keys[D3D11_APP_KEYCODE_1] ) { show_points  = !show_points;  input->keys[D3D11_APP_KEYCODE_1] = 0; }
  if( input->keys[D3D11_APP_KEYCODE_2] ) { show_lines   = !show_lines;   input->keys[D3D11_APP_KEYCODE_2] = 0; }
  if( input->keys[D3D11_APP_KEYCODE_3] ) { show_solid   = !show_solid;   input->keys[D3D11_APP_KEYCODE_3] = 0; }
  if( input->keys[D3D11_APP_KEYCODE_4] ) { shading_mode = !shading_mode; input->keys[D3D11_APP_KEYCODE_4] = 0; }
  if( input->keys[D3D11_APP_KEYCODE_5] ) { show_overlay = !show_overlay; input->keys[D3D11_APP_KEYCODE_5] = 0; }
  
  if( input->keys[D3D11_APP_KEYCODE_EQUAL] ) { detail_lvl++; input->keys[D3D11_APP_KEYCODE_EQUAL] = 0; }
  if( input->keys[D3D11_APP_KEYCODE_MINUS] ) { detail_lvl--; input->keys[D3D11_APP_KEYCODE_MINUS] = 0; }
  
  detail_lvl = msh_clamp( detail_lvl, 0, 4 );
  
  dd_new_frame_info_t info =
  { 
    .view_matrix       = cam->view.data,
    .projection_matrix = cam->proj.data,
    .viewport_size     = cam->viewport.data,
    .vertical_fov      = cam->fovy,
    .projection_type   = DBGDRAW_PERSPECTIVE
  };
  dd_new_frame( primitives, &info );
  
  primitives->enable_depth_test = true;
  primitives->detail_level = detail_lvl;
  
#define N_TIMES 100
  static float times[N_TIMES] = {0};
  static int32_t time_idx         = 0;
  
  msh_vec3_t min_pt       = msh_vec3( -0.5f, -0.5f, -0.5f );
  msh_vec3_t max_pt       = msh_vec3(  0.5f,  0.5f,  0.5f );
  msh_vec3_t p0           = msh_vec3(  0.0f, -0.5f,  0.0f );
  msh_vec3_t p1           = msh_vec3(  0.0f,  0.5f,  0.0f );
  msh_vec3_t cur_loc      = msh_vec3(  0.0f,  0.0f,  0.0f );
  msh_mat4_t proj         = msh_perspective( (float)msh_deg2rad(45.0f), 4.0f / 3.0f, 0.5f, 1.5f );
  msh_mat4_t view         = msh_mat4_identity();
  dt1 = msh_time_now();
  
  dd_color_t* color = NULL;
  dd_set_shading_type( primitives, shading_mode );
  for (int32_t draw_mode = DBGDRAW_MODE_FILL; draw_mode < DBGDRAW_MODE_COUNT; ++draw_mode)
  {
    switch( draw_mode )
    {
      case DBGDRAW_MODE_POINT:
        if( !show_points ) { continue; }
        dd_set_primitive_size( primitives, 5.0f );
        color = state->colors + 6;
        break;
      case DBGDRAW_MODE_STROKE:
        if( !show_lines) { continue; }
        dd_set_primitive_size( primitives, 3.0f );
        color = state->colors + 3;
        break;
      case DBGDRAW_MODE_FILL:
        if( !show_solid ) { continue; }
        color = state->colors;
        break;
    }
    
    dd_begin_cmd( primitives, draw_mode );
    cur_loc = msh_vec3( 3, 0, 0 );
    dd_set_color( primitives, *color );
    msh_vec3_t v0 = msh_vec3_add(cur_loc, min_pt);
    msh_vec3_t v1 = msh_vec3_add(cur_loc, max_pt);
    dd_aabb( primitives, v0.data, v1.data );
    
    cur_loc = msh_vec3( 1, 0, 0 );
    dd_set_color( primitives, *color );
    msh_mat3_t m = msh_mat3_identity();
    m.col[0] = msh_vec3_normalize( msh_vec3( 1.5, 0.0, 0.5 ) );
    m.col[2] = msh_vec3_normalize( msh_vec3_cross( m.col[0], m.col[1] ) );
    m.col[0] = msh_vec3_scalar_mul( m.col[0], 0.25 );
    m.col[1] = msh_vec3_scalar_mul( m.col[1], 0.5 );
    m.col[2] = msh_vec3_scalar_mul( m.col[2], 0.5 );
    dd_obb( primitives, cur_loc.data, m.data );
    
    cur_loc = msh_vec3( -1, 0, 0 );
    view    = msh_look_at( msh_vec3_add( cur_loc, msh_vec3( 0.0f, 0.0f, 1.0f ) ),
                          cur_loc,
                          msh_vec3(  0.0f, 1.0f, 0.0f ) );
    dd_set_color( primitives, *color );
    dd_frustum( primitives, view.data, proj.data );
    
    cur_loc = msh_vec3( -3, 0, 0 );
    dd_set_color( primitives, *color );
    dd_quad( primitives, msh_vec3_add(cur_loc, msh_vec3( -0.5, -0.5, 0.0 ) ).data,
                         msh_vec3_add(cur_loc, msh_vec3(  0.5, -0.5, 0.0 ) ).data,
                         msh_vec3_add(cur_loc, msh_vec3(  0.5,  0.5, 0.0 ) ).data,
                         msh_vec3_add(cur_loc, msh_vec3( -0.5,  0.5, 0.0 ) ).data );
    color++;
    
    cur_loc = msh_vec3( 3, 0, 2 );
    dd_set_color( primitives, *color );
    dd_circle( primitives, cur_loc.data, 0.5f );
    
    cur_loc = msh_vec3( 1, 0, 2 );
    dd_set_color( primitives, *color );
    dd_arc( primitives, cur_loc.data, 0.5f, (float)MSH_TWO_PI*0.8 );
    
    cur_loc = msh_vec3( -1, 0, 2 );
    dd_set_color( primitives, *color );
    dd_sphere( primitives, cur_loc.data, 0.5f );

    cur_loc = msh_vec3( -3, 0, 2 );
    dd_set_color( primitives, *color );
    dd_torus( primitives, cur_loc.data, 0.5f, 0.1f );
    color++;
    
    cur_loc = msh_vec3( 3, 0, -2 );
    dd_set_color( primitives, *color );
    dd_cylinder( primitives, msh_vec3_add(cur_loc, p0).data, msh_vec3_add(cur_loc, p1).data, 0.5f );
    
    cur_loc = msh_vec3( 1, 0, -2 );
    dd_set_color( primitives, *color );
    dd_cone( primitives, msh_vec3_add(cur_loc, p0).data, msh_vec3_add(cur_loc, p1).data, 0.5f );
    
    cur_loc = msh_vec3( -1, 0, -2 );
    dd_set_color( primitives, *color );
    dd_conical_frustum( primitives, msh_vec3_add(cur_loc, p0).data, msh_vec3_add(cur_loc, p1).data, 0.5f, 0.25f );
    
    cur_loc = msh_vec3( -3, 0, -2 );
    dd_set_color( primitives, *color );
    dd_arrow( primitives, msh_vec3_add(cur_loc, p0).data, msh_vec3_add(cur_loc, p1).data, 0.3f, 0.45f, 0.25f );
    
    dd_end_cmd( primitives );
  }
  
  dd_render( primitives );
  
  if( show_overlay )
  {
    msh_vec3_t cam_pos = msh_vec3( 0, 0, 5 );
    proj = msh_ortho( 0.0f, (float)w, 0.0f, (float)h, 0.01f, 100.0f );
    view = msh_look_at( cam_pos, msh_vec3_zeros(), msh_vec3_posy() );
    info.view_matrix       = view.data;
    info.projection_matrix = proj.data;
    info.vertical_fov      = (float)h;
    info.projection_type   = DBGDRAW_ORTHOGRAPHIC;
    dd_new_frame( overlay, &info );
    
    int32_t x = w - 260;
    int32_t y = 10;
    draw_frame_timer( overlay, x, y, times, N_TIMES, time_idx );
    
    char legend[256];
    snprintf( legend, 256,
              "1   - Toggle Points\n"
              "2   - Toggle Lines\n"
              "3   - Toggle Flat\n"
              "4   - Toggle Shading\n"
              "5   - Toggle Overlay\n"
              "+/- - Increase/decrease detail" );
    
    x = 10;
    y = h - 10;
    
    draw_legend( overlay, legend, x, y );
    
    dd_render( overlay );
  }

  d3d11_present(d3d11);    
  dt2 = msh_time_now();
  
  times[time_idx] = (float)msh_time_diff_ms( dt2, dt1 );
  time_idx = (time_idx+1) % N_TIMES;
}



void cleanup( app_state_t* state )
{
  assert(state);
  free(state->overlay.render_backend);
  free(state->primitives.render_backend);
  dd_term(&state->overlay);
  dd_term(&state->primitives);
  d3d11_terminate(&state->d3d11);

}
