#ifndef DBGDRAW_H
#define DBGDRAW_H

#ifdef __cplusplus
extern "C" {
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
#define DBGDRAW_HAS_STB_TRUETYPE 1
#else
#define DBGDRAW_HAS_STB_TRUETYPE 0
#endif


#if defined(DBGDRAW_MALLOC) && defined(DBGDRAW_REALLOC) && defined(DBGDRAW_FREE)
// all good
#elif !defined(DBGDRAW_MALLOC) && !defined(DBGDRAW_REALLOC) && !defined(DBGDRAW_FREE)
// all good
#else
#error "If you redefined any of 'malloc', 'realloc', or 'free' functions, please redefine all of them"
#endif

#ifndef DBGDRAW_MALLOC
#include <stdlib.h>
#define DBGDRAW_MALLOC(size)        malloc(size)
#define DBGDRAW_REALLOC(ptr, size)  realloc(ptr, size)
#define DBGDRAW_FREE(ptr)           free(ptr)
#endif

#if defined(DBGDRAW_SIN) && defined(DBGDRAW_COS) && defined(DBGDRAW_FABS) && defined(DBGDRAW_ROUND)
// all good
#elif !defined(DBGDRAW_SIN) && !defined(DBGDRAW_COS) && !defined(DBGDRAW_FABS) && !defined(DBGDRAW_ROUND)
// all good
#else
#error "If you redefined any of 'sin', 'cos', 'fabs' or 'round' functions, please redefine all of them"
#endif

#ifndef DBGDRAW_SINF
#include <math.h>
#define DBGDRAW_SIN(x)   sinf(x)
#define DBGDRAW_COS(x)   cosf(x)
#define DBGDRAW_FABS(x)  fabsf(x)
#define DBGDRAW_ROUND(x) roundf(x)
#endif

#if (defined(DBGDRAW_MEMSET) && defined(DBGDRAW_MEMCPY) && defined(DBGDRAW_STRNCPY) && defined(DBGDRAW_STRNLEN) && defined(STRNCMP))
// all good
#elif !defined(DBGDRAW_MEMSET) && !defined(DBGDRAW_MEMCPY) && !defined(DBGDRAW_STRNCPY) && !defined(DBGDRAW_STRNLEN) && !defined(STRNCMP)
// all good
#else
#error "If you redefined any of 'memset', 'memcpy', 'strncmp', 'strncpy' or 'strnlen' functions, please redefine all of them"
#endif

#ifndef DBGDRAW_MEMSET
#include <string.h>
#define DBGDRAW_MEMSET( dest, ch, len ) memset( dest, ch, len )
#define DBGDRAW_MEMCPY( dest, src, len ) memcpy(dest, src, len )
#define DBGDRAW_STRNCMP( str1, str2, len ) strncmp( str1, str2, len )
#define DBGDRAW_STRNCPY( dest, src, len ) strncpy( dest, src, len )
#define DBGDRAW_STRNLEN( str, len )   strnlen( str, len )
#endif

#ifndef DBGDRAW_ASSERT
#include <assert.h>
#define DBGDRAW_ASSERT(cond) assert(cond)
#endif

#define DBGDRAW_PI          3.14159265358979323846264338327950288419716939937510582097494459231
#define DBGDRAW_TWO_PI      6.28318530717958647692528676655900576839433879875021164194988918462
#define DBGDRAW_PI_OVER_TWO 1.57079632679489661923132169163975144209858469968755291048747229615

#define DD_MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#define DD_MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#define DD_ABS(x)    ( ((x) < 0 ) ? -(x) : (x) )

#ifndef DBGDRAW_NO_STDIO
#include <stdio.h>
#endif

#ifndef DBGDRAW_LOG
#ifdef DBGDRAW_NO_STDIO
#define DBGDRAW_LOG( format, msg, func )
#else
#define DBGDRAW_LOG(format, msg, func) { printf( format, msg, func ); }
#endif
#endif

#include <stdint.h>
#include <stdbool.h>


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declares and required structs / enums
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct dd_color
{
  uint8_t r, g, b, a;
} dd_color_t;

typedef enum dd_mode
{ 
  DBGDRAW_MODE_FILL,
  DBGDRAW_MODE_STROKE,
  DBGDRAW_MODE_POINT,
  
  DBGDRAW_MODE_TEXT,

  DBGDRAW_MODE_COUNT
} dd_mode_t;

typedef enum dd_projection_type
{
  DBGDRAW_PERSPECTIVE,
  DBGDRAW_ORTHOGRAPHIC
} dd_proj_type_t;

typedef struct dd_context_desc dd_ctx_desc_t;
typedef struct dd_new_frame_info dd_new_frame_info_t;
typedef struct dd_context dd_ctx_t;
typedef struct dd_text_info dd_text_info_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Initialize / Terminate - call on init and shut down
int32_t dd_init( dd_ctx_t *ctx, dd_ctx_desc_t* descs );
int32_t dd_term( dd_ctx_t *ctx );

// Start new frame (update all necessary data as listed in info pointer) / Render - call at the start and end of a frame
int32_t dd_new_frame( dd_ctx_t *ctx, dd_new_frame_info_t* info );
int32_t dd_render( dd_ctx_t *ctx );

// Command start and end + modify global state
int32_t dd_begin_cmd( dd_ctx_t *ctx, dd_mode_t draw_mode );
int32_t dd_end_cmd( dd_ctx_t *ctx );

int32_t dd_set_transform( dd_ctx_t *ctx, float* xform );
int32_t dd_set_color( dd_ctx_t *ctx, dd_color_t color );
int32_t dd_set_detail_level( dd_ctx_t *ctx, uint32_t level );
int32_t dd_set_primitive_size( dd_ctx_t *ctx, float primitive_size );

// Primitive draw functions
int32_t dd_point( dd_ctx_t *ctx, float* pt_a );
int32_t dd_line( dd_ctx_t *ctx, float* pt_a, float* pt_b );
int32_t dd_quad( dd_ctx_t *ctx, float* pt_a, float* pt_b, float* pt_c, float* pt_d );
int32_t dd_rect( dd_ctx_t *ctx, float* pt_a, float* pt_b );
int32_t dd_circle( dd_ctx_t *ctx, float* center_pt, float radius );
int32_t dd_arc( dd_ctx_t *ctx, float* center_pt, float radius, float angle );

int32_t dd_aabb( dd_ctx_t *ctx, float* min_pt, float* max_pt );
int32_t dd_obb( dd_ctx_t *ctx, float* center_pt, float* axes_matrix );
int32_t dd_frustum( dd_ctx_t *ctx, float* view_matrix, float* proj_matrix );
int32_t dd_sphere( dd_ctx_t *ctx, float* center_pt, float radius );
int32_t dd_torus( dd_ctx_t *ctx, float* center_pt, float radius_a, float radius_b );
int32_t dd_cone( dd_ctx_t *ctx, float* a, float* b, float radius );
int32_t dd_cylinder( dd_ctx_t *ctx, float* a, float* b, float radius );
int32_t dd_conical_frustum( dd_ctx_t *ctx, float* a, float* b, float radius_a, float radius_b );
int32_t dd_arrow( dd_ctx_t *ctx, float* a, float* b, float radius, float head_radius, float head_length );

int32_t dd_billboard_rect( dd_ctx_t *ctx, float* p, float width, float height );
int32_t dd_billboard_circle( dd_ctx_t *ctx, float* c, float radius );

// Text rendering
int32_t dd_find_font( dd_ctx_t* ctx, char* font_name );
int32_t dd_set_font( dd_ctx_t *ctx, int32_t font_idx );
int32_t dd_text( dd_ctx_t *ctx, float* pos, char* str, dd_text_info_t* info );
float   dd_get_text_width_font_space( dd_ctx_t* ctx, int32_t font_idx, char* str );
int32_t dd_init_font_from_memory( dd_ctx_t* ctx, const void* ttf_buf,
                                       const char* name, int32_t font_size, int32_t width, int32_t height, 
                                       int32_t* font_idx );
#ifndef DBGDRAW_NO_STDIO
int32_t dd_init_font_from_file( dd_ctx_t* ctx, const char* font_path,
                                     int32_t font_size, int32_t width, int32_t height,
                                     int32_t* font_idx );
#endif

// Utilities
void       dd_extract_frustum_planes(dd_ctx_t *ctx);
void       dd_sort_commands( dd_ctx_t* ctx );
char*      dd_error_message( int32_t error_code );
dd_color_t dd_hex2color( uint32_t hex_code );
dd_color_t dd_rgbf( float r, float g, float b );
dd_color_t dd_rgbaf( float r, float g, float b, float a);
dd_color_t dd_rgbu( uint8_t r, uint8_t g, uint8_t b );
dd_color_t dd_rgbau( uint8_t r, uint8_t g, uint8_t b, uint8_t a );
dd_color_t dd_interpolate_color( dd_color_t c0, dd_color_t c1, float t );

// User provides implementation for these - see examples for implementations of these
int32_t dd_backend_init( dd_ctx_t* ctx );
int32_t dd_backend_render( dd_ctx_t *ctx );
int32_t dd_backend_term( dd_ctx_t* ctx );
int32_t dd_backend_init_font_texture( dd_ctx_t* ctx, 
                                           const uint8_t* data, int32_t width, int32_t height, uint32_t* tex_id );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Build in colors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DBGDRAW_RED           (dd_color_t) { 218,  40,  42, 255 }
#define DBGDRAW_GREEN         (dd_color_t) {  38, 172,  38, 255 }
#define DBGDRAW_BLUE          (dd_color_t) {  18,  72, 223, 255 }
#define DBGDRAW_CYAN          (dd_color_t) {  21, 194, 195, 255 }
#define DBGDRAW_MAGENTA       (dd_color_t) { 211,  68, 168, 255 }
#define DBGDRAW_YELLOW        (dd_color_t) { 245, 245,  38, 255 }
#define DBGDRAW_ORANGE        (dd_color_t) { 243, 146,  26, 255 }
#define DBGDRAW_PURPLE        (dd_color_t) { 129,  26, 243, 255 }
#define DBGDRAW_LIME          (dd_color_t) { 142, 243,  26, 255 }
#define DBGDRAW_BROWN         (dd_color_t) { 138,  89,  47, 255 }

#define DBGDRAW_LIGHT_RED     (dd_color_t) { 255, 108, 110, 255 }
#define DBGDRAW_LIGHT_GREEN   (dd_color_t) { 117, 234, 117, 255 }
#define DBGDRAW_LIGHT_BLUE    (dd_color_t) { 104, 140, 243, 255 }
#define DBGDRAW_LIGHT_CYAN    (dd_color_t) { 129, 254, 255, 255 }
#define DBGDRAW_LIGHT_MAGENTA (dd_color_t) { 255, 144, 217, 255 }
#define DBGDRAW_LIGHT_YELLOW  (dd_color_t) { 255, 255, 127, 255 }
#define DBGDRAW_LIGHT_ORANGE  (dd_color_t) { 255, 197, 119, 255 }
#define DBGDRAW_LIGHT_PURPLE  (dd_color_t) { 200, 152, 255, 255 }
#define DBGDRAW_LIGHT_LIME    (dd_color_t) { 205, 255, 148, 255 }
#define DBGDRAW_LIGHT_BROWN   (dd_color_t) { 209, 155, 100, 255 }

#define DBGDRAW_BLACK         (dd_color_t) { 0, 0, 0, 255}
#define DBGDRAW_WHITE         (dd_color_t) { 255, 255, 255, 255}
#define DBGDRAW_GRAY          (dd_color_t) { 162, 162, 162, 255}
#define DBGDRAW_LIGHT_GRAY    (dd_color_t) { 200, 200, 200, 255}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vector Math 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef union dd_vec2
{
  float data[2];
  struct { float x; float y; };
} dd_vec2_t;

typedef union dd_vec3
{
  float data[3];
  struct { float x; float y; float z; };
} dd_vec3_t;

typedef union dd_vec4
{
  float data[4];
  struct { float x; float y; float z; float w; };
} dd_vec4_t;

typedef union dd_mat3
{
  float data[9];
  dd_vec3_t col[3];
} dd_mat3_t;

typedef union dd_mat4
{
  float data[16];
  dd_vec4_t col[4];
} dd_mat4_t;

dd_vec2_t dd_vec2( float x, float y );
dd_vec3_t dd_vec3( float x, float y, float z );
dd_vec4_t dd_vec4( float x, float y, float z, float w );

dd_mat3_t dd_mat3_identity();
dd_mat4_t dd_mat4_identity();

dd_vec3_t dd_vec3_add( dd_vec3_t a, dd_vec3_t b );
dd_vec3_t dd_vec3_sub( dd_vec3_t a, dd_vec3_t b );
dd_vec3_t dd_vec3_scalar_mul( dd_vec3_t a, float s );
dd_vec3_t dd_vec3_scalar_div( dd_vec3_t a, float s );
float     dd_vec3_dot( dd_vec3_t a, dd_vec3_t b );
dd_vec3_t dd_vec3_cross( dd_vec3_t a, dd_vec3_t b );
dd_vec3_t dd_vec3_invert( dd_vec3_t v );
float     dd_vec3_norm( dd_vec3_t v );
dd_vec3_t dd_vec3_normalize( dd_vec3_t v ) ;
dd_vec4_t dd_vec3_to_vec4( dd_vec3_t v );
dd_vec3_t dd_mat3_vec3_mul( dd_mat3_t m, dd_vec3_t v );

dd_vec4_t dd_vec4_add( dd_vec4_t a, dd_vec4_t b );
dd_vec4_t dd_vec4_sub( dd_vec4_t a, dd_vec4_t b );
float     dd_vec4_dot( dd_vec4_t a, dd_vec4_t b );
dd_vec4_t dd_vec4_mul( dd_vec4_t a, dd_vec4_t b );
dd_vec4_t dd_vec4_scalar_mul( dd_vec4_t a, float s );
dd_vec3_t dd_vec4_to_vec3( dd_vec4_t a );

dd_mat4_t dd_mat4_mul( dd_mat4_t a, dd_mat4_t b );
dd_vec3_t dd_mat4_vec3_mul( dd_mat4_t m, dd_vec3_t v, int32_t is_point );
dd_vec4_t dd_mat4_vec4_mul( dd_mat4_t m, dd_vec4_t v );
dd_mat3_t dd_mat4_to_mat3( dd_mat4_t m );
float     dd_mat4_determinant( dd_mat4_t m );
dd_mat4_t dd_mat4_se3_inverse( dd_mat4_t m );
dd_mat4_t dd_mat4_inverse( dd_mat4_t m );
dd_mat4_t dd_mat4_transpose( dd_mat4_t m );

dd_mat4_t dd_pre_scale( dd_mat4_t m, dd_vec3_t s );
dd_mat4_t dd_pre_translate( dd_mat4_t m, dd_vec3_t t );
dd_mat4_t dd_pre_rotate( dd_mat4_t m, float angle, dd_vec3_t v );

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DebugDraw Structs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum dd_text_valign
{
  DBGDRAW_TEXT_BASELINE,
  DBGDRAW_TEXT_BOTTOM,
  DBGDRAW_TEXT_MIDDLE,
  DBGDRAW_TEXT_TOP, 
} dd_text_valign_t;

typedef enum dd_text_halign
{
  DBGDRAW_TEXT_LEFT,
  DBGDRAW_TEXT_CENTER,
  DBGDRAW_TEXT_RIGHT
} dd_text_halign_t;

typedef enum dd_error
{
  DBGDRAW_ERR_OK = 0,
  DBGDRAW_ERR_NO_ACTIVE_CMD,
  DBGDRAW_ERR_PREV_CMD_NOT_ENDED,
  DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER,
  DBGDRAW_ERR_OUT_OF_COMMAND_BUFFER,
  DBGDRAW_ERR_CULLED,
  DBGDRAW_ERR_FONT_FILE_NOT_FOUND,
  DBGDRAW_ERR_FONT_LIMIT_REACHED,
  DBGDRAW_ERR_OUT_OF_BOUNDS_ACCESS,

  DBGDRAW_ERR_COUNT
} dd_err_code_t;


typedef struct dd_render_backend dd_render_backend_t;

typedef struct dd_context_desc
{
  int32_t max_vertices;
  int32_t max_commands;
  int32_t max_fonts;
  int32_t detail_level;
  float   line_antialias_radius;
  uint8_t enable_frustum_cull;
  uint8_t enable_depth_test;
} dd_ctx_desc_t;

typedef struct dd_new_frame_info
{
  float*  view_matrix;
  float*  projection_matrix;
  float*  viewport_size;
  float   vertical_fov;
  uint8_t projection_type;
} dd_new_frame_info_t;

typedef struct dd_text_info
{
  dd_text_halign_t horz_align;
  dd_text_valign_t vert_align;

  dd_vec3_t anchor;
  float width;
  float height;
} dd_text_info_t;

typedef struct dd_vertex
{
  dd_vec4_t pos_size;
  dd_vec2_t uv;
  dd_color_t col;
  float dummy;
} dd_vertex_t;

typedef struct dd_command
{
  int32_t base_index;
  int32_t vertex_count;

  dd_mat4_t xform;
  float min_depth;

  dd_mode_t draw_mode;
#if DBGDRAW_HAS_STB_TRUETYPE
  int32_t font_idx;
#endif
} dd_cmd_t;

#if DBGDRAW_HAS_STB_TRUETYPE
typedef struct dd_font_data
{
  char* name;
  int32_t bitmap_width, bitmap_height;
  float ascent, descent, line_gap;

  stbtt_packedchar char_data[256];
  stbtt_fontinfo info;
  uint32_t size;
  uint32_t tex_id;
} dd_font_data_t;
#endif

typedef struct dd_context
{
  /* User accessible state */
  dd_color_t color;
  dd_mat4_t xform;
  uint8_t detail_level;
  uint8_t frustum_cull;
  float primitive_size;

  /* Command storage */
  dd_cmd_t *cur_cmd;
  dd_cmd_t *commands;
  int32_t commands_len;
  int32_t commands_cap;

  /* Vertex buffer */
  dd_vertex_t *verts_data;
  int32_t verts_len;
  int32_t verts_cap;

  /* Camera info */
  dd_mat4_t view;
  dd_mat4_t proj;
  dd_vec4_t viewport;
  dd_vec3_t view_origin;
  bool is_ortho;
  float proj_scale_y;

  dd_vec4_t frustum_planes[6];

#if DBGDRAW_HAS_STB_TRUETYPE
  /* Text info */
  dd_font_data_t* fonts;
  int32_t fonts_len;
  int32_t fonts_cap;
  int32_t active_font_idx;
#endif

  /* Render backend */
  dd_render_backend_t* backend;
  int32_t drawcall_count;
  dd_vec2_t aa_radius;
  uint8_t enable_depth_test;
} dd_ctx_t;


#ifdef __cplusplus
}
#endif

