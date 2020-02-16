static const char *PROGRAM_NAME = "dbgdraw_frustum_culling";

#define MSH_STD_INCLUDE_LIBC_HEADERS
#define MSH_STD_IMPLEMENTATION
#define MSH_VEC_MATH_IMPLEMENTATION
#define MSH_CAMERA_IMPLEMENTATION
#define GLFW_INCLUDE_NONE

#include "msh_std.h"
#include "msh_vec_math.h"
#include "msh_camera.h"
#include "stb_truetype.h"
#include "dbgdraw.h"

#include "GLFW/glfw3.h"
#include "glad.h"
#include "gui.h"
#include "dbgdraw_opengl45.h"


typedef struct {
  GLFWwindow* window;
  dd_ctx_t* dd_ctx;
  msh_camera_t* camera;
} app_state_t;

int32_t init( app_state_t* state );
void frame( app_state_t* state );
void cleanup( app_state_t* state );


typedef struct input
{
  uint8_t keyboard[256];
  uint8_t mouse_buttons[32];
  double cur_xpos;
  double cur_ypos;
  double prev_xpos;
  double prev_ypos;
  float scroll_x;
  float scroll_y;
} input_t;

static input_t input;

void
mouse_button_callback( GLFWwindow* window, int32_t button, int32_t action, int32_t mods )
{
  (void) window;
  (void) mods;
  input.mouse_buttons[button] = (action == GLFW_RELEASE ) ? 0 : 1;
}

void
scroll_callback( GLFWwindow* window, double xoffset, double yoffset )
{
  (void) window;
  input.scroll_x = (float)xoffset;
  input.scroll_y = (float)yoffset;
}

void
key_callback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods)
{
  (void) window;
  (void) mods;
  (void) scancode;
  if( key < 256 ) { input.keyboard[key] = (action == GLFW_RELEASE) ? 0 : 1; }
}

int32_t
main( void )
{
  int32_t error    = 0;
  app_state_t* state    = calloc( 1, sizeof(app_state_t) );

  GLFWwindow* window    = NULL;
  dd_ctx_t* dd_ctx = NULL;

  error = init( state );
  if( error ) { goto main_return; }

  window  = state->window;
  dd_ctx  = state->dd_ctx;

  while( !glfwWindowShouldClose( window ) )
  {
    frame( state );

    glfwSwapBuffers( window );
    glfwPollEvents();
  }

main_return:
  dd_term( dd_ctx );
  glfwTerminate();
  free( state );
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
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  state->window = glfwCreateWindow( win_width, win_height, PROGRAM_NAME, NULL, NULL );
  if( !state->window )
  {
    fprintf( stderr, "[ERROR] Failed to create window\n" );
    return 1;
  }
  glfwSetMouseButtonCallback( state->window, mouse_button_callback);
  glfwSetScrollCallback( state->window, scroll_callback );
  glfwSetKeyCallback( state->window, key_callback);
  glfwMakeContextCurrent( state->window );

  if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
  {
    fprintf( stderr, "[ERROR] Failed to initialize OpenGL context!\n" );
    return 1;
  }

  state->dd_ctx = calloc( 1, sizeof(dd_ctx_t) );
  dd_ctx_desc_t desc = { .max_vertices = 1024*200,
                         .max_commands = 16,
                         .detail_level = 2,
                         .enable_frustum_cull = true,
                         .enable_depth_test = true };
  error = dd_init( state->dd_ctx, &desc );

  if( error )
  {
    fprintf( stderr, "[ERROR] Failed to initialize dbgdraw library!\n" );
    return 1;
  }
  
  state->camera = calloc( 1, sizeof(msh_camera_t) );
  msh_camera_init( state->camera, &(msh_camera_desc_t){ .eye = msh_vec3( -6, 2.5, -6 ),
                                                        .center = msh_vec3_zeros(),
                                                        .up = msh_vec3_posy(),
                                                        .viewport = msh_vec4( 0, 0, win_width, win_height),
                                                        .fovy = msh_rad2deg( 60.0f ),
                                                        .znear = 0.01f,
                                                        .zfar = 100.0f,
                                                        .use_ortho = false } );
  return 0;
}

void
frame( app_state_t* state )
{
  GLFWwindow* window    = state->window;
  dd_ctx_t* dd_ctx = state->dd_ctx;
  msh_camera_t* cam     = state->camera;
 
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glClearColor( 0.2f, 0.2f, 0.2f, 1.0f );

  int32_t win_width, win_height;
  glfwGetWindowSize( window, &win_width, &win_height );

  input.prev_xpos = input.cur_xpos;
  input.prev_ypos = input.cur_ypos;
  glfwGetCursorPos( window, &input.cur_xpos, &input.cur_ypos );
  msh_vec2_t scrn_p0 = msh_vec2( input.prev_xpos, input.prev_ypos );
  msh_vec2_t scrn_p1 = msh_vec2( input.cur_xpos, input.cur_ypos );
    
  if( input.mouse_buttons[GLFW_MOUSE_BUTTON_1] )
  {
    msh_camera_rotate( cam, scrn_p0, scrn_p1 );
    msh_camera_update_view( cam );
  }

  if( input.scroll_y )
  {
    msh_camera_zoom( cam, input.scroll_y );
    msh_camera_update_view( cam );
    if( cam->use_ortho ) { msh_camera_update_proj( cam ); }
    input.scroll_y = 0.0;
  }

  if( input.mouse_buttons[GLFW_MOUSE_BUTTON_2] )
  {
    msh_camera_pan( cam, scrn_p0, scrn_p1 );
    msh_camera_update_view( cam );
  }

  if( win_width != cam->viewport.z || win_height != cam->viewport.w )
  {
    cam->viewport.z = win_width;
    cam->viewport.w = win_height;
    msh_camera_update_proj( cam );
    glViewport( cam->viewport.x, cam->viewport.y, cam->viewport.z, cam->viewport.w );
  }

  dd_new_frame_info_t info = { .view_matrix       = cam->view.data,
                               .projection_matrix = cam->proj.data,
                               .viewport_size     = cam->viewport.data,
                               .vertical_fov      = cam->fovy,
                               .projection_type   = DBGDRAW_ORTHOGRAPHIC };
  dd_new_frame( dd_ctx, &info );

  static float angle = 0.0f;
  if( angle > MSH_TWO_PI ) { angle -= MSH_TWO_PI; }
  angle += 0.02f;
  float aspect_ratio = (float)win_width/win_height;
  msh_mat4_t proj    = msh_perspective( msh_deg2rad(45.0f), aspect_ratio, 0.25, 1.75 );
  msh_mat4_t view    = msh_mat4_identity();
  view = msh_post_translate( view, msh_vec3( 1.0, 0.0, 0.0 ) );
  view = msh_post_rotate( view, angle, msh_vec3_posy() );

  /* Switch the dbgdraw view/projection matrices to the one of a testing frustum */
  memcpy( dd_ctx->proj.data, proj.data, sizeof(dd_ctx->proj));
  memcpy( dd_ctx->view.data, view.data, sizeof(dd_ctx->view));
  dd_extract_frustum_planes( dd_ctx );

  /* Draw spheres */
  float radius = 0.21;
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

