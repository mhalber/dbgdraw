static const char *PROGRAM_NAME = "debugdraw_bezier";

#define MSH_STD_INCLUDE_LIBC_HEADERS
#define MSH_STD_IMPLEMENTATION
#define MSH_VEC_MATH_IMPLEMENTATION
#define GLFW_INCLUDE_NONE

#include "msh_std.h"
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

static int32_t CMU_FONT,CMU_FONT_SMALL;

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

  state->dd_ctx = calloc( 1, sizeof(dbgdraw_ctx_t) );
  error = dbgdraw_init( state->dd_ctx, 1024 * 50, 2048 );
  error = dbgdraw_init_font_from_file( state->dd_ctx, "examples/fonts/cmunrm.ttf", 32, 512, 512, &CMU_FONT );
  error = dbgdraw_init_font_from_file( state->dd_ctx, "examples/fonts/cmunrm.ttf", 20, 512, 512, &CMU_FONT_SMALL );
  if( error )
  {
    fprintf( stderr, "[ERROR] Failed to initialize dbgdraw library!\n" );
    return 1;
  }
  // Set antialiasing
  state->dd_ctx->aa_radius = dd_vec2( 2.0f, 0.0f );

  return 0;
}

void
draw_line_segments( dbgdraw_ctx_t* dd_ctx, dd_vec2_t* pts, int32_t n_pts, dd_color_t color, float width )
{
  dbgdraw_set_color( dd_ctx, color );
  dbgdraw_set_primitive_size( dd_ctx, width );

  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );
  for( int32_t i = 0; i < n_pts-1; ++i )
  {
    dbgdraw_line( dd_ctx, dd_vec3( pts[i].x, pts[i].y, 0.0 ).data, dd_vec3( pts[i+1].x, pts[i+1].y, 0.0 ).data );
  }
  dbgdraw_end_cmd( dd_ctx );
}

void
draw_points( dbgdraw_ctx_t* dd_ctx, dd_vec2_t* pts, int32_t n_pts, 
             dd_color_t pt_fill_color, dd_color_t pt_stroke_color, float radius )
{
  dd_ctx->detail = 3;
  
  dbgdraw_set_color( dd_ctx, pt_fill_color );
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_FILL );
  for( int32_t i = 0; i < n_pts ; ++i )
  {
    dbgdraw_circle( dd_ctx, dd_vec3( pts[i].x, pts[i].y, 0.0 ).data, radius );
  }
  dbgdraw_end_cmd( dd_ctx );

  dbgdraw_set_color( dd_ctx, pt_stroke_color );
  dbgdraw_set_primitive_size( dd_ctx, 2.0f );
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );
  for( int32_t i = 0; i < n_pts ; ++i )
  {
    dbgdraw_circle( dd_ctx, dd_vec3( pts[i].x, pts[i].y, 0.0 ).data, radius );
  }
  dbgdraw_end_cmd( dd_ctx );
  
  dd_ctx->detail = 2;
}

// Based on  https://blog.demofox.org/2015/07/05/the-de-casteljeau-algorithm-for-evaluating-bezier-curves/
float linear_interpolation( float a, float b, float t )
{
  return (1.0-t) * a + t * b;
}

float quadratic_interpolation( float a, float b, float c, float t )
{
  float v1 = linear_interpolation( a, b, t );
  float v2 = linear_interpolation( b, c, t );
  return linear_interpolation( v1, v2, t );
}

float cubic_interpolation( float a, float b, float c, float d, float t )
{
  float v1 = quadratic_interpolation( a, b, c, t );
  float v2 = quadratic_interpolation( b, c, d, t );
  return linear_interpolation( v1, v2, t );
}

float quartic_interpolation( float a, float b, float c, float d, float e, float t )
{
  float v1 = cubic_interpolation( a, b, c, d, t );
  float v2 = cubic_interpolation( b, c, d, e, t );
  return linear_interpolation( v1, v2, t );
}

dd_vec2_t linear_interpolation_pts( dd_vec2_t pt_a, dd_vec2_t pt_b, float t )
{
  dd_vec2_t output;
  output.x = linear_interpolation( pt_a.x, pt_b.x, t );
  output.y = linear_interpolation( pt_a.y, pt_b.y, t );
  return output;
}

dd_vec2_t quadratic_interpolation_pts( dd_vec2_t pt_a, dd_vec2_t pt_b, dd_vec2_t pt_c, float t )
{
  dd_vec2_t pt_1 = linear_interpolation_pts( pt_a, pt_b, t );
  dd_vec2_t pt_2 = linear_interpolation_pts( pt_b, pt_c, t );
  return linear_interpolation_pts( pt_1, pt_2, t );
}