#endif /* DBGDRAW_H */



#ifdef DBGDRAW_IMPLEMENTATION

#ifdef DBGDRAW_VALIDATION_LAYERS

#define DBGDRAW_VALIDATE( cond, err_code ) do{                                            \
if( !(cond) )                                                                             \
{                                                                                         \
  DBGDRAW_LOG("vvvvvvvvvvvvvvvvvvvv  VALIDATION ERROR vvvvvvvvvvvvvvvv \n %s\n"           \
  "     Occured while calling function: %s\n", dd_error_message( err_code ), __func__ );  \
  DBGDRAW_ASSERT( false );                                                                \
}                                                                                         \
} while(0)
#else
#define DBGDRAW_VALIDATE( cond, err ) \
if( !(cond) )\
{\
  return err;\
}
#endif


int32_t
dd_init( dd_ctx_t *ctx, dd_ctx_desc_t* desc )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( desc );

  ctx->verts_len  = 0;
  ctx->verts_cap  = DD_MAX( 1024, desc->max_vertices );
  ctx->verts_data = DBGDRAW_MALLOC( ctx->verts_cap * sizeof(dd_vertex_t) );
  if( !ctx->verts_data )
  {
    return DBGDRAW_ERR_OK;
  }

  ctx->commands_len = 0;
  ctx->commands_cap = DD_MAX( 128, desc->max_commands );
  ctx->commands     = DBGDRAW_MALLOC( ctx->commands_cap * sizeof(dd_cmd_t) );
  if( !ctx->commands )
  {
    return DBGDRAW_ERR_OK;
  }

#if DBGDRAW_HAS_STB_TRUETYPE
  ctx->fonts_len = 0;
  ctx->fonts_cap = DD_MAX( 4, desc->max_fonts );
  ctx->fonts     = DBGDRAW_MALLOC( ctx->fonts_cap * sizeof(dd_font_data_t) );
  if( !ctx->fonts )
  {
    return DBGDRAW_ERR_OK;
  }
#endif

  ctx->cur_cmd           = NULL;
  ctx->color             = (dd_color_t){0, 0, 0, 255};
  ctx->detail_level      = DD_MAX( desc->detail_level, 0 );
  ctx->xform             = dd_mat4_identity();
  ctx->frustum_cull      = desc->enable_frustum_cull;
  ctx->primitive_size    = 1.5f;
  ctx->view              = dd_mat4_identity();
  ctx->proj              = dd_mat4_identity();
  ctx->aa_radius         = dd_vec2( desc->line_antialias_radius, 0.0f );
  ctx->enable_depth_test = desc->enable_depth_test;

  dd_backend_init( ctx );

  return DBGDRAW_ERR_OK;
}

int32_t
dd_term(dd_ctx_t *ctx)
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_FREE( ctx->verts_data );
  DBGDRAW_FREE( ctx->commands );

#if DBGDRAW_HAS_STB_TRUETYPE
  for( int32_t i = 0; i < ctx->fonts_len; ++i )
  {
    dd_font_data_t* font = ctx->fonts + i;
    DBGDRAW_FREE( font->name );
  }
  DBGDRAW_FREE(ctx->fonts );
#endif

  dd_backend_term( ctx );
  memset( ctx, 0, sizeof(dd_ctx_t) );

  return DBGDRAW_ERR_OK;
}

dd_color_t
dd_hex2color( uint32_t hex )
{
  uint8_t a = hex & 0x000000ff;
  uint8_t b = (hex & 0x0000ff00) >> 8;
  uint8_t g = (hex & 0x00ff0000) >> 16;
  uint8_t r = (hex & 0xff000000) >> 24;
  return (dd_color_t){r, g, b, a};
}

dd_color_t
dd_rgbf(float r, float g, float b)
{
  return (dd_color_t){(uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), 255};
}

dd_color_t
dd_rgbaf(float r, float g, float b, float a)
{
  return (dd_color_t){ (uint8_t)(r * 255),
                       (uint8_t)(g * 255),
                       (uint8_t)(b * 255),
                       (uint8_t)(a * 255)};
}

dd_color_t
dd_rgbu(uint8_t r, uint8_t g, uint8_t b)
{
  return (dd_color_t){r, g, b, 255};
}

dd_color_t
dd_rgbau(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  return (dd_color_t){r, g, b, a};
}

dd_color_t
dd_interpolate_color( dd_color_t c0, dd_color_t c1, float t )
{
  dd_color_t retcol;
  retcol.r = (uint8_t)((1.0 - t) * (float)c0.r + t * (float)c1.r);
  retcol.g = (uint8_t)((1.0 - t) * (float)c0.g + t * (float)c1.g);
  retcol.b = (uint8_t)((1.0 - t) * (float)c0.b + t * (float)c1.b);
  retcol.a = (uint8_t)((1.0 - t) * (float)c0.a + t * (float)c1.a);
  return retcol;
}

int32_t
dd_set_transform( dd_ctx_t *ctx, float* xform )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_VALIDATE( ctx->cur_cmd == NULL, DBGDRAW_ERR_PREV_CMD_NOT_ENDED );
  memcpy( ctx->xform.data, xform, sizeof(ctx->xform) );
  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_detail_level( dd_ctx_t *ctx, uint32_t level )
{
  DBGDRAW_ASSERT( ctx );
  ctx->detail_level = level;
  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_color( dd_ctx_t *ctx, dd_color_t color )
{
  DBGDRAW_ASSERT( ctx );
  ctx->color = color;
  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_primitive_size(dd_ctx_t *ctx, float primitive_size )
{
  DBGDRAW_ASSERT( ctx );
  ctx->primitive_size = primitive_size;
  return DBGDRAW_ERR_OK;
}


int32_t
dd_begin_cmd(dd_ctx_t *ctx, dd_mode_t draw_mode)
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( (int32_t)draw_mode >= 0 && (int32_t)draw_mode < (int32_t)DBGDRAW_MODE_COUNT );

  DBGDRAW_VALIDATE( ctx->cur_cmd == NULL, DBGDRAW_ERR_PREV_CMD_NOT_ENDED );
  DBGDRAW_VALIDATE( ctx->commands_len < ctx->commands_cap, DBGDRAW_ERR_OUT_OF_COMMAND_BUFFER );

  ctx->cur_cmd = &ctx->commands[ctx->commands_len];

  ctx->cur_cmd->xform        = ctx->xform;
  ctx->cur_cmd->min_depth    = 0.0f;
  ctx->cur_cmd->base_index   = ctx->verts_len;
  ctx->cur_cmd->vertex_count = 0;
  ctx->cur_cmd->draw_mode    = draw_mode;

#if DBGDRAW_HAS_STB_TRUETYPE
  ctx->cur_cmd->font_idx     = -1;
#endif

  return DBGDRAW_ERR_OK;
}

int32_t
dd_end_cmd(dd_ctx_t *ctx)
{
  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );

  ctx->commands_len++;
  ctx->cur_cmd = 0;

  return DBGDRAW_ERR_OK;
}

int32_t
dd__cmd_cmp( const void* a, const void* b )
{
  const dd_cmd_t* cmd_a = (const dd_cmd_t*)a;
  const dd_cmd_t* cmd_b = (const dd_cmd_t*)b;

  float key_a = (float)(cmd_a->draw_mode << 20) + cmd_a->min_depth;
  float key_b = (float)(cmd_b->draw_mode << 20) + cmd_b->min_depth;

  return (int32_t)DBGDRAW_ROUND( key_a - key_b );
}

void 
dd_sort_commands( dd_ctx_t* ctx )
{
  DBGDRAW_ASSERT( ctx );
  if( ctx->commands_len )
  {
    qsort( ctx->commands, ctx->commands_len, sizeof(ctx->commands[0]), dd__cmd_cmp );
#if 0
    const char* strings[4] = {"Point", "Line", "Triangle", "Text"};
    for( int32_t i = 0 ; i < ctx->commands_len; ++i )
    {
      float key = (float)(ctx->commands[i].draw_mode << 20) + ctx->commands[i].min_depth;
      printf("%d %s %f %f\n", i, strings[ctx->commands[i].draw_mode], ctx->commands[i].min_depth, key );
    }
    printf("-----------\n");
#endif
  }
}

int32_t
dd_render( dd_ctx_t *ctx )
{
  DBGDRAW_ASSERT( ctx );
  return dd_backend_render( ctx );
}

