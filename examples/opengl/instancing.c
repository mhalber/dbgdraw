#define MSH_STD_INCLUDE_LIBC_HEADERS
#define MSH_STD_IMPLEMENTATION
#define MSH_VEC_MATH_IMPLEMENTATION
#define GLFW_INCLUDE_NONE

#include "stb_truetype.h"
#include "msh_std.h"
#include "msh_vec_math.h"
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

typedef struct {
    GLFWwindow* window;
    dd_ctx_t* dd_ctx;
} app_state_t;

int32_t init(app_state_t* state);
void frame(app_state_t* state);
void cleanup(app_state_t* state);

#define SIZE_X 31
#define SIZE_Y 15
static dd_instance_data_t instance_data[SIZE_Y][SIZE_X] = {{0}};

int32_t
main( void )
{
  int32_t error = 0;
  app_state_t* state = calloc( 1, sizeof(app_state_t) );
  
  error = init( state );
  if (error) { goto main_return; }
  
  GLFWwindow* window = state->window;
  
  while (!glfwWindowShouldClose(window))
  {
    frame( state );

    glfwSwapBuffers( window );
    glfwPollEvents();
  }
  
  main_return:
  cleanup( state );
    return error;
}

int32_t 
init( app_state_t* state )
{
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
  glfwWindowHint( GLFW_RESIZABLE, GL_FALSE );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
  glfwWindowHint( GLFW_SAMPLES, 4 );
  state->window = glfwCreateWindow( win_width, win_height, "dbgdraw_ogl_instancing", NULL, NULL );
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
  dd_ctx_desc_t desc = 
  { 
    .max_vertices = 1024,
    .max_commands = 16,
    .detail_level = 4
  };
  error = dd_init( state->dd_ctx, &desc );
  if( error )
  {
    fprintf( stderr, "[ERROR] Failed to initialize dbgdraw library!\n" );
    return 1;
  }

  for (int32_t y = 0; y < SIZE_Y; ++y)
  {
    for (int32_t x = 0; x < SIZE_X; ++x)
    {
      instance_data[y][x].position = dd_vec3((x+1) * 20.0f, (y+1) * 20.0f, 0.0);
      instance_data[y][x].color = dd_hsl( (x+0.001f)/SIZE_X * 360.0f, 0.8f, 0.5f);
    }
  }
  
  return 0;
}

void 
frame(app_state_t* state) 
{
  GLFWwindow* window = state->window;
  dd_ctx_t* dd_ctx = state->dd_ctx;
    
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glClearColor( 0.9f, 0.9f, 0.95f, 1.0f );
    
  int32_t w, h;
  glfwGetWindowSize( window, &w, &h );
    
  msh_vec3_t cam_pos = msh_vec3(0.0f, 0.0f, 5.0f);
  msh_mat4_t view = msh_look_at( cam_pos, msh_vec3_zeros(), msh_vec3_posy() );
  msh_vec4_t viewport = msh_vec4( 0.0f, 0.0f, (float)w, (float)h );
  msh_mat4_t proj = msh_ortho( 0.0f, (float)w, 0.0f, (float)h, 0.01f, 10.0f );
  proj = msh_ortho( 0.0f, (float)w, (float)h, 0.0f, 0.01f, 10.0f );

  dd_new_frame_info_t info = 
  { 
    .view_matrix       = view.data,
    .projection_matrix = proj.data,
    .viewport_size     = viewport.data,
    .vertical_fov      = (float)h,
    .projection_type   = DBGDRAW_ORTHOGRAPHIC
  };
  dd_new_frame( dd_ctx, &info );
    
  dd_ctx->aa_radius = dd_vec2( 10.0f, 0.0f );
  dd_set_primitive_size(dd_ctx, 12.0f);

  dd_begin_cmd(dd_ctx, DBGDRAW_MODE_FILL);
  dd_set_instance_data(dd_ctx, SIZE_X*SIZE_Y, &instance_data[0][0]);
  dd_circle2d(dd_ctx, (float[2]){0.0, 0.0}, 7.5 );
  dd_end_cmd(dd_ctx);
    
  dd_render(dd_ctx);
    
}

void 
cleanup(app_state_t* state)
{
  dd_term(state->dd_ctx);
  glfwTerminate();
  free(state);
}
