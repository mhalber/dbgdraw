
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
    d3d11_clear(d3d11, 0.3f, 0.3f, 0.45f, 1.0f);

    frame(&state);

    d3d11_present(d3d11);
  }

  main_return:
  cleanup(&state);
  return error;
}

static int32_t TRUENO_FONT;

int32_t init(app_state_t* state)
{
  assert(state);

  int32_t error = 0;
  
  d3d11_ctx_desc_t d3d11_desc = 
  {
    .win_title = "dbgdraw_d3d11_colors",
    .win_x = 100,
    .win_y = 100,
    .win_w = 640,
    .win_h = 320,
    .sample_count = 4
  };
 
  error = d3d11_init( &state->d3d11, &d3d11_desc ); 
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
  error = dd_init( &state->dd_ctx, &desc );
  if (error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize dbgdraw library!\n");
    return 1;
  }
  const char* font_path = "examples/fonts/TruenoLt.otf";
  error = dd_init_font_from_file( &state->dd_ctx, font_path, "TRUENO", 26, 512, 512, &TRUENO_FONT );
  if( error )
  { 
    fprintf(stderr, "[ERROR] Failed to load font from %s\n", font_path );
    exit( EXIT_FAILURE );
  }
  
  return 0;
}

void
draw_color_tile(dd_ctx_t* dd_ctx, float x, float y, float r, 
                dd_color_t main_color, dd_color_t variant_color, const char* name )
{
  float p = 8.0f;
  dd_set_font( dd_ctx, TRUENO_FONT );
  float font_size = (float)dd_ctx->fonts[dd_ctx->active_font_idx].size;
  
  dd_begin_cmd( dd_ctx, DBGDRAW_MODE_FILL );
  
  dd_color_t text_color = main_color;
  text_color.r = msh_max( text_color.r - 127, 0 );
  text_color.g = msh_max( text_color.g - 127, 0 );
  text_color.b = msh_max( text_color.b - 127, 0 );
  
  dd_set_color( dd_ctx, variant_color );
  dd_quad(dd_ctx,
          msh_vec3( x-p, y-(font_size+p), -1 ).data,
          msh_vec3( x+r+p, y-(font_size+p), -1 ).data,
          msh_vec3( x+r+p, y+r+p, -1 ).data,
          msh_vec3( x-p, y+r+p, -1 ).data );
  
  dd_set_color( dd_ctx, main_color );
  dd_quad(dd_ctx, 
          msh_vec3( x, y, 0.0f ).data,
          msh_vec3( x+r, y, 0.0f ).data,
          msh_vec3( x+r, y+r, 0.0f ).data,
          msh_vec3( x, y+r, 0.0f ).data );
  
  dd_end_cmd( dd_ctx );
  dd_begin_cmd( dd_ctx, DBGDRAW_MODE_FILL );
  
  dd_set_color( dd_ctx, text_color );
  dd_text_info_t ti = {.vert_align = DBGDRAW_TEXT_CENTER };
  dd_text_line( dd_ctx, msh_vec3((float)x, y-font_size-p/2.0f, 0.0f).data, name, &ti );
  dd_end_cmd( dd_ctx );
}

void frame(app_state_t* state) 
{
  assert(state);
  dd_ctx_t* dd_ctx = &state->dd_ctx;
  d3d11_ctx_t* d3d11 = &state->d3d11;

  int32_t w = d3d11->render_target_width;
  int32_t h = d3d11->render_target_height;
  
  static float angle = 0.0f;
  if (angle > DBGDRAW_TWO_PI) { angle -= (float)DBGDRAW_TWO_PI; }
  angle += 0.02f;
  
  msh_vec3_t cam_pos = msh_vec3(0.0f, 0.0f, 5.0f);
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
  
  float size = 96.0f;
  float spacing = 26.0f;
  float x = spacing;
  float y = h-(size+spacing);
  draw_color_tile( dd_ctx, x, y, size, DBGDRAW_RED, DBGDRAW_LIGHT_RED, "Red" );             x += size+spacing;
  draw_color_tile( dd_ctx, x, y, size, DBGDRAW_GREEN, DBGDRAW_LIGHT_GREEN, "Green" );       x += size+spacing;
  draw_color_tile( dd_ctx, x, y, size, DBGDRAW_BLUE, DBGDRAW_LIGHT_BLUE, "Blue" );          x += size+spacing;
  draw_color_tile( dd_ctx, x, y, size, DBGDRAW_CYAN, DBGDRAW_LIGHT_CYAN, "Cyan" );          x += size+spacing;
  draw_color_tile( dd_ctx, x, y, size, DBGDRAW_MAGENTA, DBGDRAW_LIGHT_MAGENTA, "Magenta" ); x = spacing;
  y -= (size+2*spacing);
  
  draw_color_tile( dd_ctx, x, y, size, DBGDRAW_YELLOW, DBGDRAW_LIGHT_YELLOW, "Yellow" );    x += size+spacing;
  draw_color_tile( dd_ctx, x, y, size, DBGDRAW_ORANGE, DBGDRAW_LIGHT_ORANGE, "Orange" );    x += size+spacing;
  draw_color_tile( dd_ctx, x, y, size, DBGDRAW_PURPLE, DBGDRAW_LIGHT_PURPLE, "Purple" );    x += size+spacing;
  draw_color_tile( dd_ctx, x, y, size, DBGDRAW_LIME, DBGDRAW_LIGHT_LIME, "Lime" );          x += size+spacing;
  draw_color_tile( dd_ctx, x, y, size, DBGDRAW_BROWN, DBGDRAW_LIGHT_BROWN, "Brown" );       x = spacing;
  y -= (size+spacing);
  
  dd_render( dd_ctx );
}

void cleanup( app_state_t* state )
{
  assert(state);
  dd_term(&state->dd_ctx);
  d3d11_terminate(&state->d3d11);
  free(state->dd_ctx.render_backend);
}