int32_t
dd_new_frame( dd_ctx_t *ctx, dd_new_frame_info_t* info )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( info );
  DBGDRAW_ASSERT( info->view_matrix );
  DBGDRAW_ASSERT( info->projection_matrix );
  DBGDRAW_ASSERT( info->viewport_size );

  ctx->xform          = dd_mat4_identity();
  ctx->verts_len      = 0;
  ctx->commands_len   = 0;
  ctx->drawcall_count = 0;
  ctx->is_ortho       = (info->projection_type == DBGDRAW_ORTHOGRAPHIC);

  memcpy( ctx->view.data, info->view_matrix, sizeof( ctx->view ) );
  memcpy( ctx->proj.data, info->projection_matrix, sizeof( ctx->proj ) );
  memcpy( ctx->viewport.data, info->viewport_size, sizeof( dd_vec4_t ) );
  
  dd_mat4_t inv_view_matrix = dd_mat4_se3_inverse( ctx->view );
  memcpy( ctx->view_origin.data, inv_view_matrix.col[3].data, sizeof( dd_vec3_t ) );

  if( ctx->is_ortho ) { ctx->proj_scale_y = 2.0 / ctx->proj.data[5]; }
  else                { ctx->proj_scale_y = 2.0f * tan( info->vertical_fov * 0.5f ); }
  
  dd_extract_frustum_planes( ctx );
  
  return DBGDRAW_ERR_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Frustum culling for higher order primitives
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// By fabian "rygorous" giessen
void dd__normalize_plane( dd_vec4_t *plane )
{
  float mag = sqrtf(plane->x * plane->x + plane->y * plane->y + plane->z * plane->z);
  plane->x /= mag;
  plane->y /= mag;
  plane->z /= mag;
  plane->w /= mag;
}

void dd_extract_frustum_planes( dd_ctx_t *ctx )
{
  DBGDRAW_ASSERT( ctx );
  dd_mat4_t pv = dd_mat4_transpose(dd_mat4_mul(ctx->proj, ctx->view));

  ctx->frustum_planes[0] = dd_vec4_add(pv.col[3], pv.col[2]);
  ctx->frustum_planes[1] = dd_vec4_sub(pv.col[3], pv.col[2]);
  ctx->frustum_planes[2] = dd_vec4_add(pv.col[3], pv.col[1]);
  ctx->frustum_planes[3] = dd_vec4_sub(pv.col[3], pv.col[1]);
  ctx->frustum_planes[4] = dd_vec4_add(pv.col[3], pv.col[0]);
  ctx->frustum_planes[5] = dd_vec4_sub(pv.col[3], pv.col[0]);

  dd__normalize_plane(&ctx->frustum_planes[0]);
  dd__normalize_plane(&ctx->frustum_planes[1]);
  dd__normalize_plane(&ctx->frustum_planes[2]);
  dd__normalize_plane(&ctx->frustum_planes[3]);
  dd__normalize_plane(&ctx->frustum_planes[4]);
  dd__normalize_plane(&ctx->frustum_planes[5]);
}

int32_t dd__frustum_sphere_test( dd_ctx_t *ctx, dd_vec3_t c, float radius )
{
  if( !ctx->frustum_cull )
  {
    return true;
  }
  dd_vec4_t xc = dd_mat4_vec4_mul(ctx->xform, dd_vec4(c.x, c.y, c.z, 1.0));
  for( int32_t i = 0; i < 6; ++i )
  {
    float dot = dd_vec4_dot(xc, ctx->frustum_planes[i]);
    if( dot <= -radius )
    {
      return false;
    }
  }
  return true;
}

int32_t dd__frustum_aabb_test(dd_ctx_t *ctx, dd_vec3_t min, dd_vec3_t max)
{
  if( !ctx->frustum_cull )
  {
    return true;
  }
  dd_vec4_t min_pt = dd_mat4_vec4_mul(ctx->xform, dd_vec4(min.x, min.y, min.z, 1.0));
  dd_vec4_t max_pt = dd_mat4_vec4_mul(ctx->xform, dd_vec4(max.x, max.y, max.z, 1.0));
  for( int32_t i = 0; i < 6; ++i )
  {
    const dd_vec4_t plane = ctx->frustum_planes[i];
    float d = DD_MAX(min_pt.x * plane.x, max_pt.x * plane.x) +
              DD_MAX(min_pt.y * plane.y, max_pt.y * plane.y) +
              DD_MAX(min_pt.z * plane.z, max_pt.z * plane.z) +
              plane.w;

    if( d < 0.0f )
    {
      return false;
    }
  }
  return true;
}

