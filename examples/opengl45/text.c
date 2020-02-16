static const char *PROGRAM_NAME = "debugdraw_text";

#define MSH_VEC_MATH_INCLUDE_LIBC_HEADERS
#define MSH_VEC_MATH_IMPLEMENTATION
#define GLFW_INCLUDE_NONE

#include "msh_vec_math.h"
#include "stb_truetype.h"

#include "debugdraw.h"

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


static int32_t TRUENO_FONT, CMU_FONT, ANAKTORIA_FONT;

int32_t
init( app_state_t* state ) {
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

  state->dd_ctx = calloc( 1, sizeof(dbgdraw_ctx_t) );
  error = dbgdraw_init( state->dd_ctx, 1024 * 50, 2048 );
  error = dbgdraw_init_font_from_file( state->dd_ctx, "examples/fonts/TruenoLt.otf", 26, 512, 512, &TRUENO_FONT );
  error = dbgdraw_init_font_from_file( state->dd_ctx, "examples/fonts/cmunrm.ttf", 32, 512, 512, &CMU_FONT );
  error = dbgdraw_init_font_from_file( state->dd_ctx, "examples/fonts/Anaktoria.ttf", 32, 512, 512, &ANAKTORIA_FONT );
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
  glClearColor( 0.3f, 0.3f, 0.45f, 1.0f );


  int32_t w, h;
  glfwGetWindowSize( window, &w, &h );
  msh_vec3_t cam_pos = msh_vec3(0, 0, 5);
  msh_mat4_t view = msh_look_at( cam_pos, msh_vec3_zeros(), msh_vec3_posy() );
  msh_vec4_t viewport = msh_vec4( 0, 0, w, h );
  msh_mat4_t proj = msh_ortho( -w/2, w/2, -h/2, h/2, 0.01, 10.0 );
  dbgdraw_new_frame( state->dd_ctx, view.data, proj.data, viewport.data, cam_pos.data, h, 1 );

  int base_x = -w/2;
  int base_y = h/2;
  dbgdraw_set_font(dd_ctx, CMU_FONT );
  dbgdraw_set_color( dd_ctx, DBGDRAW_LIGHT_LIME );
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_TEXT );
  dbgdraw_text( dd_ctx, msh_vec3( base_x + 10, base_y - 42, 0).data, "The quick brown fox jumps over the lazy dog", NULL );
  dbgdraw_end_cmd( dd_ctx );

  dbgdraw_set_color( dd_ctx, DBGDRAW_LIGHT_CYAN );
  dbgdraw_text_info_t info = {};
  dbgdraw_set_font(dd_ctx, ANAKTORIA_FONT );
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_TEXT );
  dbgdraw_text( dd_ctx, msh_vec3( base_x + 10, base_y - 82, 0).data, "Sphinx of black quartz, judge my vow.", &info );
  dbgdraw_end_cmd( dd_ctx );

  dbgdraw_set_color( dd_ctx, DBGDRAW_BLUE);
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_FILL );
  float padding = 5.5f;
  msh_vec3_t p = msh_vec3( info.anchor.x - padding, info.anchor.y - padding, info.anchor.z-0.01 );
  msh_vec3_t u = msh_vec3( info.width + 2 * padding, 0.0, 0.0 );
  msh_vec3_t v = msh_vec3( 0.0, info.height + 2 * padding, 0.0 );
  dbgdraw_quad( dd_ctx, p.data,
                        msh_vec3_add( p, u ).data,
                        msh_vec3_add( msh_vec3_add( p, u ), v).data,
                        msh_vec3_add( p, v ).data );
  dbgdraw_end_cmd(dd_ctx);

  dbgdraw_set_font(dd_ctx, CMU_FONT );
  dbgdraw_set_color( dd_ctx, DBGDRAW_LIGHT_BROWN );
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_TEXT );
  dbgdraw_text( dd_ctx, msh_vec3( base_x + 10, base_y - 122, 0).data, "Σωκράτης was a famous philosopher", NULL );
  dbgdraw_end_cmd( dd_ctx );

  float x = 0.0f;
  float y = 0.0f;
  dbgdraw_set_transform( dd_ctx, msh_mat4_identity().data );
  dbgdraw_set_font(dd_ctx, TRUENO_FONT );
  dbgdraw_set_color( dd_ctx, DBGDRAW_WHITE );

  dbgdraw_text_info_t info_bottom = { .vert_align=DBGDRAW_TEXT_BOTTOM };
  dbgdraw_text_info_t info_middle = { .vert_align=DBGDRAW_TEXT_MIDDLE };
  dbgdraw_text_info_t info_top = { .vert_align=DBGDRAW_TEXT_TOP };

  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_TEXT );
  dbgdraw_text( dd_ctx, msh_vec3(x, y, 0).data, "Baseline", NULL);
  dbgdraw_text( dd_ctx, msh_vec3(x+100, y, 0).data, "Bottom", &info_bottom );
  dbgdraw_text( dd_ctx, msh_vec3(x-100, y, 0).data, "Middle", &info_middle );
  dbgdraw_text( dd_ctx, msh_vec3(x-180, y, 0).data, "Top", &info_top );
  dbgdraw_end_cmd( dd_ctx );


  dbgdraw_set_color( dd_ctx, DBGDRAW_LIGHT_GRAY );
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );
  dbgdraw_line( dd_ctx, msh_vec3(x-250, y, 0).data, msh_vec3(x+250, y, 0).data );
  dbgdraw_end_cmd( dd_ctx );

  dbgdraw_text_info_t info_left = { .horz_align=DBGDRAW_TEXT_LEFT };
  dbgdraw_text_info_t info_right = { .horz_align=DBGDRAW_TEXT_RIGHT };
  dbgdraw_text_info_t info_center = { .horz_align=DBGDRAW_TEXT_CENTER };

  x = 0.0f;
  y = -60.0f;

  dbgdraw_set_color( dd_ctx, DBGDRAW_WHITE );
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_TEXT );
  dbgdraw_text( dd_ctx, msh_vec3(x, y, 0).data, "Left", &info_left );
  dbgdraw_text( dd_ctx, msh_vec3(x, y-32, 0).data, "Right", &info_right);
  dbgdraw_text( dd_ctx, msh_vec3(x, y-64, 0).data, "Center", &info_center );
  dbgdraw_end_cmd( dd_ctx );

  dbgdraw_set_color( dd_ctx, DBGDRAW_LIGHT_GRAY );
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );
  dbgdraw_line( dd_ctx, msh_vec3(x, y+30.0, 0).data, msh_vec3(x, y-80.0, -5).data );
  dbgdraw_end_cmd( dd_ctx );

  dbgdraw_sort_commands( dd_ctx );
  dbgdraw_render( dd_ctx );
}

void cleanup( app_state_t* state )
{
  dbgdraw_term( state->dd_ctx );
  glfwTerminate();
  free( state );
}
