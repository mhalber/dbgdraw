#define MSH_VEC_MATH_INCLUDE_LIBC_HEADERS
#define MSH_VEC_MATH_IMPLEMENTATION

#include "msh_vec_math.h"
#include "stb_truetype.h"
#include "debugdraw.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "glad.h"
#include "debugdraw_opengl45.h"


typedef struct {
  GLFWwindow* window;
  dbgdraw_ctx_t* dd_ctx;
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

  int32_t win_width = 640, win_height = 640;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  state->window = glfwCreateWindow( win_width, win_height, "dd_basic", NULL, NULL );
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

  state->dd_ctx = calloc( 1, sizeof(dbgdraw_ctx_t) );
  error = dbgdraw_init( state->dd_ctx, 1024 * 50, 2048 );
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
  dbgdraw_ctx_t* dd_ctx = state->dd_ctx;
 
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glClearColor( 0.9f, 0.9f, 0.9f, 1.0f );

  int32_t w, h;
  glfwGetWindowSize( window, &w, &h );

  float fovy = 1.0472; /* approx. 60 deg in radians */
  
  static float angle = 0.0f;
  if( angle > DBGDRAW_TWO_PI ) { angle -= DBGDRAW_TWO_PI; }
  angle += 0.02f;
  
  msh_vec3_t cam_pos = msh_vec3( 2.8, 2.6, 3.0 );
  msh_mat4_t view = msh_look_at( cam_pos, msh_vec3_zeros(), msh_vec3_posy() );
  msh_vec4_t viewport = msh_vec4( 0, 0, w, h );
  msh_mat4_t proj = msh_perspective( fovy, (float)w/h, 0.1, 100.0 );
  msh_mat4_t model = msh_mat4_identity();
  model = msh_post_rotate( model, angle, msh_vec3_posy() );

  dbgdraw_new_frame( state->dd_ctx, view.data, proj.data, viewport.data, cam_pos.data, fovy, false );

  msh_vec3_t x0 = msh_vec3_negx(); msh_vec3_t x1 = msh_vec3_posx();
  msh_vec3_t y0 = msh_vec3_negy(); msh_vec3_t y1 = msh_vec3_posy();
  msh_vec3_t z0 = msh_vec3_negz(); msh_vec3_t z1 = msh_vec3_posz();

  dbgdraw_set_primitive_size( dd_ctx, 1.0f );
  dbgdraw_set_transform( dd_ctx, model.data );

  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );
  dbgdraw_set_color( dd_ctx, DBGDRAW_RED );
  dbgdraw_line( dd_ctx, x0.data, x1.data );
  dbgdraw_set_color( dd_ctx, DBGDRAW_GREEN );
  dbgdraw_line( dd_ctx, y0.data, y1.data );
  dbgdraw_set_color( dd_ctx, DBGDRAW_BLUE );
  dbgdraw_line( dd_ctx, z0.data, z1.data );
  dbgdraw_set_color( dd_ctx, DBGDRAW_GRAY );
  dbgdraw_aabb( dd_ctx, msh_vec3( -1.1, -1.1, -1.1 ).data, msh_vec3( 1.1, 1.1, 1.1 ).data );
  dbgdraw_end_cmd( dd_ctx );

  dbgdraw_render( dd_ctx );
}

void cleanup( app_state_t* state )
{
  dbgdraw_term( state->dd_ctx );
  glfwTerminate();
  free( state );
}