int32_t dd__frustum_obb_test(dd_ctx_t *ctx, dd_vec3_t c, dd_mat3_t axes)
{
  if( !ctx->frustum_cull )
  {
    return true;
  }

  dd_vec4_t xc = dd_mat4_vec4_mul(ctx->xform, dd_vec4(c.x, c.y, c.z, 1.0));
  dd_vec4_t xu = dd_mat4_vec4_mul(ctx->xform, dd_vec4(axes.col[0].x, axes.col[0].y, axes.col[0].z, 0.0));
  dd_vec4_t xv = dd_mat4_vec4_mul(ctx->xform, dd_vec4(axes.col[1].x, axes.col[1].y, axes.col[1].z, 0.0));
  dd_vec4_t xn = dd_mat4_vec4_mul(ctx->xform, dd_vec4(axes.col[2].x, axes.col[2].y, axes.col[2].z, 0.0));
  for( int32_t i = 0; i < 6; ++i )
  {
    const dd_vec4_t plane = ctx->frustum_planes[i];

    float pdotu = DD_ABS( dd_vec4_dot(plane, xu) );
    float pdotv = DD_ABS( dd_vec4_dot(plane, xv) );
    float pdotn = DD_ABS( dd_vec4_dot(plane, xn) );

    float effective_radius = (pdotu + pdotv + pdotn);

    float dot = dd_vec4_dot(xc, ctx->frustum_planes[i]);
    if( dot <= -effective_radius )
    {
      return false;
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float
dd__pixels_to_world_size( dd_ctx_t* ctx, dd_vec3_t pos, float pixels )
{
  DBGDRAW_ASSERT( ctx );

  dd_vec3_t v = dd_vec3_sub( pos, ctx->view_origin);
  float dist = ctx->is_ortho ? 1.0 : dd_vec3_norm( v );
  float viewport_height = ctx->viewport.w;
  float projected_size = ctx->proj_scale_y * (pixels / viewport_height);
  return dist * projected_size;

}

float
dd__world_size_to_pixels( dd_ctx_t* ctx, dd_vec3_t pos, float size )
{
  DBGDRAW_ASSERT( ctx );

  float dist = ctx->is_ortho ? 1.0 : dd_vec3_norm( dd_vec3_sub( pos, ctx->view_origin) );
  float viewport_height = ctx->viewport.w - ctx->viewport.y;
  return (size * viewport_height) / dist / ctx->proj_scale_y;
}

void 
dd__transform_verts( dd_mat4_t xform, dd_vertex_t *start, dd_vertex_t *end )
{
  for( dd_vertex_t *it = start; it != end; it++ )
  {
    dd_vec3_t pos = dd_mat4_vec3_mul(xform, dd_vec4_to_vec3( it->pos_size ), 1);
    it->pos_size = dd_vec4( pos.x, pos.y, pos.z, it->pos_size.w );
  }
}

dd_mat3_t
dd__get_view_aligned_basis( dd_ctx_t* ctx, dd_vec3_t p )
{
  DBGDRAW_ASSERT( ctx );
  dd_mat3_t m;
  dd_mat4_t inv_view = dd_mat4_se3_inverse( ctx->view );

  if( !ctx->is_ortho )
  {
    dd_vec3_t u = dd_vec4_to_vec3( inv_view.col[1] );
    dd_vec3_t v = dd_vec4_to_vec3( inv_view.col[3] );

    m.col[2] = dd_vec3_normalize( dd_vec3_sub(v, p) );

    m.col[0] = dd_vec3_normalize( dd_vec3_cross( m.col[2], u) );
    m.col[1] = dd_vec3_cross( m.col[0], m.col[2] );
  }
  else
  {
    m = dd_mat4_to_mat3( inv_view );
    m.col[0] = dd_vec3_invert( m.col[0] );
  }
  return m;
}

// NOTE(maciej):
// Idea here is to calculate transformation that will map a cylinder with ends at (0,0,0) and (0,1,0) and radius 1
// to a cylinder with endpoints q0 and q1 and radius "radius"
dd_mat4_t
dd__generate_cone_orientation( dd_vec3_t q0, dd_vec3_t q1 )
{
  dd_vec3_t n = dd_vec3_sub(q1, q0);
  dd_vec3_t u0 = dd_vec3( n.z, n.z, -n.x - n.y );
  dd_vec3_t u1 = dd_vec3( -n.y - n.z, n.x, n.x );
  dd_vec3_t u = ((n.z != 0) && (-n.x != n.y)) ? u0 : u1;
  dd_vec3_t v = dd_vec3_cross(u, n);

  n = dd_vec3_normalize(n);
  u = dd_vec3_normalize(u);
  v = dd_vec3_normalize(v);

  dd_mat4_t rot = dd_mat4_identity();
  rot.col[0] = dd_vec3_to_vec4(u);
  rot.col[1] = dd_vec3_to_vec4(n);
  rot.col[2] = dd_vec3_to_vec4(v);
  float det = dd_mat4_determinant(rot);
  rot.col[2] = dd_vec4_scalar_mul(rot.col[2], det);

  return rot;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Draw Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
dd__vertex( dd_ctx_t *ctx, dd_vec3_t pt )
{
  ctx->verts_data[ctx->verts_len++] = (dd_vertex_t){ .pos_size = dd_vec4( pt.x, pt.y, pt.z, ctx->primitive_size),
                                                     .uv = (dd_vec2_t){{0.0f, 0.0f}},
                                                     .col = ctx->color};
  ctx->cur_cmd->vertex_count++;
}

void
dd__vertex_text( dd_ctx_t *ctx, dd_vec3_t pt, dd_vec2_t uv )
{
  dd_color_t c = ctx->color;
  c.a = 0;
  ctx->verts_data[ctx->verts_len++] = (dd_vertex_t){ .pos_size = dd_vec4( pt.x, pt.y, pt.z, ctx->primitive_size ),
                                                     .uv = uv,
                                                     .col = c};
  ctx->cur_cmd->vertex_count++;
}

void
dd__line( dd_ctx_t *ctx, dd_vec3_t pt_a, dd_vec3_t pt_b )
{
  dd__vertex( ctx, pt_a );
  dd__vertex( ctx, pt_b );
}

void
dd__triangle( dd_ctx_t *ctx, dd_vec3_t a, dd_vec3_t b, dd_vec3_t c )
{
  dd__vertex( ctx, a );
  dd__vertex( ctx, b );
  dd__vertex( ctx, c );
}

void 
dd__quad_point( dd_ctx_t *ctx, dd_vec3_t pts[4] )
{
  dd__vertex( ctx, pts[0] );
  dd__vertex( ctx, pts[1] );
  dd__vertex( ctx, pts[2] );
  dd__vertex( ctx, pts[3] );
}

void
dd__quad_stroke( dd_ctx_t *ctx, dd_vec3_t pts[4] )
{
  dd__line( ctx, pts[0], pts[1] );
  dd__line( ctx, pts[1], pts[2] );
  dd__line( ctx, pts[2], pts[3] );
  dd__line( ctx, pts[3], pts[0] );
}

void 
dd__quad_fill( dd_ctx_t *ctx, dd_vec3_t pts[4] )
{
  dd__triangle(ctx, pts[0], pts[1], pts[2]);
  dd__triangle(ctx, pts[0], pts[2], pts[3]);
}

void
dd__quad( dd_ctx_t* ctx, dd_vec3_t pts[4] )
{
  switch( ctx->cur_cmd->draw_mode )
  {
    case DBGDRAW_MODE_POINT:
      dd__quad_point( ctx, pts );
      break;
    case DBGDRAW_MODE_STROKE:
      dd__quad_stroke( ctx, pts );
      break;
    case DBGDRAW_MODE_FILL:
      dd__quad_fill( ctx, pts );
      break;
    default:
      break;
  }
}

void
dd__box_point( dd_ctx_t *ctx, dd_vec3_t pts[8] )
{
  dd__vertex( ctx, pts[0]);
  dd__vertex( ctx, pts[1]);
  dd__vertex( ctx, pts[2]);
  dd__vertex( ctx, pts[3]);

  dd__vertex( ctx, pts[4]);
  dd__vertex( ctx, pts[5]);
  dd__vertex( ctx, pts[6]);
  dd__vertex( ctx, pts[7]);
}

void 
dd__box_stroke( dd_ctx_t *ctx, dd_vec3_t pts[8] )
{
  dd__quad_stroke(ctx, pts);
  dd__quad_stroke(ctx, pts + 4);

  dd__line(ctx, pts[0], pts[4]);
  dd__line(ctx, pts[1], pts[5]);
  dd__line(ctx, pts[2], pts[6]);
  dd__line(ctx, pts[3], pts[7]);
}

void 
dd__box_fill( dd_ctx_t *ctx, dd_vec3_t pts[8] )
{
  dd__quad_fill(ctx, (dd_vec3_t[4]){pts[3], pts[2], pts[1], pts[0]});
  dd__quad_fill(ctx, (dd_vec3_t[4]){pts[4], pts[5], pts[6], pts[7]});
  dd__quad_fill(ctx, (dd_vec3_t[4]){pts[1], pts[5], pts[4], pts[0]});
  dd__quad_fill(ctx, (dd_vec3_t[4]){pts[2], pts[6], pts[5], pts[1]});
  dd__quad_fill(ctx, (dd_vec3_t[4]){pts[3], pts[7], pts[6], pts[2]});
  dd__quad_fill(ctx, (dd_vec3_t[4]){pts[0], pts[4], pts[7], pts[3]});
}

void
dd__box( dd_ctx_t* ctx, dd_vec3_t pts[8] )
{
  switch( ctx->cur_cmd->draw_mode )
  {
    case DBGDRAW_MODE_POINT:
      dd__box_point( ctx, pts );
      break;
    case DBGDRAW_MODE_STROKE:
      dd__box_stroke( ctx, pts );
      break;
    case DBGDRAW_MODE_FILL:
      dd__box_fill( ctx, pts );
      break;
    default:
      break;
  }
}

void
dd__arc_point( dd_ctx_t *ctx, dd_vec3_t center, float radius, float theta, float resolution )
{
  resolution = DD_MAX( 4, resolution );
  float d_theta = theta / resolution;
  int32_t full_circle = (int32_t)(!(theta < DBGDRAW_TWO_PI));

  if( !full_circle )
  {
    dd__vertex( ctx, center );
  }

  float theta1;
  float ox1 = 0.0f, oy1 = 0.0f;
  int32_t final_res = full_circle ? resolution : resolution + 1;
  for( int32_t i = 0; i < final_res; ++i )
  {
    theta1 = i * d_theta;
    ox1 = radius * DBGDRAW_SIN( theta1 );
    oy1 = radius * DBGDRAW_COS( theta1 );

    dd__vertex( ctx, dd_vec3( center.x + ox1, center.y + oy1, center.z ) );
  }
}

void
dd__arc_stroke( dd_ctx_t *ctx, dd_vec3_t center, float radius, float theta, float resolution )
{
  float d_theta = theta / resolution;
  int32_t full_circle = (int32_t)(!(theta < DBGDRAW_TWO_PI));

  if( !full_circle )
  {
    dd__line(ctx, center, dd_vec3(center.x, center.y + radius, center.z));
  }

  float theta1, theta2;
  float ox1, ox2, oy1, oy2;
  ox1 = ox2 = oy1 = oy2 = 0.0f;
  for( int32_t i = 0; i < resolution; ++i )
  {
    theta1 = i * d_theta;
    theta2 = (i + 1) * d_theta;
    ox1 = radius * DBGDRAW_SIN( theta1 );
    ox2 = radius * DBGDRAW_SIN( theta2 );
    oy1 = radius * DBGDRAW_COS( theta1 );
    oy2 = radius * DBGDRAW_COS( theta2 );

    dd__line(ctx, dd_vec3( center.x + ox1, center.y + oy1, center.z ),
                       dd_vec3( center.x + ox2, center.y + oy2, center.z ) );
  }

  if( !full_circle )
  {
    dd__line( ctx, dd_vec3( center.x + ox2, center.y + oy2, center.z ), center );
  }
}

void
dd__arc_fill( dd_ctx_t *ctx, dd_vec3_t center, float radius, float theta, float resolution, uint8_t flip )
{
  float d_theta = theta / resolution;

  float theta1, theta2;
  float ox1, ox2, oy1, oy2;
  ox1 = ox2 = oy1 = oy2 = 0.0f;
  for( int32_t i = 0; i < resolution; ++i )
  {
    theta1 = i * d_theta;
    theta2 = (i + 1) * d_theta;
    ox1 = radius * DBGDRAW_SIN( theta1 );
    ox2 = radius * DBGDRAW_SIN( theta2 );
    oy1 = radius * DBGDRAW_COS( theta1 );
    oy2 = radius * DBGDRAW_COS( theta2 );

    dd_vec3_t p1 = dd_vec3( center.x + ox1, center.y + oy1, center.z );
    dd_vec3_t p2 = dd_vec3( center.x + ox2, center.y + oy2, center.z );

    if( flip ) { dd__triangle(ctx, center, p1, p2); }
    else       { dd__triangle(ctx, center, p2, p1); }
  }
}

void
dd__arc( dd_ctx_t* ctx, dd_vec3_t center, float radius, float theta, int32_t resolution, uint8_t flip )
{
  switch( ctx->cur_cmd->draw_mode )
  {
    case DBGDRAW_MODE_POINT:
      dd__arc_point( ctx, center, radius, theta, resolution >> 1 );
      break;
    case DBGDRAW_MODE_STROKE:
      dd__arc_stroke( ctx, center, radius, theta, resolution );
      break;
    case DBGDRAW_MODE_FILL:
      dd__arc_fill( ctx, center, radius, theta, resolution, flip );
      break;
    default:
      break;
  }
}

//NOTE(maciej): dd_arc(...) deals with both points and strokes
void
dd__sphere_point_stroke( dd_ctx_t *ctx, dd_vec3_t c, float radius, int32_t resolution )
{
  /* Store initial state */
  const int32_t n_rings = 3;
  dd_vertex_t *base_ptr = ctx->verts_data + ctx->verts_len;

  /* Create unit circle, and get data boundaries */
  dd_vertex_t *start_ptr = base_ptr;
  dd__arc( ctx, dd_vec3( 0, 0, 0 ), 1.0f, DBGDRAW_TWO_PI, resolution, 0 );
  dd_vertex_t *end_ptr = ctx->verts_data + ctx->verts_len;
  size_t len = end_ptr - start_ptr;
  int32_t new_verts = n_rings * len;

  /* Copy the memory, so that we have all the rings we need. */
  for( int32_t i = 1; i < n_rings; ++i )
  {
    memcpy( end_ptr, start_ptr, len * sizeof(dd_vertex_t) );
    end_ptr += len;
    start_ptr += len;
  }

  dd_mat4_t xform;

  /* Circle A */
  xform = dd_pre_scale(dd_mat4_identity(), dd_vec3(radius, radius, radius));
  xform = dd_pre_translate(xform, c);
  start_ptr = base_ptr;
  end_ptr = start_ptr + len;
  dd__transform_verts(xform, start_ptr, end_ptr);

  /* Circle B */
  xform = dd_pre_scale(dd_mat4_identity(), dd_vec3(radius, radius, radius));
  xform = dd_pre_rotate(xform, DBGDRAW_PI_OVER_TWO, dd_vec3( 1, 0, 0 ));
  xform = dd_pre_translate(xform, c);
  start_ptr = end_ptr;
  end_ptr = start_ptr + len;
  dd__transform_verts(xform, start_ptr, end_ptr);

  /* Circle C */
  xform = dd_pre_scale(dd_mat4_identity(), dd_vec3(radius, radius, radius));
  xform = dd_pre_rotate(xform, DBGDRAW_PI_OVER_TWO, dd_vec3( 0, 1, 0 ) );
  xform = dd_pre_translate(xform, c);
  start_ptr = end_ptr;
  end_ptr = start_ptr + len;
  dd__transform_verts(xform, start_ptr, end_ptr);

  /* Tell the context that there is new vertex data */
  ctx->verts_len += (new_verts - (int32_t)len);
  ctx->cur_cmd->vertex_count += (new_verts - (int32_t)len);
}

void
dd__sphere_fill( dd_ctx_t *ctx, dd_vec3_t c, float radius, int32_t resolution )
{
  float half_pi = DBGDRAW_PI_OVER_TWO;
  float half_res = resolution >> 1;
  float prev_y = -1.0;
  float prev_r = 0.0f;
  for( int32_t i = 1; i <= half_res; ++i )
  {
    float phi = ((i / half_res) * 2.0 - 1.0f) * half_pi;
    float curr_r = DBGDRAW_COS( phi ) * radius;
    float curr_y = DBGDRAW_SIN( phi );

    float prev_x = 1.0f;
    float prev_z = 0.0f;

    for( int32_t j = 1; j <= resolution; ++j )
    {
      float theta = (j / (float)resolution) * DBGDRAW_TWO_PI;
      float curr_z = DBGDRAW_SIN( theta );
      float curr_x = DBGDRAW_COS( theta );

      dd__vertex(ctx, dd_vec3(c.x + prev_x * prev_r, c.y + prev_y * radius, c.z + prev_z * prev_r));
      dd__vertex(ctx, dd_vec3(c.x + curr_x * curr_r, c.y + curr_y * radius, c.z + curr_z * curr_r));
      dd__vertex(ctx, dd_vec3(c.x + prev_x * curr_r, c.y + curr_y * radius, c.z + prev_z * curr_r));

      dd__vertex(ctx, dd_vec3(c.x + prev_x * prev_r, c.y + prev_y * radius, c.z + prev_z * prev_r));
      dd__vertex(ctx, dd_vec3(c.x + curr_x * prev_r, c.y + prev_y * radius, c.z + curr_z * prev_r));
      dd__vertex(ctx, dd_vec3(c.x + curr_x * curr_r, c.y + curr_y * radius, c.z + curr_z * curr_r));
      prev_x = curr_x;
      prev_z = curr_z;
    }
    prev_y = curr_y;
    prev_r = curr_r;
  }
}

void
dd__sphere( dd_ctx_t *ctx, dd_vec3_t c, float radius, int32_t resolution )
{
  switch( ctx->cur_cmd->draw_mode )
  {
    case DBGDRAW_MODE_POINT:
    case DBGDRAW_MODE_STROKE:
      dd__sphere_point_stroke( ctx, c, radius, resolution );
      break;
    case DBGDRAW_MODE_FILL:
      dd__sphere_fill( ctx, c, radius, resolution );
      break;
    default:
      break;
  }
}

// NOTE(maciej): This transformation will transform a cone with a radius 1 base at (0,0,0) and apex at (0, 0, 1) to a
//               cone that has base at p and oriented as specified by rot, with height and radius controlled by
//               parameters.
dd_mat4_t
dd__get_cone_xform( dd_vec3_t p, dd_mat4_t rot, float radius, float height )
{
  dd_mat4_t xform = dd_pre_rotate( dd_mat4_identity(), DBGDRAW_PI_OVER_TWO, dd_vec3( -1.0f, 0.0f, 0.0f ) );
  xform = dd_pre_scale( xform, dd_vec3( radius, height, radius ) );
  xform = dd_mat4_mul( rot, xform );
  
  xform = dd_pre_translate( xform, p );
  return xform;
}

void
dd__cone( dd_ctx_t *ctx, dd_vec3_t a, dd_vec3_t b, float radius, int32_t resolution )
{
  dd_mat4_t rot = dd__generate_cone_orientation( a, b );
  float height = dd_vec3_norm( dd_vec3_sub( a, b ) );

  dd_mat4_t xform = dd__get_cone_xform( a, rot, radius, height );

  dd_vertex_t *start_ptr = ctx->verts_data + ctx->verts_len;
  dd__arc( ctx, dd_vec3(0.0, 0.0, 0.0), 1.0, DBGDRAW_TWO_PI, resolution, 1 );
  dd_vertex_t *end_ptr = ctx->verts_data + ctx->verts_len;
  dd__transform_verts(xform, start_ptr, end_ptr);
 
  dd_vec3_t apex = dd_mat4_vec3_mul( xform, dd_vec3(0.0, 0.0, 1.0 ), 1 );

  switch( ctx->cur_cmd->draw_mode )
  {
    case DBGDRAW_MODE_POINT:
      dd__vertex( ctx, apex );
      break;
    case DBGDRAW_MODE_STROKE:
      for( int32_t i = 0; i < (resolution >> 1); ++i )
      {
        dd__line(ctx, dd_vec4_to_vec3(start_ptr->pos_size), apex);
        start_ptr += 4;
      }
      break;
    case DBGDRAW_MODE_FILL:
      for( int32_t i = 0; i < resolution; ++i )
      {
        start_ptr++;
        dd_vec3_t p1 = dd_vec4_to_vec3(start_ptr->pos_size);
        start_ptr++;
        dd_vec3_t p2 = dd_vec4_to_vec3(start_ptr->pos_size);
        start_ptr++;
        dd__triangle(ctx, p2, p1, apex);
      }
      break;
    default:
      break;
  }
}

void 
dd__conical_frustum( dd_ctx_t *ctx, dd_vec3_t a, dd_vec3_t b, float radius_a, float radius_b,
                          int32_t resolution )
{
  dd_mat4_t rot = dd__generate_cone_orientation( a, b );
  float height = dd_vec3_norm( dd_vec3_sub( a, b ) );

  dd_mat4_t xform_a = dd__get_cone_xform( a, rot, radius_a, height );
  dd_mat4_t xform_b = dd__get_cone_xform( a, rot, radius_b, height );

  dd_vertex_t *start_ptr_1 = ctx->verts_data + ctx->verts_len;
  dd__arc( ctx, dd_vec3(0.0, 0.0, 0.0), 1.0, DBGDRAW_TWO_PI, resolution, 1 );
  dd_vertex_t *end_ptr_1 = ctx->verts_data + ctx->verts_len;
  dd__transform_verts(xform_a, start_ptr_1, end_ptr_1);
 
  dd_vertex_t *start_ptr_2 = ctx->verts_data + ctx->verts_len;
  dd__arc( ctx, dd_vec3(0.0, 0.0, 1.0), 1.0, DBGDRAW_TWO_PI, resolution, 0 );
  dd_vertex_t *end_ptr_2 = ctx->verts_data + ctx->verts_len;
  dd__transform_verts(xform_b, start_ptr_2, end_ptr_2);

  switch( ctx->cur_cmd->draw_mode )
  {
    case DBGDRAW_MODE_POINT:
      break;
    case DBGDRAW_MODE_STROKE:
      for( int32_t i = 0; i < resolution >> 1; ++i )
      {
        dd__line(ctx, dd_vec4_to_vec3( start_ptr_1->pos_size ), dd_vec4_to_vec3( start_ptr_2->pos_size ) );
        start_ptr_1 += 4;
        start_ptr_2 += 4;
      }
      break;
    case DBGDRAW_MODE_FILL:
      for( int32_t i = 0; i < resolution; ++i )
      {
        start_ptr_1++;
        start_ptr_2++;
        dd_vec3_t p1 = dd_vec4_to_vec3( start_ptr_1->pos_size );
        start_ptr_1++;
        dd_vec3_t p2 = dd_vec4_to_vec3( start_ptr_1->pos_size );
        dd_vec3_t p3 = dd_vec4_to_vec3( start_ptr_2->pos_size );
        start_ptr_2++;
        dd_vec3_t p4 = dd_vec4_to_vec3( start_ptr_2->pos_size );
        start_ptr_1++;
        start_ptr_2++;
        dd__triangle(ctx, p2, p1, p3);
        dd__triangle(ctx, p1, p4, p3);
      }
      break;
    default:
      break;
  }
}

void
dd__torus_point_stroke( dd_ctx_t *ctx, dd_vec3_t center, float radius_a, float radius_b,
                             int32_t resolution, int32_t n_small_rings )
{
  /* Store useful data */
  dd_vertex_t *base_ptr = ctx->verts_data + ctx->verts_len;
  dd_mat4_t xform = dd_mat4_identity();
  dd_mat4_t rotx = dd_pre_rotate( xform, DBGDRAW_PI_OVER_TWO, dd_vec3( 1.0, 0.0, 0.0 ) );
  float radius = 1.0f;
  dd_vertex_t *start_ptr = base_ptr;
  dd_vertex_t *end_ptr = base_ptr;
  size_t len;

  /* We do large circles only for stroke mode */
  if( ctx->cur_cmd->draw_mode == DBGDRAW_MODE_STROKE )
  {
    /* create unit circle, and get data boundaries */
    start_ptr = base_ptr;
    dd__arc( ctx, dd_vec3( 0, 0, 0 ), 1.0f, DBGDRAW_TWO_PI, resolution, 0 );
    end_ptr = ctx->verts_data + ctx->verts_len;
    len = end_ptr - start_ptr;

    /* copy the memory, so that we have all the rings we need. Also make sure to move verts_len accordingly */
    for( int32_t i = 1; i < 4; ++i )
    {
      memcpy( end_ptr, start_ptr, len * sizeof(dd_vertex_t) );
      end_ptr += len;
      start_ptr += len;
    }

    /* Large outer circle */
    radius = radius_a + radius_b;
    xform = dd_pre_scale(  rotx, dd_vec3( radius, radius, radius ) );
    xform = dd_pre_translate( xform, center );
    start_ptr = base_ptr;
    end_ptr = start_ptr + len;
    dd__transform_verts( xform, start_ptr, end_ptr );

    /* Smaller inner circle */
    radius = radius_a - radius_b;
    xform = dd_pre_scale(  rotx, dd_vec3( radius, radius, radius ) );
    xform = dd_pre_translate( xform, center );
    start_ptr = end_ptr;
    end_ptr = start_ptr + len;
    dd__transform_verts( xform, start_ptr, end_ptr );

    /* Medium top circle */
    xform = dd_pre_scale(  rotx, dd_vec3( radius_a, radius_a, radius_a ) );
    xform = dd_pre_translate( xform, dd_vec3_add( dd_vec3( 0.0f, radius_b, 0.0f ), center ) );
    start_ptr = end_ptr;
    end_ptr = start_ptr + len;
    dd__transform_verts( xform, start_ptr, end_ptr );

    /* Medium bottom circle */
    xform = dd_pre_scale(  rotx, dd_vec3( radius_a, radius_a, radius_a ) );
    xform = dd_pre_translate( xform, dd_vec3_add( center, dd_vec3(0.0f, -radius_b, 0.0f) ) );
    start_ptr = end_ptr;
    end_ptr = start_ptr + len;
    dd__transform_verts( xform, start_ptr, end_ptr );

    ctx->verts_len += len * 3;
    ctx->cur_cmd->vertex_count += len * 3;
  }

  /* create data for small circles, and get data boundaries */
  base_ptr = end_ptr;
  start_ptr = base_ptr;
  int32_t res = DD_MAX( resolution >> 1, 4 );
  dd__arc( ctx, dd_vec3( 0, 0, 0 ), 1.0f, DBGDRAW_TWO_PI, res, 0 );
  end_ptr = ctx->verts_data + ctx->verts_len;
  len = end_ptr - start_ptr;

  /* copy the memory, so that we have all the rings we need. Also make sure to move verts_len accordingly */
  for( int32_t i = 1; i < n_small_rings; ++i )
  {
    memcpy(end_ptr, start_ptr, len * sizeof(dd_vertex_t));
    end_ptr += len;
    start_ptr += len;
  }
  start_ptr = base_ptr;
  end_ptr = start_ptr + len;

  /* small circles around the torus */
  float theta = DBGDRAW_TWO_PI / n_small_rings;
  xform = dd_pre_scale( rotx, dd_vec3(radius_b, radius_b, radius_b));
  xform = dd_pre_rotate(xform, DBGDRAW_PI_OVER_TWO, dd_vec3( 1, 0, 0 ));
  xform = dd_pre_translate(xform, dd_vec3(radius_a, 0.0, 0.0f));
  for( int32_t i = 0; i < n_small_rings; ++i )
  {
    xform = dd_pre_rotate(xform, theta, dd_vec3( 0, 1, 0) );
    dd_mat4_t cxform = dd_pre_translate(xform, center);

    dd__transform_verts(cxform, start_ptr, end_ptr);
    start_ptr = end_ptr;
    end_ptr = start_ptr + len;
  }

  /* Tell the context that there is new vertex data */
  ctx->verts_len += len * (n_small_rings - 1);
  ctx->cur_cmd->vertex_count += len * (n_small_rings - 1);
}

void
dd__torus_fill( dd_ctx_t *ctx, dd_vec3_t center, float radius_a, float radius_b,
                     int32_t resolution, int32_t n_small_rings )
{
  (void) n_small_rings;

  int32_t res_a = resolution;
  int32_t res_b = DD_MAX( 4, resolution >> 1 );
  float step_a = DBGDRAW_TWO_PI / (float)res_a;
  float step_b = DBGDRAW_TWO_PI / (float)res_b;
  float ax = radius_a;
  float ay = 0.0f;
  float az = 0.0f;
  dd_vec3_t p1, p2, p3;

  for( int32_t i = 0; i < res_a; ++i )
  {
    float c1 = DBGDRAW_COS(i * step_a);
    float c2 = DBGDRAW_COS((i + 1) * step_a);
    float s1 = DBGDRAW_SIN(i * step_a);
    float s2 = DBGDRAW_SIN((i + 1) * step_a);

    float s1az = s1 * az;
    float c1az = c1 * az;
    float s2az = s2 * az;
    float c2az = c2 * az;

    for( int32_t j = 0; j < res_b; ++j )
    {
      float bx0 = (ax + radius_b * DBGDRAW_COS(j * step_b));
      float by0 = (ay + radius_b * DBGDRAW_SIN(j * step_b));

      float bx1 = (ax + radius_b * DBGDRAW_COS((j + 1) * step_b));
      float by1 = (ay + radius_b * DBGDRAW_SIN((j + 1) * step_b));

      float bx2 = c1 * bx0 - s1az;
      float bz2 = s1 * bx0 + c1az;

      float bx3 = c1 * bx1 - s1az;
      float bz3 = s1 * bx1 + c1az;

      float bx4 = c2 * bx0 - s2az;
      float bz4 = s2 * bx0 + c2az;

      float bx5 = c2 * bx1 - s2az;
      float bz5 = s2 * bx1 + c2az;

      p1.x = bx2;
      p1.y = by0, p1.z = bz2;
      p2.x = bx4;
      p2.y = by0, p2.z = bz4;
      p3.x = bx3;
      p3.y = by1, p3.z = bz3;
      p1 = dd_vec3_add( p1, center );
      p2 = dd_vec3_add( p2, center );
      p3 = dd_vec3_add( p3, center );
      dd__triangle( ctx, p1, p2, p3 );

      p1.x = bx3;
      p1.y = by1, p1.z = bz3;
      p2.x = bx4;
      p2.y = by0, p2.z = bz4;
      p3.x = bx5;
      p3.y = by1, p3.z = bz5;
      p1 = dd_vec3_add( p1, center );
      p2 = dd_vec3_add( p2, center );
      p3 = dd_vec3_add( p3, center );
      dd__triangle( ctx, p1, p2, p3 );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Draw Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t
dd_point( dd_ctx_t *ctx, float* a )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( a );

  int32_t new_verts = 1;
  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  dd__vertex( ctx, dd_vec3( a[0], a[1], a[2] ) );

  return DBGDRAW_ERR_OK;
}

int32_t
dd_line(dd_ctx_t *ctx, float* a, float* b)
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( a );
  DBGDRAW_ASSERT( b );

  int32_t new_verts = 2;
  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );


  dd__line( ctx, dd_vec3( a[0], a[1], a[2] ), dd_vec3( b[0], b[1], b[2] ) );

  return DBGDRAW_ERR_OK;
}

int32_t
dd_quad( dd_ctx_t *ctx, float* a, float* b, float* c, float* d )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( a );
  DBGDRAW_ASSERT( b );
  DBGDRAW_ASSERT( c );
  DBGDRAW_ASSERT( d );

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = 4;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 8;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 6;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];
  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  // TODO(maciej): Just pass for float pointers to dd__quad
  dd_vec3_t arr[4] =
  {
    dd_vec3( a[0], a[1], a[2] ),
    dd_vec3( b[0], b[1], b[2] ),
    dd_vec3( c[0], c[1], c[2] ),
    dd_vec3( d[0], d[1], d[2] ),
  };
  dd__quad( ctx, arr );

  return DBGDRAW_ERR_OK;
}

int32_t
dd_rect( dd_ctx_t *ctx, float* a, float* b )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( a );
  DBGDRAW_ASSERT( b );

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = 4;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 8;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 6;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];
  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  // TODO(maciej): Just pass for float pointers to dd__quad
  dd_vec3_t arr[4] =
  {
    dd_vec3( a[0], a[1], a[2] ),
    dd_vec3( a[0], b[1], a[2] ),
    dd_vec3( b[0], b[1], b[2] ),
    dd_vec3( b[0], a[1], b[2] ),
  };
  dd__quad( ctx, arr );

  return DBGDRAW_ERR_OK;
}

