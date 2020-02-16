static const char *PROGRAM_NAME = "dbgdraw_lines";

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
main( void )
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

int32_t init( app_state_t* state ) {
  int32_t error = 0;

  error = !(glfwInit());
  if( error )
  {
    fprintf( stderr, "[ERROR] Failed to initialize GLFW library!\n" );
    return 1;
  }

  int32_t win_width = 640, win_height = 320;
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 5 );
  glfwWindowHint( GLFW_RESIZABLE, GL_FALSE );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  glfwWindowHint( GLFW_SAMPLES, 4 );
  state->window = glfwCreateWindow( win_width, win_height, PROGRAM_NAME, NULL, NULL );
  if( !state->window )
  {
    fprintf( stderr, "[ERROR] Failed to create window\n" );
    return 1;
  }

  glfwMakeContextCurrent( state->window );
  if( !gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) )
  {
    fprintf( stderr, "[ERROR] Failed to initialize OpenGL context!\n" );
    return 1;
  }

  state->dd_ctx = calloc( 1, sizeof(dd_ctx_t) );
  dd_ctx_desc_t desc = { .max_vertices = 1024,
                              .max_commands = 16 };
  error = dd_init( state->dd_ctx, &desc );
  if( error )
  {
    fprintf( stderr, "[ERROR] Failed to initialize dbgdraw library!\n" );
    return 1;
  }

  return 0;
}



void frame(app_state_t* state) 
{
  GLFWwindow* window    = state->window;
  dd_ctx_t* dd_ctx = state->dd_ctx;
 
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glClearColor( 0.9f, 0.9f, 0.95f, 1.0f );

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

  dd_ctx->aa_radius = dd_vec2( 2.0f, 2.0f );
  dd_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );

  float x = 30.0f;
  float y = (float)h/2;
  float line_width = 0.5f;
  for( int32_t i = 0; i < 20 ; ++i )
  {
    dd_set_primitive_size( dd_ctx, line_width );
    dd_line( dd_ctx, msh_vec3( x - 20.0f, y - 100.0f, 0.0f ).data, msh_vec3( x+20.0f, y + 100.0f, 0.0f ).data ); 
    x += 20.0f;
    line_width += 0.5f;
  }

  x += 100.0f;
  float radius_a = 10.0f;
  float radius_b = 100.0f;
  float step = DBGDRAW_TWO_PI / 36.0f;
  dd_set_primitive_size( dd_ctx, 1.0f );
  for( float theta = 0; theta < DBGDRAW_TWO_PI-step; theta += step )
  {
    float s = sin( theta );
    float t = cos( theta );
    dd_line( dd_ctx, msh_vec3( x + s * radius_a, y + t * radius_a, 0.0 ).data,
                          msh_vec3( x + s * radius_b, y + t * radius_b, 0.0 ).data );
  }

  dd_end_cmd( dd_ctx );
 
  dd_render( dd_ctx );

}


void cleanup( app_state_t* state )
{
  dd_term( state->dd_ctx );
  glfwTerminate();
  free( state );
}