dd_vec2_t cubic_interpolation_pts( dd_vec2_t pt_a, dd_vec2_t pt_b, dd_vec2_t pt_c, dd_vec2_t pt_d, float t )
{
  dd_vec2_t pt_1 = quadratic_interpolation_pts( pt_a, pt_b, pt_c, t );
  dd_vec2_t pt_2 = quadratic_interpolation_pts( pt_b, pt_c, pt_d, t );
  return linear_interpolation_pts( pt_1, pt_2, t );
}

dd_vec2_t quartic_interpolation_pts( dd_vec2_t pt_a, dd_vec2_t pt_b, dd_vec2_t pt_c, dd_vec2_t pt_d, dd_vec2_t pt_e, float t )
{
  dd_vec2_t pt_1 = cubic_interpolation_pts( pt_a, pt_b, pt_c, pt_d, t );
  dd_vec2_t pt_2 = cubic_interpolation_pts( pt_b, pt_c, pt_d, pt_e, t );
  return linear_interpolation_pts( pt_1, pt_2, t );
}

void
draw_labeled_points( dbgdraw_ctx_t* dd_ctx, dd_vec2_t* pts, char** pts_labels, int32_t n_pts, 
                     dd_color_t pt_fill_color, dd_color_t pt_stroke_color, dd_color_t text_color, float radius )
{
  draw_points(dd_ctx, pts, n_pts, pt_fill_color, pt_stroke_color, radius );
  
  float offset_x = radius;
  float offset_y = -(25.0f + radius);
  dbgdraw_set_color( dd_ctx, text_color );
  dbgdraw_set_font( dd_ctx, CMU_FONT );
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_TEXT );
  char buf[128] = {0};
  for( int32_t i = 0; i < n_pts ; ++i )
  {
    char* underscore = strchr( pts_labels[i], '_' );
    strncpy( buf, pts_labels[i], (underscore - pts_labels[i]) );
    dbgdraw_text( dd_ctx, dd_vec3( pts[i].x + offset_x, pts[i].y + offset_y, 0.0f ).data, buf, NULL );
  }
  dbgdraw_end_cmd( dd_ctx );

  offset_x = 15.0f + radius;
  offset_y = -(30.0f + radius );
  dbgdraw_set_font( dd_ctx, CMU_FONT_SMALL );
  dbgdraw_begin_cmd( dd_ctx, DBGDRAW_MODE_TEXT );
  for( int32_t i = 0; i < n_pts ; ++i )
  {
    char* underscore = strchr( pts_labels[i], '_' );
    dbgdraw_text( dd_ctx, dd_vec3( pts[i].x + offset_x, pts[i].y + offset_y, 0.0f ).data, underscore+1, NULL );
  }
  dbgdraw_end_cmd( dd_ctx );
}

float sigmoid( float x, float a, float b, float k )
{
  return k / (1.0f + exp(a+b*x) ); 
}

float smoothstep(float edge0, float edge1, float x) {
  x = msh_clamp01( (x - edge0) / (edge1 - edge0) );
  return x * x * x * (x * (x * 6 - 15) + 10);
}


void frame(app_state_t* state) 
{
  GLFWwindow* window    = state->window;
  dbgdraw_ctx_t* dd_ctx = state->dd_ctx;
 
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );

  int32_t w, h;
  glfwGetWindowSize( window, &w, &h );

  msh_vec3_t cam_pos = msh_vec3(0, 0, 5);
  msh_mat4_t view = msh_look_at( cam_pos, msh_vec3_zeros(), msh_vec3_posy() );
  msh_vec4_t viewport = msh_vec4( 0, 0, w, h );
  msh_mat4_t proj = msh_ortho( 0, w, 0, h, -100.0, 100.0 );

  // TODO(maciej): Reset transformation at the start of each frame
  dbgdraw_new_frame( state->dd_ctx, view.data, proj.data, viewport.data, cam_pos.data, h, true );

  static float at = 0.0f;
  static float delta_t = 0.01f;
  