// TODO(maciej): Sizes should be in pixel size..
int32_t
dd_billboard_rect(dd_ctx_t *ctx, float* p, float width, float height)
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( p );

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = 4;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 8;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 6;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];
  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  dd_vec3_t pt = dd_vec3( p[0], p[1], p[2] );
  dd_mat3_t m = dd__get_view_aligned_basis(ctx, pt);
  m.col[0] = dd_vec3_scalar_mul(m.col[0], width * 0.5f);
  m.col[1] = dd_vec3_scalar_mul(m.col[1], height * 0.5f);

  dd_vec3_t pts[4] =
  {
    dd_vec3_sub(pt, dd_vec3_add(m.col[0], m.col[1])),
    dd_vec3_add(pt, dd_vec3_sub(m.col[0], m.col[1])),
    dd_vec3_add(pt, dd_vec3_add(m.col[0], m.col[1])),
    dd_vec3_sub(pt, dd_vec3_sub(m.col[0], m.col[1])),
  };

  dd__quad( ctx, pts );
  
  return DBGDRAW_ERR_OK;
}

int32_t
dd_circle(dd_ctx_t *ctx, float* c, float radius)
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( c );

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = resolution;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 2*resolution;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 3*resolution;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  dd__arc( ctx, dd_vec3( c[0], c[1], c[2] ), radius, DBGDRAW_TWO_PI, resolution, 0 );

  return DBGDRAW_ERR_OK;
}

int32_t
dd_billboard_circle( dd_ctx_t *ctx, float* c, float radius )
{
  DBGDRAW_ASSERT( ctx );
  if( !ctx->cur_cmd )
  {
    return DBGDRAW_ERR_NO_ACTIVE_CMD;
  }

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = resolution;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 2*resolution;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 3*resolution;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  if( ctx->verts_len + new_verts >= ctx->verts_cap )
  {
    return DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER;
  }

  dd_vertex_t *start = ctx->verts_data + ctx->verts_len;
  dd__arc( ctx, dd_vec3( 0, 0, 0 ), radius, DBGDRAW_TWO_PI, resolution, 0 );
  dd_vertex_t *end = ctx->verts_data + ctx->verts_len;

  dd_vec3_t cp = dd_vec3( c[0], c[1], c[2]);
  dd_mat3_t m = dd__get_view_aligned_basis( ctx, cp );
  dd_mat4_t xform = dd_mat4_identity();
  xform.col[0] = dd_vec3_to_vec4( m.col[0] );
  xform.col[1] = dd_vec3_to_vec4( m.col[1] );
  xform.col[2] = dd_vec3_to_vec4( m.col[2] );
  xform.col[3] = dd_vec3_to_vec4( cp );
  xform.col[3].w = 1.0f;
  dd__transform_verts( xform, start, end );

  return DBGDRAW_ERR_OK;
}

int32_t
dd_arc( dd_ctx_t *ctx, float* c, float radius, float theta )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( c );

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = resolution+1;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 2*resolution;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 3*resolution;
  int32_t new_verts = mode_vert_count[ ctx->cur_cmd->draw_mode ];

  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  dd__arc( ctx, dd_vec3( c[0], c[1], c[2] ), radius, theta, resolution, 0 );

  return DBGDRAW_ERR_OK;
}

int32_t
dd_aabb(dd_ctx_t *ctx, float* a, float* b)
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( a );
  DBGDRAW_ASSERT( b );

  dd_vec3_t pt_a = dd_vec3( a[0], a[1], a[2] );
  dd_vec3_t pt_b = dd_vec3( b[0], b[1], b[2] );
  if( !dd__frustum_aabb_test( ctx, pt_a, pt_b ) )
  {
    return DBGDRAW_ERR_CULLED;
  }

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = 8;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 24;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 36;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  dd_vec3_t pts[8] =
  {
    pt_a,
    dd_vec3( b[0], a[1], a[2] ),
    dd_vec3( b[0], a[1], b[2] ),
    dd_vec3( a[0], a[1], b[2] ),

    dd_vec3( a[0], b[1], a[2] ),
    dd_vec3( b[0], b[1], a[2] ),
    pt_b,
    dd_vec3( a[0], b[1], b[2] ),
  };

  dd__box( ctx, pts );

  return DBGDRAW_ERR_OK;
}

