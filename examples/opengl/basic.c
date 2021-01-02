#define MSH_VEC_MATH_INCLUDE_LIBC_HEADERS
#define MSH_VEC_MATH_IMPLEMENTATION
#define DBGDRAW_VALIDATION_LAYERS
#define DBGDRAW_USE_DEFAULT_FONT
#define GLFW_INCLUDE_NONE

#include "msh_vec_math.h"
#include "stb_truetype.h"
#include "dbgdraw.h"

#include "GLFW/glfw3.h"
#if defined(DD_USE_OGL_33)
#include "glad33.h"
#include "dbgdraw_opengl33.h"
#define DD_GL_VERSION_MAJOR 3
#define DD_GL_VERSION_MINOR 3
#elif defined(DD_USE_OGL_45)
#include "glad45.h"
#include "dbgdraw_opengl45.h"
#define DD_GL_VERSION_MAJOR 4
#define DD_GL_VERSION_MINOR 5
#else
#error "Unrecognized OpenGL Version! Please define either DD_USE_OGL_33 or DD_USE_OGL45!"
#endif


typedef struct app_state_t {
  GLFWwindow* window;
  dd_ctx_t* dd_ctx;
} app_state_t;

int32_t init( app_state_t* state );
void frame( app_state_t* state );
void cleanup( app_state_t* state );

int32_t
main(void)
{
  int32_t error = 0;
  app_state_t* state = calloc( 1, sizeof(app_state_t) );
  
  error = init( state );
  if( error ) { goto main_return; }
  
  GLFWwindow* window = state->window;
  
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
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, DD_GL_VERSION_MAJOR );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, DD_GL_VERSION_MINOR );
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  state->window = glfwCreateWindow( win_width, win_height, "dbgdraw_ogl_basic", NULL, NULL );
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
  dd_ctx_desc_t desc = 
  { 
    .max_vertices = 32,
    .max_commands = 16,
    .detail_level = 2,
    .enable_frustum_cull = false,
    .enable_default_font = 1
  };
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
  glClearColor( 0.9f, 0.9f, 0.9f, 1.0f );
  
  int32_t w, h;
  glfwGetWindowSize( window, &w, &h );
  
  float fovy = 1.0472f; /* approx. 60 deg in radians */
  
  static float angle = 0.0f;
  if( angle > DBGDRAW_TWO_PI ) { angle -= (float)DBGDRAW_TWO_PI; }
  angle += 0.02f;
  
  msh_vec3_t cam_pos = msh_vec3( 0.8f, 2.6f, 3.0f );
  msh_mat4_t view = msh_look_at( cam_pos, msh_vec3_zeros(), msh_vec3_posy() );
  msh_vec4_t viewport = msh_vec4( 0.0f, 0.0f, (float)w, (float)h );
  msh_mat4_t proj = msh_perspective( fovy, (float)w/h, 0.1f, 100.0f );
  msh_mat4_t model = msh_mat4_identity();
  model = msh_post_rotate( model, angle, msh_vec3_posy() );
  
  dd_new_frame_info_t info = { 
    .view_matrix       = view.data,
    .projection_matrix = proj.data,
    .viewport_size     = viewport.data,
    .vertical_fov      = fovy,
    .projection_type   = DBGDRAW_PERSPECTIVE };
  dd_new_frame( dd_ctx, &info );
  
  msh_vec3_t x0 = msh_vec3_negx(); msh_vec3_t x1 = msh_vec3_posx();
  msh_vec3_t y0 = msh_vec3_negy(); msh_vec3_t y1 = msh_vec3_posy();
  msh_vec3_t z0 = msh_vec3_negz(); msh_vec3_t z1 = msh_vec3_posz();
  
  dd_set_primitive_size( dd_ctx, 1.0f );
  dd_set_transform( dd_ctx, model.data );

  dd_set_primitive_size(dd_ctx, 1.0);
  dd_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );
  dd_set_color( dd_ctx, DBGDRAW_RED );
  dd_line( dd_ctx, x0.data, x1.data );
  dd_set_color( dd_ctx, DBGDRAW_GREEN );
  dd_line( dd_ctx, y0.data, y1.data );
  dd_set_color( dd_ctx, DBGDRAW_BLUE );
  dd_line( dd_ctx, z0.data, z1.data );
  dd_set_color( dd_ctx, DBGDRAW_GRAY );
  dd_aabb( dd_ctx, msh_vec3( -1.1f, -1.1f, -1.1f ).data, msh_vec3( 1.1f, 1.1f, 1.1f ).data );
  dd_end_cmd( dd_ctx );
  


  dd_render( dd_ctx );
}

void cleanup( app_state_t* state )
{
  dd_term( state->dd_ctx );
  glfwTerminate();
  free( state );
}