#define NPTS 5
  dd_vec2_t control_pts[NPTS] = { dd_vec2( 50.0f, 70.0f ), dd_vec2( 180.0f, 270.0f ),
                                  dd_vec2( 240.0f, 280.0f ), dd_vec2( 420.0f, 40.0f ), dd_vec2( 590.0f, 280.0f ) };
  char* control_pts_labels[NPTS] = { "P_0", "P_1", "P_2", "P_3", "P_4" };


  float t = smoothstep( 0, 1, at );
  dd_vec2_t average_pts_a[NPTS-1];
  for( int32_t i = 0; i < NPTS-1; ++i )
  { 
    average_pts_a[i] = linear_interpolation_pts( control_pts[i], control_pts[i+1], t );
  }

  dd_vec2_t average_pts_b[NPTS-2];
  for( int32_t i = 0; i < NPTS-2; ++i )
  {
    average_pts_b[i] = linear_interpolation_pts( average_pts_a[i], average_pts_a[i+1], t );
  }

  dd_vec2_t average_pts_c[NPTS-3];
  for( int32_t i = 0; i < NPTS-3; ++i )
  {
    average_pts_c[i] = linear_interpolation_pts( average_pts_b[i], average_pts_b[i+1], t );
  }

  dd_vec2_t average_pts_d[NPTS-4];
  for( int32_t i = 0; i < NPTS-4; ++i )
  {
    average_pts_d[i] = linear_interpolation_pts( average_pts_c[i], average_pts_c[i+1], t );
  }

  if( at > 1.0f ) { at = 1.0f; delta_t *= -1; }
  if( at < 0.0f )  { at = 0.0f; delta_t *= -1; }
  at += delta_t;

  const int32_t bezier_pts_cap = 64;
  const int32_t bezier_pts_len = 2 + ((bezier_pts_cap-1) * (t * 1000)) / 1000.0;
  static dd_vec2_t* bezier_pts = NULL;
  if (!bezier_pts)
    bezier_pts = malloc( sizeof(dd_vec2_t)*bezier_pts_cap );

  bezier_pts[0] = control_pts[0];
  for( int32_t i = 1; i < bezier_pts_len; ++i )
  {
    float ct = (i) * ( 1.0 / (bezier_pts_cap-1) );
    bezier_pts[i] = quartic_interpolation_pts( control_pts[0], control_pts[1], control_pts[2], control_pts[3], control_pts[4], ct );
  }
  int idx = 1;
  if( bezier_pts_len > 1 ) idx = bezier_pts_len - 1;
  bezier_pts[ idx ] = quartic_interpolation_pts( control_pts[0], control_pts[1], control_pts[2], control_pts[3], control_pts[4], t );

  float r = 2.0f;
  draw_line_segments( dd_ctx, bezier_pts, bezier_pts_len, DBGDRAW_LIGHT_PURPLE, r * 2.0f );

  r = 1.25;
  draw_line_segments( dd_ctx, control_pts,   NPTS,   DBGDRAW_GRAY,   r*2.5f );
  draw_line_segments( dd_ctx, average_pts_a, NPTS-1, DBGDRAW_GREEN,  r*3.5f );
  draw_line_segments( dd_ctx, average_pts_b, NPTS-2, DBGDRAW_RED,    r*3.5f );
  draw_line_segments( dd_ctx, average_pts_c, NPTS-3, DBGDRAW_BLUE,   r*3.5f );
  draw_line_segments( dd_ctx, average_pts_d, NPTS-4, DBGDRAW_PURPLE, r*3.5f );

  draw_points( dd_ctx, control_pts, NPTS,   DBGDRAW_LIGHT_GRAY,   DBGDRAW_GRAY,     r*3.0f );
  draw_points( dd_ctx, average_pts_a, NPTS-1, DBGDRAW_LIGHT_GREEN,  DBGDRAW_GREEN,  r*3.0f );
  draw_points( dd_ctx, average_pts_b, NPTS-2, DBGDRAW_LIGHT_RED,    DBGDRAW_RED,    r*3.0f );
  draw_points( dd_ctx, average_pts_c, NPTS-3, DBGDRAW_LIGHT_BLUE,   DBGDRAW_BLUE,   r*3.0f );
  draw_points( dd_ctx, average_pts_d, NPTS-4, DBGDRAW_LIGHT_PURPLE, DBGDRAW_PURPLE, r*3.0f );

  draw_labeled_points( dd_ctx, control_pts, control_pts_labels, NPTS,
                       DBGDRAW_LIGHT_GRAY, DBGDRAW_GRAY, DBGDRAW_BLACK, 5.0f );

  dbgdraw_render( dd_ctx );
}


void cleanup( app_state_t* state )
{
  dbgdraw_term( state->dd_ctx );
  glfwTerminate();
  free( state );
}