int32_t
dd_obb( dd_ctx_t *ctx, float* c, float* m )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( c );
  DBGDRAW_ASSERT( m );


  dd_vec3_t center_pt = dd_vec3( c[0], c[1], c[2] );
  dd_mat3_t axes;
  memcpy( axes.data, m, sizeof(axes) ); // TODO(maciej): do we need all these memcpys?

  if( !dd__frustum_obb_test( ctx, center_pt, axes ) )
  {
    return DBGDRAW_ERR_CULLED;
  }

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = 8;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 24;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 36;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  dd_vec3_t v1 = axes.col[0];
  dd_vec3_t v2 = axes.col[1];
  dd_vec3_t v3 = axes.col[2];
  dd_vec3_t pts[8] =
  {
    dd_vec3( center_pt.x - v1.x + v2.x + v3.x, center_pt.y  - v1.y + v2.y + v3.y, center_pt.z - v1.z + v2.z + v3.z ),
    dd_vec3( center_pt.x + v1.x + v2.x + v3.x, center_pt.y  + v1.y + v2.y + v3.y, center_pt.z + v1.z + v2.z + v3.z ),
    dd_vec3( center_pt.x + v1.x + v2.x - v3.x, center_pt.y  + v1.y + v2.y - v3.y, center_pt.z + v1.z + v2.z - v3.z ),
    dd_vec3( center_pt.x - v1.x + v2.x - v3.x, center_pt.y  - v1.y + v2.y - v3.y, center_pt.z - v1.z + v2.z - v3.z ),

    dd_vec3( center_pt.x - v1.x - v2.x + v3.x, center_pt.y  - v1.y - v2.y + v3.y, center_pt.z - v1.z - v2.z + v3.z ),
    dd_vec3( center_pt.x + v1.x - v2.x + v3.x, center_pt.y  + v1.y - v2.y + v3.y, center_pt.z + v1.z - v2.z + v3.z ),
    dd_vec3( center_pt.x + v1.x - v2.x - v3.x, center_pt.y  + v1.y - v2.y - v3.y, center_pt.z + v1.z - v2.z - v3.z ),
    dd_vec3( center_pt.x - v1.x - v2.x - v3.x, center_pt.y  - v1.y - v2.y - v3.y, center_pt.z - v1.z - v2.z - v3.z ),
  };

  dd__box( ctx, pts );

  return DBGDRAW_ERR_OK;
}

int32_t
dd_frustum( dd_ctx_t *ctx, float* view_matrix, float* proj_matrix )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( view_matrix );
  DBGDRAW_ASSERT( proj_matrix );

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = 8;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 24;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 36;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  dd_mat4_t proj, view;
  memcpy( proj.data, proj_matrix, sizeof(dd_mat4_t) );
  memcpy( view.data, view_matrix, sizeof(dd_mat4_t) );

  dd_mat4_t inv_clip = dd_mat4_inverse( dd_mat4_mul( proj, view ) );

  dd_vec4_t pts_a[8] = 
  {
    dd_vec4( -1.0f, -1.0f, 1.0f, 1.0f ),
    dd_vec4( -1.0f,  1.0f, 1.0f, 1.0f ),
    dd_vec4(  1.0f,  1.0f, 1.0f, 1.0f ),
    dd_vec4(  1.0f, -1.0f, 1.0f, 1.0f ),

    dd_vec4( -1.0f, -1.0f, -1.0f, 1.0f ),
    dd_vec4( -1.0f,  1.0f, -1.0f, 1.0f ),
    dd_vec4(  1.0f,  1.0f, -1.0f, 1.0f ),
    dd_vec4(  1.0f, -1.0f, -1.0f, 1.0f ),
  };

  dd_vec3_t pts_b[8];
  for( int32_t i = 0; i < 8; ++i )
  {
    pts_a[i] = dd_mat4_vec4_mul(inv_clip, pts_a[i]);
    pts_b[i] = dd_vec4_to_vec3(pts_a[i]);
    pts_b[i] = dd_vec3_scalar_div(pts_b[i], pts_a[i].w);
  }

  dd__box( ctx, pts_b );

  return DBGDRAW_ERR_OK;
}

int32_t
dd_sphere( dd_ctx_t *ctx, float* c, float radius )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( c );

  dd_vec3_t center_pt = dd_vec3(c[0], c[1], c[2] );
  if( !dd__frustum_sphere_test(ctx, center_pt, radius) )
  {
    return DBGDRAW_ERR_CULLED;
  }

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t n_rings = 3;

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = n_rings * resolution;
  mode_vert_count[DBGDRAW_MODE_STROKE] = n_rings * resolution * 2;
  mode_vert_count[DBGDRAW_MODE_FILL]   = resolution * resolution * 0.5 * 6;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  dd__sphere( ctx, center_pt, radius, resolution );

  return DBGDRAW_ERR_OK;
}


int32_t
dd_cone(dd_ctx_t *ctx, float* a, float* b, float radius)
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( a );
  DBGDRAW_ASSERT( b );

  dd_vec3_t pt_a = dd_vec3(a[0], a[1], a[2] );
  dd_vec3_t pt_b = dd_vec3(b[0], b[1], b[2] );
  int32_t resolution = 1 << (ctx->detail_level + 2);

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = resolution / 2 + 1;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 3 * resolution;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 6 * resolution;
  int32_t new_verts = mode_vert_count[ ctx->cur_cmd->draw_mode ];

  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  dd__cone(ctx, pt_a, pt_b, radius, resolution);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_conical_frustum( dd_ctx_t *ctx, float* a, float* b, float radius_a, float radius_b )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( a );
  DBGDRAW_ASSERT( b );

  dd_vec3_t pt_a = dd_vec3(a[0], a[1], a[2] );
  dd_vec3_t pt_b = dd_vec3(b[0], b[1], b[2] );
  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = resolution;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 5 * resolution;
  mode_vert_count[DBGDRAW_MODE_FILL]   = 12 * resolution;
  int32_t new_verts = mode_vert_count[ ctx->cur_cmd->draw_mode ];

  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  dd__conical_frustum(ctx, pt_a, pt_b, radius_a, radius_b, resolution);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_cylinder( dd_ctx_t *ctx, float* a, float* b, float radius )
{
  return dd_conical_frustum( ctx, a, b, radius, radius );
}

int32_t
dd_arrow( dd_ctx_t *ctx, float* a, float* b, float radius, float head_radius, float head_length )
{
  dd_vec3_t pt_a = dd_vec3( a[0], a[1], a[2] );
  dd_vec3_t pt_b = dd_vec3( b[0], b[1], b[2] );
  dd_vec3_t v    = dd_vec3_sub( pt_b, pt_a );
  dd_vec3_t pt_c = dd_vec3_add( pt_a, dd_vec3_scalar_mul( v, head_length ));

  bool error = dd_cone( ctx, pt_c.data, pt_b.data, head_radius );
  if( error ) { return error; }
  return dd_conical_frustum( ctx, pt_a.data, pt_c.data, radius, radius );
}

int32_t
dd_torus(dd_ctx_t *ctx, float* center, float radius_a, float radius_b )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( center );

  dd_mat3_t axes = dd_mat3_identity();
  axes.col[0].x = radius_a * 2;
  axes.col[1].y = radius_b * 2;
  axes.col[2].z = radius_a * 2;
  dd_vec3_t center_pt = dd_vec3( center[0], center[1], center[2] );
  if( !dd__frustum_obb_test( ctx, center_pt, axes ) )
  {
    return DBGDRAW_ERR_CULLED;
  }

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t n_big_rings = 4;
  int32_t n_small_rings = resolution >> 1;
  int32_t n_rings = n_big_rings + n_small_rings;

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT]  = n_rings * resolution,
  mode_vert_count[DBGDRAW_MODE_STROKE] = n_rings * resolution * 2,
  mode_vert_count[DBGDRAW_MODE_FILL]   = n_small_rings * resolution * 2 * 3;
  int32_t new_verts = mode_vert_count[ ctx->cur_cmd->draw_mode ];

  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  switch( ctx->cur_cmd->draw_mode )
  {
    case DBGDRAW_MODE_POINT:
      dd__torus_point_stroke( ctx, center_pt, radius_a, radius_b, resolution >> 1, n_small_rings );
      break;
    case DBGDRAW_MODE_STROKE:
      dd__torus_point_stroke( ctx, center_pt, radius_a, radius_b, resolution, n_small_rings );
      break;
    default:
      dd__torus_fill( ctx, center_pt, radius_a, radius_b, resolution, n_small_rings );
      break;
  }

  return DBGDRAW_ERR_OK;
}

/// -------------------------------------------------------------------------------------------------------------------

#if DBGDRAW_HAS_STB_TRUETYPE

// TODO(maciej): Font name
int32_t
dd_init_font_from_memory( dd_ctx_t* ctx, const void* ttf_buf, 
                          const char* name, int32_t font_size, int32_t width, int32_t height, 
                          int32_t* font_idx )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( ttf_buf );
  DBGDRAW_ASSERT( name );
  DBGDRAW_ASSERT( font_idx );

  DBGDRAW_VALIDATE( ctx->fonts_len < ctx->fonts_cap, DBGDRAW_ERR_FONT_LIMIT_REACHED );
  dd_font_data_t* font = ctx->fonts + ctx->fonts_len; 
  font->bitmap_width = width;
  font->bitmap_height = height;
  font->size = font_size;
  size_t name_len = strnlen( name, 4096 );
  font->name = DBGDRAW_MALLOC( name_len );
  strncpy( font->name, name, name_len );

  uint8_t* pixel_buf = DBGDRAW_MALLOC( width * height );
  memset( pixel_buf, 0, width * height );

  stbtt_InitFont(&font->info, ttf_buf, 0);
  float to_pixel_scale = stbtt_ScaleForPixelHeight( &font->info, font->size);

  int32_t ascent, descent, line_gap;
  stbtt_GetFontVMetrics( &font->info, &ascent, &descent, &line_gap);
  font->ascent = to_pixel_scale * ascent;
  font->descent = to_pixel_scale * descent;
  font->line_gap = to_pixel_scale * line_gap;
  int32_t num_latin_chars = 127 - 32;
  int32_t num_greek_chars = 0x3C9 - 0x391;
  stbtt_pack_context spc = {0};
  stbtt_PackBegin( &spc, pixel_buf, width, height, 0, 1, NULL );
  stbtt_PackSetOversampling(&spc, 2, 2);
  stbtt_PackFontRange( &spc, (void*)ttf_buf, 0, font->size, 32, num_latin_chars, font->char_data );
  stbtt_PackFontRange( &spc, (void*)ttf_buf, 0, font->size, 0x391, num_greek_chars, font->char_data + num_latin_chars );
  stbtt_PackEnd( &spc );

  dd_backend_init_font_texture( ctx, pixel_buf, width, height, &font->tex_id );
  
  DBGDRAW_FREE( pixel_buf );

  *font_idx = ctx->fonts_len++;
  return DBGDRAW_ERR_OK;
}

/* Functions below are incorrect, as they dont correctly decode utf-8, just a subset including latin and greek */
char*
dd__decode_char_incorrect( char* str, uint32_t* cp )
{
  uint8_t ch = (uint8_t)(*str);
  if( ch < 128 ) { *cp = (uint32_t)*str++; }
  else {
    uint8_t a = (uint8_t)(*str++) & 0x1f;
    uint8_t b = (uint8_t)(*str++) & 0x3f;
    *cp = (a << 6) | b;
  }
  // 32 is first ascii char, 0x391 is the first greek char, and 95 is number of ASCII chars
  int32_t shift = (*cp < 128) ? 32 : (0x391 - 95);
  *cp -= shift;

  return str;
}

int32_t
dd__strlen_incorrect( const char* str )
{
  int32_t len = 0;
  const char* str_cpy = str;
  while(1)
  {
    if( *str_cpy=='\0' ) { break; }
    // ASCII Char
    if( (uint8_t)*str_cpy < 128 ) { str_cpy++; len++; }
    // Other char, assuming 2 bytes
    else { str_cpy+=2; len++; }
  }
  return len;
}

float
dd_get_text_width_font_space( dd_ctx_t* ctx, int32_t font_idx, char* str )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( str );
  DBGDRAW_VALIDATE( font_idx < ctx->fonts_cap, DBGDRAW_ERR_OUT_OF_BOUNDS_ACCESS );

  int32_t n_chars = dd__strlen_incorrect( str );
  char* str_scan = str;
  dd_font_data_t* font = ctx->fonts + font_idx;
  float to_pixel_scale = stbtt_ScaleForPixelHeight( &font->info, font->size );
  float width = 0;
  for( int32_t i = 0; i < n_chars; ++i )
  {
    uint32_t cp;
    str_scan = dd__decode_char_incorrect( str_scan, &cp );
    int32_t advance, lsb;
    stbtt_GetCodepointHMetrics( &font->info, cp, &advance, &lsb );
    width += to_pixel_scale * (advance);
  }
  return width;
}

#ifndef DBGDRAW_NO_STDIO
int32_t
dd_init_font_from_file( dd_ctx_t* ctx, const char* font_path,
                        int32_t font_size, int32_t width, int32_t height,
                        int32_t* font_idx )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( font_path );
  DBGDRAW_ASSERT( font_idx );
  if( font_path )
  {
    FILE* fp = fopen( font_path, "rb" );
    if( !fp ) { return DBGDRAW_ERR_FONT_FILE_NOT_FOUND; }
    else
    {
      fseek( fp, 0L, SEEK_END );
      size_t size = ftell( fp );
      rewind(fp);

      uint8_t* ttf_buffer = DBGDRAW_MALLOC( size );
      fread( ttf_buffer, 1, size, fp );
      fclose( fp );

      dd_init_font_from_memory( ctx, ttf_buffer, font_path, font_size, width, height, font_idx );
    }
  }
  return DBGDRAW_ERR_OK;
}
#endif

// NOTE(maciej): I do not expect to have more like 5 fonts, so it is just pure search over an array
int32_t
dd_find_font( dd_ctx_t* ctx, char* font_name )
{
  for( int32_t i = 0; i < ctx->fonts_len; ++i )
  {
    if( strncmp( font_name, ctx->fonts[i].name, 4096 ) == 0 ) { return i; }
  }
  return -1;
}

