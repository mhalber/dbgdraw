static const char *PROGRAM_NAME = "dbgdraw_colors";

#define MSH_STD_INCLUDE_LIBC_HEADERS
#define MSH_STD_IMPLEMENTATION
#define MSH_VEC_MATH_IMPLEMENTATION
#define GLFW_INCLUDE_NONE

#include "msh_std.h"
#include "msh_vec_math.h"
#include "stb_truetype.h"
#include "dbgdraw.h"

#include "GLFW/glfw3.h"
#include "glad.h"
#include "dbgdraw_opengl45.h"

typedef struct {
  GLFWwindow* window;
  dd_ctx_t* dd_ctx;
} app_state_t;

int32_t init( app_state_t* state );
void frame( app_state_t* state );
void cleanup( app_state_t* state );

int32_t
main()
{
  int32_t error      = 0;
  app_state_t* state = calloc( 1, sizeof(app_state_t) );

  error = init( state );
  if( error ) { goto main_return; }

  GLFWwindow* window    = state->window;

  while( !glfwWindowShouldClose( window ) )
  {
    frame( state );

    glfwSwapBuffers( window );
    glfwPollEvents();
  }

main_return:
  cleanup( state );
  return error;
}


static int32_t TRUENO_FONT;

int32_t init( app_state_t* state ) {
  int32_t error = 0;

  error = !(glfwInit());
  if( error )
  {
    fprintf( stderr, "[ERROR] Failed to initialize GLFW library!\n" );
    return 1;
  }

  int32_t win_width = 640, win_height = 320;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  state->window = glfwCreateWindow( win_width, win_height, PROGRAM_NAME, NULL, NULL );
  if( !state->window )
  {
    fprintf( stderr, "[ERROR] Failed to create window\n" );
    return 1;
  }

  glfwMakeContextCurrent( state->window );
  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
  {
    fprintf( stderr, "[ERROR] Failed to initialize OpenGL context!\n" );
    return 1;
  }

  state->dd_ctx = calloc( 1, sizeof(dd_ctx_t) );
  dd_ctx_desc_t desc = { .max_vertices = 1024*50,
                         .max_commands = 16 };
  error = dd_init( state->dd_ctx, &desc );
  if( error )
  {
    fprintf( stderr, "[ERROR] Failed to initialize dbgdraw library!\n" );
    return 1;
  }
  error = dd_init_font_from_file( state->dd_ctx, "examples/fonts/TruenoLt.otf", 26, 512, 512, &TRUENO_FONT );
  if( error ) { exit( EXIT_FAILURE ); }

  return 0;
}


void
draw_color_tile( dd_ctx_t* dd_ctx, int32_t x, int32_t y, int32_t r, 
                 dd_color_t main_color, dd_color_t variant_color, const char* name )
{
  int32_t p = 8;
  dd_set_font( dd_ctx, TRUENO_FONT );
  int32_t font_size = dd_ctx->fonts[dd_ctx->active_font_idx].size;

  dd_begin_cmd( dd_ctx, DBGDRAW_MODE_FILL );


  dd_color_t text_color = main_color;
  text_color.r = msh_max( text_color.r - 127, 0 );
  text_color.g = msh_max( text_color.g - 127, 0 );
  text_color.b = msh_max( text_color.b - 127, 0 );
  
  dd_set_color( dd_ctx, variant_color );
  dd_quad( dd_ctx, msh_vec3( x-p, y-(font_size+p), -1 ).data,
                        msh_vec3( x+r+p, y-(font_size+p), -1 ).data,
                        msh_vec3( x+r+p, y+r+p, -1 ).data,
                        msh_vec3( x-p, y+r+p, -1 ).data );

  dd_set_color( dd_ctx, main_color );
  dd_quad( dd_ctx, msh_vec3( x, y, 0 ).data,
                        msh_vec3( x+r, y, 0 ).data,
                        msh_vec3( x+r, y+r, 0 ).data,
                        msh_vec3( x, y+r, 0 ).data );
  
  dd_end_cmd( dd_ctx );
  dd_begin_cmd( dd_ctx, DBGDRAW_MODE_TEXT );

  dd_set_color( dd_ctx, text_color );
  dd_text_info_t ti = {.vert_align = DBGDRAW_TEXT_CENTER };
  dd_text( dd_ctx, msh_vec3(x, y-font_size-p/2, 0).data, name, &ti );
  dd_end_cmd( dd_ctx );
}

void frame(app_state_t* state) 
{
  GLFWwindow* window    = state->window;
  dd_ctx_t* dd_ctx = state->dd_ctx;
 
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glClearColor( 0.3f, 0.3f, 0.45f, 1.0f );

  int32_t w, h;
  glfwGetWindowSize( window, &w, &h );

  msh_vec3_t cam_pos = msh_vec3(0, 0, 5);
  msh_mat4_t view = msh_look_at( cam_pos, msh_vec3_zeros(), msh_vec3_posy() );
  msh_vec4_t viewport = msh_vec4( 0, 0, w, h );
  msh_mat4_t proj = msh_ortho( 0, w, 0, h, 0.01, 10.0 );
  dd_new_frame_info_t info = { .view_matrix       = view.data,
                               .projection_matrix = proj.data,
                               .viewport_size     = viewport.data,
                               .vertical_fov      = h,
                               .projection_type   = DBGDRAW_ORTHOGRAPHIC };
  dd_new_frame( dd_ctx, &info );

  int32_t size = 96;
  int32_t spacing = 26;
  int32_t x = spacing;
  int32_t y = h-(size+spacing);
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
  dd_term( state->dd_ctx );
  glfwTerminate();
  free( state );
}