int32_t
dd_set_font( dd_ctx_t *ctx, int32_t font_idx )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_VALIDATE( ctx->cur_cmd == NULL, DBGDRAW_ERR_PREV_CMD_NOT_ENDED );
  ctx->active_font_idx = font_idx;
  return DBGDRAW_ERR_OK;
}

int32_t
dd_text( dd_ctx_t *ctx, float* pos, char* str, dd_text_info_t* info )
{
  DBGDRAW_ASSERT( ctx );
  DBGDRAW_ASSERT( pos );

  // NOTE(maciej): Only handles valid UTF-8 characters between cps U+0000 and U+07FF! It's awful!!
  int32_t n_chars = dd__strlen_incorrect( str );

  int32_t new_verts = 6 * n_chars;
  DBGDRAW_VALIDATE( ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD );
  DBGDRAW_VALIDATE( ctx->verts_len + new_verts < ctx->verts_cap, DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER );

  ctx->cur_cmd->font_idx = ctx->active_font_idx;
  dd_font_data_t* font = ctx->fonts + ctx->active_font_idx;

  dd_vec3_t p = dd_vec3( pos[0], pos[1], pos[2] );
  float world_size = dd__pixels_to_world_size( ctx, p, font->size );
  float scale = fabs(world_size / font->size);

  dd_mat4_t inv_view = dd_mat4_se3_inverse( ctx->view );
  dd_mat3_t m = dd_mat4_to_mat3( inv_view );
  m.col[0] = dd_vec3_invert( m.col[0] );

  float x = 0;
  float y = 0;

  ctx->cur_cmd->min_depth = dd_mat4_vec3_mul( ctx->cur_cmd->xform, p, 1 ).z;

  // NOTE(maciej): "Decoding" below" is even worse, as it relies on the fact that we only 
  // requested latin and greek chars. Real deal needs a hashtable most likely
  float min_x=1e9, max_x=-1e9;
  dd_vertex_t* start_ptr = ctx->verts_data + ctx->verts_len;

  for( int32_t i = 0; i < n_chars; ++i )
  {
    uint32_t cp;
    str = dd__decode_char_incorrect( str, &cp );

    stbtt_aligned_quad q;
    stbtt_GetPackedQuad( font->char_data, font->bitmap_width, font->bitmap_height, cp, &x, &y, &q, 0 );

    dd_vec3_t p1 = dd_vec3( -scale*q.x0, -scale*q.y0, 0.0 );
    dd_vec3_t p2 = dd_vec3( -scale*q.x1, -scale*q.y0, 0.0 );
    dd_vec3_t p3 = dd_vec3( -scale*q.x1, -scale*q.y1, 0.0 );
    dd_vec3_t p4 = dd_vec3( -scale*q.x0, -scale*q.y1, 0.0 );

    p1 = dd_vec3_add( p, dd_mat3_vec3_mul( m, p1 ) );
    p2 = dd_vec3_add( p, dd_mat3_vec3_mul( m, p2 ) );
    p3 = dd_vec3_add( p, dd_mat3_vec3_mul( m, p3 ) );
    p4 = dd_vec3_add( p, dd_mat3_vec3_mul( m, p4 ) );

    min_x = DD_MIN(p1.x, min_x);
    max_x = DD_MAX(p3.x, max_x);

    dd__vertex_text( ctx, p1, dd_vec2( q.s0, q.t0 ) );
    dd__vertex_text( ctx, p2, dd_vec2( q.s1, q.t0 ) );
    dd__vertex_text( ctx, p3, dd_vec2( q.s1, q.t1 ) );

    dd__vertex_text( ctx, p1, dd_vec2( q.s0, q.t0 ) );
    dd__vertex_text( ctx, p3, dd_vec2( q.s1, q.t1 ) );
    dd__vertex_text( ctx, p4, dd_vec2( q.s0, q.t1 ) );
  }
  dd_vertex_t* end_ptr = ctx->verts_data + ctx->verts_len;
  
  float width  = DBGDRAW_FABS( max_x - min_x );
  float height = scale*(font->ascent - font->descent);

  dd_text_valign_t vert_align = DBGDRAW_TEXT_BASELINE;
  dd_text_halign_t horz_align = DBGDRAW_TEXT_LEFT;
  if( info )
  {
    vert_align = info->vert_align;
    horz_align = info->horz_align;
    info->width = width;
    info->height = height;
  }

  float vert_offset=0.0, horz_offset=0.0;
  switch( horz_align )
  {
    case DBGDRAW_TEXT_LEFT:
      horz_offset = 0.0f;
      break;
    case DBGDRAW_TEXT_RIGHT:
      horz_offset = -width;
      break;
    case DBGDRAW_TEXT_CENTER:
    default:
      horz_offset = -width * 0.5f;
    break;
  }

  float sign = -1.0;
  switch( vert_align )
  {
    case DBGDRAW_TEXT_BASELINE:
      vert_offset = 0.0f;
      break;
    case DBGDRAW_TEXT_BOTTOM:
      vert_offset = sign * scale * font->descent;
      break;
    case DBGDRAW_TEXT_TOP:
      vert_offset = sign * scale * font->ascent;
      break;
    case DBGDRAW_TEXT_MIDDLE:
    default:
      vert_offset = sign * scale * 0.5 * (font->ascent + font->descent);
    break;
  }

  for( dd_vertex_t* it = start_ptr; it != end_ptr; ++it )
  {
    it->pos_size.x += horz_offset;
    it->pos_size.y += vert_offset;
  }

  if( info )
  {
    info->anchor = dd_vec3( min_x, p.y+scale*font->descent, p.z );
    info->anchor.x += horz_offset;
    info->anchor.y += vert_offset;
  }

  return DBGDRAW_ERR_OK;
}

#else /* DBGDRAW_HAS_STB_TRUETYPE */

#ifndef DBGDRAW_NO_STDIO
int32_t dd_init_font_from_file( dd_ctx_t* ctx, const char* font_path,
                                     int32_t font_size, int32_t width, int32_t height,
                                     int32_t* font_idx )
{
  assert(0 && "[DBGDRAW COMPILE ERROR] You're attempting to use text functionality without including stb_trutype.h" );
  return DBGDRAW_ERR_OK;
}
#endif /* DBGDRAW_NO_STDIO */

int32_t dd_init_font_from_memory( dd_ctx_t* ctx, const void* ttf_buf,
                                       const char* name, int32_t font_size, int32_t width, int32_t height, 
                                       int32_t* font_idx )
{
  assert(0 && "[DBGDRAW COMPILE ERROR] You're attempting to use text functionality without including stb_trutype.h" );
  return DBGDRAW_ERR_OK;
}
int32_t dd_find_font( dd_ctx_t* ctx, char* font_name )
{
  assert(0 && "[DBGDRAW COMPILE ERROR] You're attempting to use text functionality without including stb_trutype.h" );
  return DBGDRAW_ERR_OK;
}
int32_t dd_set_font( dd_ctx_t *ctx, int32_t font_idx )
{
  assert(0 && "[DBGDRAW COMPILE ERROR] You're attempting to use text functionality without including stb_trutype.h" );
  return DBGDRAW_ERR_OK;
}
int32_t dd_text( dd_ctx_t *ctx, float* pos, const char* str, dd_text_info_t* info )
{
  assert(0 && "[DBGDRAW COMPILE ERROR] You're attempting to use text functionality without including stb_trutype.h" );
  return DBGDRAW_ERR_OK;
}

#endif /* DBGDRAW_HAS_STB_TRUETYPE */

char* dd_error_message( int32_t error_code )
{
  switch( error_code )
  {
    case DBGDRAW_ERR_OK:
      return (char*)"[DBGDRAW ERROR] No error";
      break;
    case DBGDRAW_ERR_NO_ACTIVE_CMD:
      return (char*)"[DBGDRAW ERROR] No active command. Make sure you're calling 'dd_begin_cmd' before any calls that require active command, like the commands for adding primitives.";
      break;
    case DBGDRAW_ERR_PREV_CMD_NOT_ENDED:
      return (char*)"[DBGDRAW ERROR] Previous command was not ended. You might be using a dbgdraw call which requires termination of previous command with 'dd_cmd_end'.";
      break;
    case DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER:
      return (char*)"[DBGDRAW ERROR] Out of vertex buffer memory. When initializing 'dd_ctx', try to increase the size of the vertex buffer.";
      break;
    case DBGDRAW_ERR_OUT_OF_COMMAND_BUFFER:
      return (char*)"[DBGDRAW ERROR] Out of command buffer memory. When initializing 'dd_ctx', try to increase the size of the command buffer.";
      break;
    case DBGDRAW_ERR_CULLED:
      return (char*)"[DBGDRAW INFO] Primitive culled";
      break;
    case DBGDRAW_ERR_FONT_FILE_NOT_FOUND:
      return (char*)"[DBGDRAW ERROR] File not found";
      break;
    case DBGDRAW_ERR_FONT_LIMIT_REACHED:
      return (char*)"[DBGDRAW ERROR] Out of font texture slots.";
      break;
    case DBGDRAW_ERR_OUT_OF_BOUNDS_ACCESS:
      return (char*)"[DBGDRAW ERROR] Attmpting to access memory out of bounds.";
      break;
    default:
      return (char*)"[DBGDRAW ERROR] Unknown error";
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vector Math 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline dd_vec2_t
dd_vec2( float x, float y )
{
  return (dd_vec2_t){{ x, y }};
}

inline dd_vec3_t
dd_vec3( float x, float y, float z )
{
  return (dd_vec3_t){{ x, y, z }};
}

inline dd_vec4_t
dd_vec4( float x, float y, float z, float w )
{
  return (dd_vec4_t){{ x, y, z, w }};
}

inline dd_mat3_t
dd_mat3_identity()
{
  return (dd_mat3_t){{ 1, 0, 0, 0, 1, 0, 0, 0, 1 }};
}

inline dd_mat4_t
dd_mat4_identity()
{
  return (dd_mat4_t){{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }};
}

dd_vec3_t
dd_vec3_add( dd_vec3_t a, dd_vec3_t b )
{
  return dd_vec3( a.x + b.x, a.y + b.y, a.z + b.z );
}

dd_vec3_t
dd_vec3_sub( dd_vec3_t a, dd_vec3_t b )
{
  return dd_vec3( a.x - b.x, a.y - b.y, a.z - b.z );
}

dd_vec4_t
dd_vec4_add( dd_vec4_t a, dd_vec4_t b )
{
  return dd_vec4( a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w );
}

dd_vec4_t
dd_vec4_sub( dd_vec4_t a, dd_vec4_t b )
{
  return dd_vec4( a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w );
}

dd_vec3_t
dd_vec3_scalar_mul( dd_vec3_t a, float s )
{
  return dd_vec3( a.x * s, a.y * s, a.z * s );
}

dd_vec3_t
dd_vec3_scalar_div( dd_vec3_t a, float s )
{
  double denom = 1.0 / s;
  return dd_vec3( a.x * denom, a.y * denom, a.z * denom );
}

 float
dd_vec3_dot( dd_vec3_t a, dd_vec3_t b )
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

dd_vec3_t
dd_vec3_cross( dd_vec3_t a, dd_vec3_t b )
{
  return dd_vec3( ( a.y * b.z - a.z * b.y ), ( a.z * b.x - a.x * b.z ), ( a.x * b.y - a.y * b.x ) );
}

 dd_vec3_t
dd_vec3_invert( dd_vec3_t v )
{
  return dd_vec3( -v.x, -v.y, -v.z );
}

 dd_vec3_t
dd_vec3_normalize( dd_vec3_t v )
{
  float denom = 1.0f / sqrtf( v.x * v.x + v.y * v.y + v.z * v.z );
  return dd_vec3( v.x * denom, v.y * denom, v.z * denom );
}

 float
dd_vec3_norm( dd_vec3_t v )
{
  return (float)sqrt( v.x * v.x + v.y * v.y + v.z * v.z );
}

dd_vec4_t
dd_vec3_to_vec4( dd_vec3_t v )
{
  return dd_vec4( v.x, v.y, v.z, 0.0f );
}

float
dd_vec4_dot( dd_vec4_t a, dd_vec4_t b )
{
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

dd_vec4_t
dd_vec4_mul( dd_vec4_t a, dd_vec4_t b )
{
  return dd_vec4( ( a.x * b.x ), ( a.y * b.y ), ( a.z * b.z ), ( a.w * b.w ) );
}

dd_vec4_t
dd_vec4_scalar_mul( dd_vec4_t a, float s )
{
  return dd_vec4( a.x * s, a.y * s, a.z * s, a.w * s );
}

dd_vec3_t
dd_vec4_to_vec3( dd_vec4_t a )
{
  return dd_vec3( a.x, a.y, a.z );
}

dd_vec3_t
dd_mat3_vec3_mul( dd_mat3_t m, dd_vec3_t v )
{
  dd_vec3_t o;
  o.x = m.data[0]*v.x + m.data[3]*v.y + m.data[6]*v.z;
  o.y = m.data[1]*v.x + m.data[4]*v.y + m.data[7]*v.z;
  o.z = m.data[2]*v.x + m.data[5]*v.y + m.data[8]*v.z;
  return o;
}

float
dd_mat4_determinant( dd_mat4_t m )
{
  float C[4];
  float coeffs[6];

  /* coeffs are determinants of 2x2 matrices */
  coeffs[0] = m.data[10] * m.data[15] - m.data[14] * m.data[11];
  coeffs[1] = m.data[ 6] * m.data[11] - m.data[10] * m.data[ 7];
  coeffs[2] = m.data[ 2] * m.data[ 7] - m.data[ 6] * m.data[ 3];
  coeffs[3] = m.data[ 6] * m.data[15] - m.data[14] * m.data[ 7];
  coeffs[4] = m.data[ 2] * m.data[11] - m.data[10] * m.data[ 3];
  coeffs[5] = m.data[ 2] * m.data[15] - m.data[14] * m.data[ 3];

  /* Cofactor matrix */
  /*00*/C[0] = m.data[ 5] * coeffs[0] - m.data[ 9] * coeffs[3] + m.data[13] * coeffs[1];
  /*01*/C[1] = m.data[ 9] * coeffs[5] - m.data[ 1] * coeffs[0] - m.data[13] * coeffs[4]; /* negated */
  /*02*/C[2] = m.data[ 1] * coeffs[3] - m.data[ 5] * coeffs[5] + m.data[13] * coeffs[2];
  /*03*/C[3] = m.data[ 5] * coeffs[4] - m.data[ 9] * coeffs[2] - m.data[ 1] * coeffs[1]; /* negated */

  /* determinant */
  float det = m.data[0] * C[0] + m.data[4]  * C[1] +
              m.data[8] * C[2] + m.data[12] * C[3];
  return det;
}

dd_mat4_t
dd_mat4_mul( dd_mat4_t a, dd_mat4_t b )
{
  dd_mat4_t o;
  o.data[ 0] = b.data[ 0]*a.data[ 0] + b.data[ 1]*a.data[ 4] + b.data[ 2]*a.data[ 8] + b.data[ 3]*a.data[12];
  o.data[ 1] = b.data[ 0]*a.data[ 1] + b.data[ 1]*a.data[ 5] + b.data[ 2]*a.data[ 9] + b.data[ 3]*a.data[13];
  o.data[ 2] = b.data[ 0]*a.data[ 2] + b.data[ 1]*a.data[ 6] + b.data[ 2]*a.data[10] + b.data[ 3]*a.data[14];
  o.data[ 3] = b.data[ 0]*a.data[ 3] + b.data[ 1]*a.data[ 7] + b.data[ 2]*a.data[11] + b.data[ 3]*a.data[15];

  o.data[ 4] = b.data[ 4]*a.data[ 0] + b.data[ 5]*a.data[ 4] + b.data[ 6]*a.data[ 8] + b.data[ 7]*a.data[12];
  o.data[ 5] = b.data[ 4]*a.data[ 1] + b.data[ 5]*a.data[ 5] + b.data[ 6]*a.data[ 9] + b.data[ 7]*a.data[13];
  o.data[ 6] = b.data[ 4]*a.data[ 2] + b.data[ 5]*a.data[ 6] + b.data[ 6]*a.data[10] + b.data[ 7]*a.data[14];
  o.data[ 7] = b.data[ 4]*a.data[ 3] + b.data[ 5]*a.data[ 7] + b.data[ 6]*a.data[11] + b.data[ 7]*a.data[15];

  o.data[ 8] = b.data[ 8]*a.data[ 0] + b.data[ 9]*a.data[ 4] + b.data[10]*a.data[ 8] + b.data[11]*a.data[12];
  o.data[ 9] = b.data[ 8]*a.data[ 1] + b.data[ 9]*a.data[ 5] + b.data[10]*a.data[ 9] + b.data[11]*a.data[13];
  o.data[10] = b.data[ 8]*a.data[ 2] + b.data[ 9]*a.data[ 6] + b.data[10]*a.data[10] + b.data[11]*a.data[14];
  o.data[11] = b.data[ 8]*a.data[ 3] + b.data[ 9]*a.data[ 7] + b.data[10]*a.data[11] + b.data[11]*a.data[15];

  o.data[12] = b.data[12]*a.data[ 0] + b.data[13]*a.data[ 4] + b.data[14]*a.data[ 8] + b.data[15]*a.data[12];
  o.data[13] = b.data[12]*a.data[ 1] + b.data[13]*a.data[ 5] + b.data[14]*a.data[ 9] + b.data[15]*a.data[13];
  o.data[14] = b.data[12]*a.data[ 2] + b.data[13]*a.data[ 6] + b.data[14]*a.data[10] + b.data[15]*a.data[14];
  o.data[15] = b.data[12]*a.data[ 3] + b.data[13]*a.data[ 7] + b.data[14]*a.data[11] + b.data[15]*a.data[15];
  return o;
}

dd_mat3_t
dd_mat4_to_mat3( dd_mat4_t m )
{
  dd_mat3_t o;
  o.data[ 0] = m.data[ 0];
  o.data[ 1] = m.data[ 1];
  o.data[ 2] = m.data[ 2];
  o.data[ 3] = m.data[ 4];
  o.data[ 4] = m.data[ 5];
  o.data[ 5] = m.data[ 6];
  o.data[ 6] = m.data[ 8];
  o.data[ 7] = m.data[ 9];
  o.data[ 8] = m.data[10];
  return o;
}

dd_vec3_t
dd_mat4_vec3_mul ( dd_mat4_t m, dd_vec3_t v, int32_t is_point )
{
  dd_vec3_t o;
  o.x = m.data[0]*v.x + m.data[4]*v.y + m.data[ 8]*v.z + (float)is_point * m.data[12];
  o.y = m.data[1]*v.x + m.data[5]*v.y + m.data[ 9]*v.z + (float)is_point * m.data[13];
  o.z = m.data[2]*v.x + m.data[6]*v.y + m.data[10]*v.z + (float)is_point * m.data[14];
  return o;
}


dd_vec4_t
dd_mat4_vec4_mul ( dd_mat4_t m, dd_vec4_t v )
{
  dd_vec4_t o;
  o.x = m.data[0]*v.x + m.data[4]*v.y + m.data[ 8]*v.z + m.data[12]*v.w;
  o.y = m.data[1]*v.x + m.data[5]*v.y + m.data[ 9]*v.z + m.data[13]*v.w;
  o.z = m.data[2]*v.x + m.data[6]*v.y + m.data[10]*v.z + m.data[14]*v.w;
  o.w = m.data[3]*v.x + m.data[7]*v.y + m.data[11]*v.z + m.data[15]*v.w;
  return o;
}

dd_mat4_t
dd_mat4_transpose( dd_mat4_t m )
{
  dd_mat4_t mt;
  mt.data[ 0] = m.data[ 0];
  mt.data[ 1] = m.data[ 4];
  mt.data[ 2] = m.data[ 8];
  mt.data[ 3] = m.data[12];
  mt.data[ 4] = m.data[ 1];
  mt.data[ 5] = m.data[ 5];
  mt.data[ 6] = m.data[ 9];
  mt.data[ 7] = m.data[13];
  mt.data[ 8] = m.data[ 2];
  mt.data[ 9] = m.data[ 6];
  mt.data[10] = m.data[10];
  mt.data[11] = m.data[14];
  mt.data[12] = m.data[ 3];
  mt.data[13] = m.data[ 7];
  mt.data[14] = m.data[11];
  mt.data[15] = m.data[15];
  return mt;
}

dd_mat4_t
dd_mat4_se3_inverse( dd_mat4_t m )
{
  dd_vec3_t t = dd_vec4_to_vec3( m.col[3] );
  m.col[3] = dd_vec4( 0.0f, 0.0f, 0.0f, 1.0f );
  m = dd_mat4_transpose( m );
  t = dd_vec3_invert( dd_mat4_vec3_mul( m, t, 0 ) );
  m.col[3] = dd_vec3_to_vec4( t );
  m.col[3].w = 1.0f;
  return m;
}

dd_mat4_t
dd_mat4_inverse( dd_mat4_t m )
{
  /* Calulate cofactor matrix */
  float C[16];
  float dets[6];

  /* dets are determinants of 2x2 matrices */
  dets[0] = m.data[10] * m.data[15] - m.data[14] * m.data[11];
  dets[1] = m.data[ 6] * m.data[11] - m.data[10] * m.data[ 7];
  dets[2] = m.data[ 2] * m.data[ 7] - m.data[ 6] * m.data[ 3];
  dets[3] = m.data[ 6] * m.data[15] - m.data[14] * m.data[ 7];
  dets[4] = m.data[ 2] * m.data[11] - m.data[10] * m.data[ 3];
  dets[5] = m.data[ 2] * m.data[15] - m.data[14] * m.data[ 3];

  /* Cofactor matrix */
  /*00*/C[0] = m.data[5]*dets[0] - m.data[9]*dets[3] + m.data[13]*dets[1];
  /*01*/C[1] = m.data[9]*dets[5] - m.data[1]*dets[0] - m.data[13]*dets[4]; /* negated */
  /*02*/C[2] = m.data[1]*dets[3] - m.data[5]*dets[5] + m.data[13]*dets[2];
  /*03*/C[3] = m.data[5]*dets[4] - m.data[9]*dets[2] - m.data[ 1]*dets[1]; /* negated */

  /*10*/C[4] = m.data[8]*dets[3] - m.data[4]*dets[0] - m.data[12]*dets[1]; /* negated */
  /*11*/C[5] = m.data[0]*dets[0] - m.data[8]*dets[5] + m.data[12]*dets[4];
  /*12*/C[6] = m.data[4]*dets[5] - m.data[0]*dets[3] - m.data[12]*dets[2]; /* negated */
  /*13*/C[7] = m.data[0]*dets[1] - m.data[4]*dets[4] + m.data[ 8]*dets[2];

  /* dets are determinants of 2x2 matrices */
  dets[0] = m.data[ 8]*m.data[13] - m.data[12]*m.data[ 9];
  dets[1] = m.data[ 4]*m.data[ 9] - m.data[ 8]*m.data[ 5];
  dets[2] = m.data[ 0]*m.data[ 5] - m.data[ 4]*m.data[ 1];
  dets[3] = m.data[ 4]*m.data[13] - m.data[12]*m.data[ 5];
  dets[4] = m.data[ 0]*m.data[ 9] - m.data[ 8]*m.data[ 1];
  dets[5] = m.data[ 0]*m.data[13] - m.data[12]*m.data[ 1];

  /* actual coefficient matrix */
  /*20*/C[ 8] = m.data[ 7]*dets[0] - m.data[11]*dets[3] + m.data[15]*dets[1];
  /*21*/C[ 9] = m.data[11]*dets[5] - m.data[ 3]*dets[0] - m.data[15]*dets[4]; /* negated */
  /*22*/C[10] = m.data[ 3]*dets[3] - m.data[ 7]*dets[5] + m.data[15]*dets[2];
  /*23*/C[11] = m.data[ 7]*dets[4] - m.data[ 3]*dets[1] - m.data[11]*dets[2]; /* negated */

  /*30*/C[12] = m.data[10]*dets[3] - m.data[ 6]*dets[0] - m.data[14]*dets[1]; /* negated */
  /*31*/C[13] = m.data[ 2]*dets[0] - m.data[10]*dets[5] + m.data[14]*dets[4];
  /*32*/C[14] = m.data[ 6]*dets[5] - m.data[ 2]*dets[3] - m.data[14]*dets[2]; /* negated */
  /*33*/C[15] = m.data[ 2]*dets[1] - m.data[ 6]*dets[4] + m.data[10]*dets[2];

  /* determinant */
  float det = m.data[0]*C[0] + m.data[4]*C[1] +
              m.data[8]*C[2] + m.data[12]*C[3];
  float denom = 1.0f / det;

  /* calculate inverse */
  dd_mat4_t mi;
  mi.data[ 0] = denom*C[ 0];
  mi.data[ 1] = denom*C[ 1];
  mi.data[ 2] = denom*C[ 2];
  mi.data[ 3] = denom*C[ 3];
  mi.data[ 4] = denom*C[ 4];
  mi.data[ 5] = denom*C[ 5];
  mi.data[ 6] = denom*C[ 6];
  mi.data[ 7] = denom*C[ 7];
  mi.data[ 8] = denom*C[ 8];
  mi.data[ 9] = denom*C[ 9];
  mi.data[10] = denom*C[10];
  mi.data[11] = denom*C[11];
  mi.data[12] = denom*C[12];
  mi.data[13] = denom*C[13];
  mi.data[14] = denom*C[14];
  mi.data[15] = denom*C[15];
  return mi;
}


dd_mat4_t
dd_pre_scale( dd_mat4_t m, dd_vec3_t s )
{
  dd_vec4_t s4 = dd_vec4( s.x, s.y, s.z, 1.0 );
  m.col[0] = dd_vec4_mul( m.col[0], s4 );
  m.col[1] = dd_vec4_mul( m.col[1], s4 );
  m.col[2] = dd_vec4_mul( m.col[2], s4 );
  m.col[3] = dd_vec4_mul( m.col[3], s4 );
  return m;
}

dd_mat4_t
dd_pre_translate( dd_mat4_t m, dd_vec3_t t )
{
  m.col[3] = dd_vec4( m.col[3].x + t.x, m.col[3].y + t.y,m.col[3].z + t.z, 1.0f );
  return m;
}

dd_mat4_t
dd_pre_rotate( dd_mat4_t m, float angle, dd_vec3_t v )
{
  float c = cosf( angle );
  float s = sinf( angle );
  float t = 1.0f - c;

  dd_vec3_t axis = dd_vec3_normalize( v );

  dd_mat4_t rotate = dd_mat4_identity();
  rotate.data[ 0] = c + axis.x * axis.x * t;
  rotate.data[ 5] = c + axis.y * axis.y * t;
  rotate.data[10] = c + axis.z * axis.z * t;

  float tmp_1 = axis.x * axis.y * t;
  float tmp_2 = axis.z * s;

  rotate.data[1] = tmp_1 + tmp_2;
  rotate.data[4] = tmp_1 - tmp_2;

  tmp_1 = axis.x * axis.z * t;
  tmp_2 = axis.y * s;

  rotate.data[2] = tmp_1 - tmp_2;
  rotate.data[8] = tmp_1 + tmp_2;

  tmp_1 = axis.y * axis.z * t;
  tmp_2 = axis.x * s;

  rotate.data[6] = tmp_1 + tmp_2;
  rotate.data[9] = tmp_1 - tmp_2;

  return dd_mat4_mul( rotate, m );
}

#endif /* DBGDRAW_IMPLEMENTATION */