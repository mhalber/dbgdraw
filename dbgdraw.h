

/*
  OVERVIEW
  =================
  dbgdraw - C99 immediate mode drawing library, by Maciej Halber, 2020. See the end of file for licensing info

  dbgdraw is an attempt at creating a drawing library that allows user to easily start drawing
  shapes like bounding boxes, spheres, and other common shapes that are useful when visualizing 3D data.
  The idea is that these shapes are driven by some algorithm that updates 3d data (think dynamic aabb trees), and 
  as such different shapes need to be drawn at each frame.
  This library also includes rudimentaty, not very memory efficient 2D support. This also includes
  optional support for text rendering, if user also includes stb_truetype.

  USAGE
  =================
  dbgdraw only provides facilities to perform drawing operations. As such any windowing support needs to be
  done separately. The use of the library is centered around context structure `dd_ctx_t` which stores
  required information to render the data. There are three steps that dbgdraw expects user to do.
  
  1) At initialization time, user should populate `dd_ctx_desc_t` structure with relevant options and call
  `dd_init` function.

  2) At a frame time, user should populate 'dd_new_frame_info_t' structure with relevant information (viewport size,
  transformation matrices).

  3) After all commands are issued, user is expected to signal this to dbgdraw by calling `dd_render` function.

  After this setup user can begin drawing shapes by simply issueing draw commands in between, dd_begin_cmd and dd_end_cmd
  calls. For example, to draw green sphere and blue aabb:
  
  ```
  dd_begin_cmd(ctx_ptr, DBGDRAW_MODE_FILL);
  dd_set_color(ctx_ptr, DBGDRAW_GREEN)l
  dd_sphere(ctx_ptr, &shpere_center, radius);
  dd_set_color(ctx_ptr, DBGDRAW_BLUE);
  dd_aabb(ctx_ptr, &min_aabb_pt, &max_aabb_pt);
  dd_end_cmd(ctx_ptr);
  ```

  A good place to start to see the API usage is the basic.c example in the github repository:
   [ link ]

  GRAPHICS API
  ==============
  dbgdraw is backend agnostic. Backend drawing functionality is initialized by providing definitions to following functions
   - `dd_backend_init`
   - `dd_backend_render`
   - `dd_backend_term`
   - (optional)`dd_backend_init_texture`

  CONFIGURATION
  ===============
  dbgdraw can be configured to match your needs. Below we 
  memory -mallocs
  string - memset memcpy strncmp, strncpy strnlen
  asserts
  math
  sin
  handle out of memory

  GOOD PRACTICES
  ================
  To limit the amount of draw call issues, if possible try to group the draw calls that operate in the same mode.
  Specifically, instead of 
  ```
  ```
  do this:
  ````
  ````

  FEATURES
  ================
  - OpenGL 4.5 backend
  - Optional validation layers that will check if state of dd_ctx_t is properly modified.
  - POD function signatures - drawing functions expect just basic data passed as float*. You can use glm, Eigen, HMM or whatever
  else libary to do client side computation and simply pass the pointer to the data to dbgdraw 
  (as long as you're using floats, which most likely you are.)
  - Default font embedded in this file

  CREDITS
  =================
  rxi - cembed used to produce the font embedding
  Micha Metke - Deflate for default font
  Bjoern Hoehrmann - UTF-8 decoder

  EXAMPLE PROGRAM
  =================
  TODO
*/

#ifndef DBGDRAW_H
#define DBGDRAW_H

#ifdef __cplusplus
extern "C"
{
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
#define DBGDRAW_HAS_TEXT_SUPPORT 1
#else
#define DBGDRAW_HAS_TEXT_SUPPORT 0
#endif

#if defined(DBGDRAW_MALLOC) && defined(DBGDRAW_FREE)
  // all good
#elif !defined(DBGDRAW_MALLOC) && !defined(DBGDRAW_FREE)
  // all good
#else
#error "If you redefined any of 'malloc', 'realloc' or 'free' functions, please redefine all of them"
#endif

#ifndef DBGDRAW_MALLOC
#include <stdlib.h>
#define DBGDRAW_MALLOC(size) malloc(size)
#define DBGDRAW_REALLOC(ptr, size) realloc(ptr, size)
#define DBGDRAW_FREE(ptr) free(ptr)
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
#define DBGDRAW_MEMSET(dest, ch, len) memset(dest, ch, len)
#define DBGDRAW_MEMCPY(dest, src, len) memcpy(dest, src, len)
#define DBGDRAW_STRNCMP(str1, str2, len) strncmp(str1, str2, len)
#define DBGDRAW_STRNCPY(dest, src, len) strncpy(dest, src, len)
#define DBGDRAW_STRNLEN(str, len) strnlen(str, len)
#endif

#ifndef DBGDRAW_ASSERT
#include <assert.h>
#define DBGDRAW_ASSERT(cond) assert(cond)
#endif

#define DBGDRAW_PI 3.14159265358979323846264338327950288419716939937510582097494459231
#define DBGDRAW_TWO_PI 6.28318530717958647692528676655900576839433879875021164194988918462
#define DBGDRAW_PI_OVER_TWO 1.57079632679489661923132169163975144209858469968755291048747229615

#if defined(DBGDRAW_SIN) && defined(DBGDRAW_COS) && defined(DBGDRAW_FABS) && defined(DBGDRAW_ROUND)
  // all good
#elif !defined(DBGDRAW_SIN) && !defined(DBGDRAW_COS) && !defined(DBGDRAW_FABS) && !defined(DBGDRAW_ROUND)
  // all good
#else
  #error "If you redefined any of 'sin', 'cos', 'fabs' or 'round' functions, please redefine all of them"
#endif

#ifndef DBGDRAW_SIN
#include <math.h>

#if DBGDRAW_USE_TRANSCENDENTAL_LUT
#define DBGDRAW_SIN(x) (x >= 0 ? 1 : -1) * ctx->sinf_lut[(int32_t)(fabsf(x) * ctx->lut_gamma)]
#else
#define DBGDRAW_SIN(x) sinf(x)
#endif

#if DBGDRAW_USE_TRANSCENDENTAL_LUT
#define DBGDRAW_COS(x) ctx->cosf_lut[(int32_t)(fabsf(x) * ctx->lut_gamma)]
#else
#define DBGDRAW_COS(x) cosf(x)
#endif

#define DBGDRAW_FABS(x) fabsf(x)
#define DBGDRAW_ROUND(x) roundf(x)
#endif

#define DD_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define DD_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define DD_ABS(x) (((x) < 0) ? -(x) : (x))

#ifndef DBGDRAW_NO_STDIO
#include <stdio.h>
#endif

#ifndef DBGDRAW_LOG
#ifdef DBGDRAW_NO_STDIO
#define DBGDRAW_LOG(format, msg, func)
#else
#define DBGDRAW_LOG(format, msg, func)  \
  {                                     \
    fprintf(stdout, format, msg, func); \
  }
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

  DBGDRAW_MODE_COUNT
} dd_mode_t;

typedef enum dd_shading_type
{
  DBGDRAW_SHADING_NONE,
  DBGDRAW_SHADING_SOLID,

  DBGDRAW_SHADING_COUNT
} dd_shading_t;

typedef enum dd_fill_t
{
  DBGDRAW_FILL_FLAT,
  DBGDRAW_FILL_LINEAR_GRADIENT,

  DBGDRAW_FILL_COUNT
} dd_fill_t;

typedef enum dd_projection_type
{
  DBGDRAW_PERSPECTIVE,
  DBGDRAW_ORTHOGRAPHIC
} dd_proj_type_t;

typedef struct dd_context_desc dd_ctx_desc_t;
typedef struct dd_new_frame_info dd_new_frame_info_t;
typedef struct dd_ctx_t dd_ctx_t;
typedef struct dd_instance_data dd_instance_data_t;
#if DBGDRAW_HAS_TEXT_SUPPORT
typedef struct dd_text_info dd_text_info_t;
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Initialize / Terminate - call on init and shut down
int32_t dd_init(dd_ctx_t *ctx, dd_ctx_desc_t *descs);
int32_t dd_term(dd_ctx_t *ctx);

// Start new frame (update all necessary data as listed in info pointer) / Render - call at the start and end of a frame
int32_t dd_new_frame(dd_ctx_t *ctx, dd_new_frame_info_t *info);
int32_t dd_render(dd_ctx_t *ctx);

// Command start and end + modify global state
int32_t dd_begin_cmd(dd_ctx_t *ctx, dd_mode_t draw_mode);
int32_t dd_end_cmd(dd_ctx_t *ctx);

int32_t dd_set_transform(dd_ctx_t *ctx, float *xform);
int32_t dd_set_shading_type(dd_ctx_t *ctx, dd_shading_t shading_type);
int32_t dd_set_fill_type(dd_ctx_t *ctx, dd_fill_t fill_type);
int32_t dd_set_color(dd_ctx_t *ctx, dd_color_t color);
int32_t dd_set_gradient_a(dd_ctx_t *ctx, dd_color_t color, float *pos);
int32_t dd_set_gradient_b(dd_ctx_t *ctx, dd_color_t color, float *pos);
int32_t dd_set_detail_level(dd_ctx_t *ctx, uint8_t level);
int32_t dd_set_primitive_size(dd_ctx_t *ctx, float primitive_size);

// 3d drawing API
int32_t dd_point(dd_ctx_t *ctx, float *pt_a);
int32_t dd_line(dd_ctx_t *ctx, float *pt_a, float *pt_b);
int32_t dd_quad(dd_ctx_t *ctx, float *pt_a, float *pt_b, float *pt_c, float *pt_d);
int32_t dd_rect(dd_ctx_t *ctx, float *pt_a, float *pt_b);
int32_t dd_circle(dd_ctx_t *ctx, float *center_pt, float radius);
int32_t dd_arc(dd_ctx_t *ctx, float *center_pt, float radius, float angle);
int32_t dd_aabb(dd_ctx_t *ctx, float *min_pt, float *max_pt);
int32_t dd_obb(dd_ctx_t *ctx, float *center_pt, float *axes_matrix);
int32_t dd_frustum(dd_ctx_t *ctx, float *view_matrix, float *proj_matrix);
int32_t dd_sphere(dd_ctx_t *ctx, float *center_pt, float radius);
int32_t dd_torus(dd_ctx_t *ctx, float *center_pt, float radius_a, float radius_b);
int32_t dd_cone(dd_ctx_t *ctx, float *a, float *b, float radius);
int32_t dd_cylinder(dd_ctx_t *ctx, float *a, float *b, float radius);
int32_t dd_conical_frustum(dd_ctx_t *ctx, float *a, float *b, float radius_a, float radius_b);
int32_t dd_arrow(dd_ctx_t *ctx, float *a, float *b, float radius, float head_radius, float head_length);
int32_t dd_billboard_rect(dd_ctx_t *ctx, float *p, float width, float height);
int32_t dd_billboard_circle(dd_ctx_t *ctx, float *c, float radius);

// 2d drawing API
int32_t dd_point2d(dd_ctx_t *ctx, float *pt_a);
int32_t dd_line2d(dd_ctx_t *ctx, float *pt_a, float *pt_b);
int32_t dd_quad2d(dd_ctx_t *ctx, float *pt_a, float *pt_b, float *pt_c, float *pt_d);
int32_t dd_rect2d(dd_ctx_t *ctx, float *pt_a, float *pt_b);
int32_t dd_circle2d(dd_ctx_t *ctx, float *center, float radius);
int32_t dd_arc2d(dd_ctx_t *ctx, float *center, float radius, float angle);
int32_t dd_rounded_rect2d(dd_ctx_t *ctx, float *a, float *b, float rounding);
int32_t dd_rounded_rect2d_ex(dd_ctx_t *ctx, float *a, float *b, float *rounding);

// Text rendering
#if DBGDRAW_HAS_TEXT_SUPPORT
int32_t dd_find_font(dd_ctx_t *ctx, char *font_name);
int32_t dd_set_font(dd_ctx_t *ctx, int32_t font_idx);
int32_t dd_text_line(dd_ctx_t *ctx, float *pos, const char *str, dd_text_info_t *info);
void dd_get_text_size_font_space(dd_ctx_t *ctx, int32_t font_idx, const char *str, int32_t strlen,
                                  float *width, float *height);
int32_t dd_init_font_from_memory(dd_ctx_t *ctx, const void *ttf_buf,
                                  const char *name, int32_t font_size, int32_t width, int32_t height,
                                  int32_t *font_idx);
#ifndef DBGDRAW_NO_STDIO
int32_t dd_init_font_from_file(dd_ctx_t *ctx, const char *font_path, 
                               const char* name, int32_t font_size, int32_t width, int32_t height,
                               int32_t *font_idx);
#endif
#endif

// Utility
dd_color_t  dd_hex2color(uint32_t hex_code);
dd_color_t  dd_rgbf(float r, float g, float b);
dd_color_t  dd_rgbaf(float r, float g, float b, float a);
dd_color_t  dd_rgbu(uint8_t r, uint8_t g, uint8_t b);
dd_color_t  dd_rgbau(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
dd_color_t  dd_hsl(float hue_degres, float saturation, float lightness);
dd_color_t  dd_interpolate_color(dd_color_t c0, dd_color_t c1, float t);
void        dd_extract_frustum_planes(dd_ctx_t *ctx);
const char* dd_error_message(int32_t error_code);
int32_t     dd_set_instance_data(dd_ctx_t* ctx, int32_t instance_count, dd_instance_data_t* data);


// User provides implementation for these - see examples for reference implementation
int32_t dd_backend_init(dd_ctx_t *ctx);
int32_t dd_backend_render(dd_ctx_t *ctx);
int32_t dd_backend_term(dd_ctx_t *ctx);
#if DBGDRAW_HAS_TEXT_SUPPORT
int32_t dd_backend_init_font_texture(dd_ctx_t *ctx,
                                      const uint8_t *data, int32_t width, int32_t height, uint32_t *tex_id);
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Build-in colors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DBGDRAW_RED (dd_color_t) { 218, 40, 42, 255 }
#define DBGDRAW_GREEN (dd_color_t) { 38, 172, 38, 255 }
#define DBGDRAW_BLUE (dd_color_t) { 18, 72, 223, 255 }
#define DBGDRAW_CYAN (dd_color_t) { 21, 194, 195, 255 }
#define DBGDRAW_MAGENTA (dd_color_t) { 211, 68, 168, 255 }
#define DBGDRAW_YELLOW (dd_color_t) { 245, 245, 38, 255 }
#define DBGDRAW_ORANGE (dd_color_t) { 243, 146, 26, 255 }
#define DBGDRAW_PURPLE (dd_color_t) { 129, 26, 243, 255 }
#define DBGDRAW_LIME (dd_color_t) { 142, 243, 26, 255 }
#define DBGDRAW_BROWN (dd_color_t) { 138, 89, 47, 255 }

#define DBGDRAW_LIGHT_RED (dd_color_t) { 255, 108, 110, 255 }
#define DBGDRAW_LIGHT_GREEN (dd_color_t) { 117, 234, 117, 255 }
#define DBGDRAW_LIGHT_BLUE (dd_color_t) { 104, 140, 243, 255 }
#define DBGDRAW_LIGHT_CYAN (dd_color_t) { 129, 254, 255, 255 }
#define DBGDRAW_LIGHT_MAGENTA (dd_color_t) { 255, 144, 217, 255 }
#define DBGDRAW_LIGHT_YELLOW (dd_color_t) { 255, 255, 127, 255 }
#define DBGDRAW_LIGHT_ORANGE (dd_color_t) { 255, 197, 119, 255 }
#define DBGDRAW_LIGHT_PURPLE (dd_color_t) { 200, 152, 255, 255 }
#define DBGDRAW_LIGHT_LIME (dd_color_t) { 205, 255, 148, 255 }
#define DBGDRAW_LIGHT_BROWN (dd_color_t) { 209, 155, 100, 255 }

#define DBGDRAW_BLACK (dd_color_t) { 0, 0, 0, 255 }
#define DBGDRAW_WHITE (dd_color_t) { 255, 255, 255, 255 }
#define DBGDRAW_GRAY (dd_color_t) { 162, 162, 162, 255 }
#define DBGDRAW_LIGHT_GRAY (dd_color_t) { 200, 200, 200, 255 }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vector Math
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef union dd_vec2 {
  float data[2];
  struct
  {
    float x;
    float y;
  };
} dd_vec2_t;

typedef union dd_vec3 {
  float data[3];
  struct
  {
    float x;
    float y;
    float z;
  };
} dd_vec3_t;

typedef union dd_vec4 {
  float data[4];
  struct
  {
    float x;
    float y;
    float z;
    float w;
  };
} dd_vec4_t;

typedef union dd_mat3 {
  float data[9];
  dd_vec3_t col[3];
} dd_mat3_t;

typedef union dd_mat4 {
  float data[16];
  dd_vec4_t col[4];
} dd_mat4_t;

dd_vec2_t dd_vec2(float x, float y);
dd_vec3_t dd_vec3(float x, float y, float z);
dd_vec4_t dd_vec4(float x, float y, float z, float w);

dd_mat3_t dd_mat3_identity();
dd_mat4_t dd_mat4_identity();

dd_vec2_t dd_vec2_add(dd_vec2_t a, dd_vec2_t b);
dd_vec2_t dd_vec2_sub(dd_vec2_t a, dd_vec2_t b);
dd_vec3_t dd_vec3_add(dd_vec3_t a, dd_vec3_t b);
dd_vec3_t dd_vec3_sub(dd_vec3_t a, dd_vec3_t b);
dd_vec3_t dd_vec3_scalar_mul(dd_vec3_t a, float s);
dd_vec3_t dd_vec3_scalar_div(dd_vec3_t a, float s);
float dd_vec2_dot(dd_vec2_t a, dd_vec2_t b);
float dd_vec3_dot(dd_vec3_t a, dd_vec3_t b);
dd_vec3_t dd_vec3_cross(dd_vec3_t a, dd_vec3_t b);
dd_vec3_t dd_vec3_invert(dd_vec3_t v);
float dd_vec2_norm(dd_vec2_t v);
float dd_vec2_norm_sq(dd_vec2_t v);
float dd_vec3_norm(dd_vec3_t v);
float dd_vec3_norm_sq(dd_vec3_t v);
dd_vec3_t dd_vec3_normalize(dd_vec3_t v);
dd_vec2_t dd_vec2_normalize(dd_vec2_t v);
dd_vec4_t dd_vec3_to_vec4(dd_vec3_t v);
dd_vec3_t dd_mat3_vec3_mul(dd_mat3_t m, dd_vec3_t v);

dd_vec4_t dd_vec4_add(dd_vec4_t a, dd_vec4_t b);
dd_vec4_t dd_vec4_sub(dd_vec4_t a, dd_vec4_t b);
float dd_vec4_dot(dd_vec4_t a, dd_vec4_t b);
dd_vec4_t dd_vec4_mul(dd_vec4_t a, dd_vec4_t b);
dd_vec4_t dd_vec4_scalar_mul(dd_vec4_t a, float s);
dd_vec3_t dd_vec4_to_vec3(dd_vec4_t a);

dd_mat4_t dd_mat4_mul(dd_mat4_t a, dd_mat4_t b);
dd_vec3_t dd_mat4_vec3_mul(dd_mat4_t m, dd_vec3_t v, int32_t is_point);
dd_vec4_t dd_mat4_vec4_mul(dd_mat4_t m, dd_vec4_t v);
dd_mat3_t dd_mat4_to_mat3(dd_mat4_t m);
float dd_mat4_determinant(dd_mat4_t m);
dd_mat4_t dd_mat4_se3_inverse(dd_mat4_t m);
dd_mat4_t dd_mat4_inverse(dd_mat4_t m);
dd_mat4_t dd_mat4_transpose(dd_mat4_t m);

dd_mat4_t dd_pre_scale(dd_mat4_t m, dd_vec3_t s);
dd_mat4_t dd_pre_translate(dd_mat4_t m, dd_vec3_t t);
dd_mat4_t dd_pre_rotate(dd_mat4_t m, float angle, dd_vec3_t v);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DebugDraw Structs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if DBGDRAW_HAS_TEXT_SUPPORT

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

typedef struct dd_text_rect
{
  float x, y;
  float w, h;
} dd_text_rect_t;

typedef struct dd_text_info
{
  dd_text_halign_t horz_align;
  dd_text_valign_t vert_align;

  dd_vec3_t anchor;
  float width;
  float height;

  dd_text_rect_t clip_rect;
} dd_text_info_t;

typedef struct dd_font_data
{
  char *name;
  int32_t bitmap_width, bitmap_height;
  float ascent, descent, line_gap;

  stbtt_packedchar char_data[1024];
  stbtt_fontinfo info;
  uint32_t size;
  uint32_t tex_id;
} dd_font_data_t;

#endif

typedef enum dd_error
{
  DBGDRAW_ERR_OK = 0,
  DBGDRAW_ERR_FAILED_ALLOC,
  DBGDRAW_ERR_NO_ACTIVE_CMD,
  DBGDRAW_ERR_PREV_CMD_NOT_ENDED,
  DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER,
  DBGDRAW_ERR_OUT_OF_COMMAND_BUFFER,
  DBGDRAW_ERR_OUT_OF_MEMORY,
  DBGDRAW_ERR_CULLED,
  DBGDRAW_ERR_FONT_FILE_NOT_FOUND,
  DBGDRAW_ERR_FONT_LIMIT_REACHED,
  DBGDRAW_ERR_OUT_OF_BOUNDS_ACCESS,
  DBGDRAW_ERR_INVALID_FONT_REQUESTED,
  DBGDRAW_ERR_INVALID_MODE,
  DBGDRAW_ERR_USING_TEXT_WITHOUT_FONT,

  DBGDRAW_ERR_COUNT
} dd_err_code_t;

typedef struct dd_context_desc
{
  int32_t max_vertices;
  int32_t max_commands;
  int32_t max_instances;
  int32_t max_fonts;
  uint8_t detail_level;
  float line_antialias_radius;
  uint8_t enable_frustum_cull;
  uint8_t enable_depth_test;
#if DBGDRAW_HAS_TEXT_SUPPORT && defined(DBGDRAW_USE_DEFAULT_FONT)
  uint8_t enable_default_font;
#endif
} dd_ctx_desc_t;

typedef struct dd_new_frame_info
{
  float *view_matrix;
  float *projection_matrix;
  float *viewport_size;
  float vertical_fov;
  uint8_t projection_type;
} dd_new_frame_info_t;

typedef struct dd_instance_data
{
  dd_vec3_t position;
  dd_color_t color;
} dd_instance_data_t;

typedef struct dd_vertex
{
  union {
    struct
    {
      dd_vec3_t pos;
      float size;
    };
    dd_vec4_t pos_size;
  };
  union {
    struct
    {
      dd_vec2_t uv;
      float dummy;
    };
    dd_vec3_t normal;
  };
  dd_color_t col;
} dd_vertex_t;

typedef struct dd_cmd_t
{
  int32_t base_index;
  int32_t vertex_count;
  int32_t instance_count;

  dd_instance_data_t* instance_data;

  dd_mat4_t xform;
  float min_depth;

  dd_mode_t draw_mode;
  dd_shading_t shading_type;
#if DBGDRAW_HAS_TEXT_SUPPORT
  int32_t font_idx;
#endif
} dd_cmd_t;

typedef struct dd_ctx_t
{
  /* User accessible state */
  dd_color_t color;
  dd_color_t gradient_a_col;
  dd_color_t gradient_b_col;
  dd_vec3_t gradient_a_pt;
  dd_vec3_t gradient_b_pt;
  dd_mat4_t xform;
  uint8_t detail_level;
  uint8_t frustum_cull;
  dd_shading_t shading_type;
  dd_fill_t fill_type;
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

#if DBGDRAW_HAS_TEXT_SUPPORT
  /* Text info */
  dd_font_data_t *fonts;
  int32_t fonts_len;
  int32_t fonts_cap;
  int32_t active_font_idx;
#ifdef DBGDRAW_USE_DEFAULT_FONT
  int32_t default_font_idx;
#endif
#endif

  /* Render backend */
  void *render_backend;
  int32_t drawcall_count;
  dd_vec2_t aa_radius;
  uint8_t enable_depth_test;

  /* Extras */
  int32_t instance_cap;
  float *sinf_lut;
  float *cosf_lut;
  float lut_gamma;
  uint32_t lut_size;
  
} dd_ctx_t;

#ifdef __cplusplus
}
#endif

#ifdef DBGDRAW_VALIDATION_LAYERS

#define DBGDRAW_VALIDATE(cond, err_code)                                            \
  do                                                                                \
  {                                                                                 \
    if (!(cond))                                                                    \
    {                                                                               \
      DBGDRAW_LOG("vvvvvvvvvvvvvvvvvvvv  VALIDATION ERROR vvvvvvvvvvvvvvvv \n %s\n" \
                  "     Occured while calling function: %s\n",                      \
                  dd_error_message(err_code), __func__);                            \
      DBGDRAW_ASSERT(false);                                                        \
    }                                                                               \
  } while (0)
#else
#define DBGDRAW_VALIDATE(cond, err_code)
#endif

#ifndef DBGDRAW_HANDLE_OUT_OF_MEMORY
#define DBGDRAW_HANDLE_OUT_OF_MEMORY(ptr, len, cap, elemsize)     \
  do                                                              \
  {                                                               \
    if (len >= cap)                                               \
    {                                                             \
      size_t new_cap = DD_MAX(2 * cap, len);                      \
      void *new_ptr = DBGDRAW_REALLOC(ptr, new_cap * (elemsize)); \
      cap = (int32_t)new_cap;                                     \
      ptr = new_ptr;                                              \
    }                                                             \
  } while (0)
#endif

#endif /* DBGDRAW_H */

#ifdef DBGDRAW_IMPLEMENTATION

#if DBGDRAW_HAS_TEXT_SUPPORT && defined(DBGDRAW_USE_DEFAULT_FONT)
int32_t dbgdraw__inflate(unsigned char *out, const unsigned char *in, int size);
static unsigned char dd_proggy_square[7976];
static struct { char *filename; unsigned char *data; int compressed_size; int size; } dd_default_font_info =
{ "ProggySquare.ttf", dd_proggy_square, 7976, 41588 };
#endif

int32_t
dd_init(dd_ctx_t *ctx, dd_ctx_desc_t *desc)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(desc);

#if DBGDRAW_USE_TRANSCENDENTAL_LUT
  ctx->lut_size = 2048;
  ctx->sinf_lut = DBGDRAW_MALLOC(2 * ((ctx->lut_size + 1) * sizeof(float)));
  ctx->cosf_lut = ctx->sinf_lut + (ctx->lut_size + 1);
  ctx->lut_gamma = (float)(ctx->lut_size / DBGDRAW_TWO_PI);

  for (uint32_t i = 0; i < ctx->lut_size + 1; ++i)
  {
    ctx->sinf_lut[i] = sinf((float)(((float)i / ctx->lut_size) * DBGDRAW_TWO_PI));
    ctx->cosf_lut[i] = cosf((float)(((float)i / ctx->lut_size) * DBGDRAW_TWO_PI));
  }
#endif /* DBGDRAW_USE_TRANSCENDENTAL_LUT */

  ctx->verts_len = 0;
  ctx->verts_cap = DD_MAX(16, desc->max_vertices);
  ctx->verts_data = DBGDRAW_MALLOC(ctx->verts_cap * sizeof(dd_vertex_t));
  if (!ctx->verts_data)
  {
    return DBGDRAW_ERR_FAILED_ALLOC;
  }
  DBGDRAW_MEMSET(ctx->verts_data, 0, ctx->verts_cap * sizeof(dd_vertex_t));

  ctx->commands_len = 0;
  ctx->commands_cap = DD_MAX(16, desc->max_commands);
  ctx->commands = DBGDRAW_MALLOC(ctx->commands_cap * sizeof(dd_cmd_t));
  if (!ctx->commands)
  {
    return DBGDRAW_ERR_FAILED_ALLOC;
  }
  DBGDRAW_MEMSET(ctx->commands, 0, ctx->commands_cap * sizeof(dd_cmd_t));

  ctx->instance_cap = DD_MAX(512, desc->max_instances);

  ctx->cur_cmd = NULL;
  ctx->color = (dd_color_t){0, 0, 0, 255};
  ctx->detail_level = DD_MAX(desc->detail_level, 0);
  ctx->xform = dd_mat4_identity();
  ctx->frustum_cull = desc->enable_frustum_cull;
  ctx->primitive_size = 1.5f;
  ctx->view = dd_mat4_identity();
  ctx->proj = dd_mat4_identity();
  ctx->aa_radius = dd_vec2(desc->line_antialias_radius, 0.0f);
  ctx->enable_depth_test = desc->enable_depth_test;

  dd_backend_init(ctx);

#if DBGDRAW_HAS_TEXT_SUPPORT
  ctx->fonts_len = 0;
  ctx->fonts_cap = DD_MAX(8, desc->max_fonts);
  ctx->fonts = malloc(ctx->fonts_cap * sizeof(dd_font_data_t));
  if (!ctx->fonts)
  {
    return DBGDRAW_ERR_FAILED_ALLOC;
  }
  DBGDRAW_MEMSET(ctx->fonts, 0, ctx->fonts_cap * sizeof(dd_font_data_t));
#endif /* DBGDRAW_HAS_TEXT_SUPPORT */

#ifdef DBGDRAW_USE_DEFAULT_FONT
  if ( desc->enable_default_font)
  {
    unsigned char* uncompressed_font_data = DBGDRAW_MALLOC( dd_default_font_info.size );
    int uncompressed_size = dbgdraw__inflate(uncompressed_font_data, dd_default_font_info.data, dd_default_font_info.compressed_size);
    DBGDRAW_ASSERT(uncompressed_size == dd_default_font_info.size);
    (void) uncompressed_size; /* Silence warning in release */
    dd_init_font_from_memory(ctx, uncompressed_font_data, "ProggySquare", 11, 256, 256, &ctx->default_font_idx);
    DBGDRAW_FREE(uncompressed_font_data);
  }
  else
  {
    ctx->default_font_idx = -1;
  }
#endif /* DBGDRAW_USE_DEFAULT_FONT */

  return DBGDRAW_ERR_OK;
}

int32_t
dd_term(dd_ctx_t *ctx)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_FREE(ctx->verts_data);
  DBGDRAW_FREE(ctx->commands);

#if DBGDRAW_USE_TRANSCENDENTAL_LUT
  DBGDRAW_FREE(ctx->sinf_lut);
#endif

#if DBGDRAW_HAS_TEXT_SUPPORT
  for (int32_t i = 0; i < ctx->fonts_len; ++i)
  {
    dd_font_data_t *font = ctx->fonts + i;
    DBGDRAW_FREE(font->name);
  }
  DBGDRAW_FREE(ctx->fonts);
#endif

  dd_backend_term(ctx);
  memset(ctx, 0, sizeof(dd_ctx_t));

  return DBGDRAW_ERR_OK;
}

dd_color_t
dd_hex2color(uint32_t hex)
{
  uint8_t a = (uint8_t)(hex & 0x000000ff);
  uint8_t b = (uint8_t)((hex & 0x0000ff00) >> 8);
  uint8_t g = (uint8_t)((hex & 0x00ff0000) >> 16);
  uint8_t r = (uint8_t)((hex & 0xff000000) >> 24);
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
  return (dd_color_t){(uint8_t)(r * 255),
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
dd_hsl( float hue, float saturation, float light)
{
  float chroma = (1.0f - fabsf(2.0f*light-1.0f)) * saturation;
  float hue_prime = hue / 60.0f;
  int hue_prime_int = (int)ceilf(hue_prime);
  float x =  chroma *fabsf(hue_prime_int%2 + (hue_prime-floorf(hue_prime)) - 1.0f) ;
  dd_color_t color;
  switch(hue_prime_int)
  {
    case 1: color = dd_rgbf(chroma, x, 0); break;
    case 2: color = dd_rgbf(x, chroma, 0); break;
    case 3: color = dd_rgbf(0, chroma, x); break;
    case 4: color = dd_rgbf(0, x, chroma); break;
    case 5: color = dd_rgbf(x, 0, chroma); break;
    case 6: color = dd_rgbf(chroma, 0, x); break;
    default: color = dd_rgbf(0,0,0); 
  }
  float m = light - chroma / 2.0f;
  color.r += (uint8_t)(m * 255);
  color.g += (uint8_t)(m * 255);
  color.b += (uint8_t)(m * 255);
  return color;
}

dd_color_t
dd_interpolate_color(dd_color_t c0, dd_color_t c1, float t)
{
  dd_color_t retcol;
  retcol.r = (uint8_t)((1.0 - t) * (float)c0.r + t * (float)c1.r);
  retcol.g = (uint8_t)((1.0 - t) * (float)c0.g + t * (float)c1.g);
  retcol.b = (uint8_t)((1.0 - t) * (float)c0.b + t * (float)c1.b);
  retcol.a = (uint8_t)((1.0 - t) * (float)c0.a + t * (float)c1.a);
  return retcol;
}

int32_t
dd_set_transform(dd_ctx_t *ctx, float *xform)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_VALIDATE(ctx->cur_cmd == NULL, DBGDRAW_ERR_PREV_CMD_NOT_ENDED);
  memcpy(ctx->xform.data, xform, sizeof(ctx->xform));
  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_shading_type(dd_ctx_t *ctx, dd_shading_t shading_type)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_VALIDATE(ctx->cur_cmd == NULL, DBGDRAW_ERR_PREV_CMD_NOT_ENDED);
  ctx->shading_type = shading_type;
  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_fill_type(dd_ctx_t *ctx, dd_fill_t fill_type)
{
  DBGDRAW_ASSERT(ctx);
  ctx->fill_type = fill_type;
  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_detail_level(dd_ctx_t *ctx, uint8_t level)
{
  DBGDRAW_ASSERT(ctx);
  ctx->detail_level = level;
  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_color(dd_ctx_t *ctx, dd_color_t color)
{
  DBGDRAW_ASSERT(ctx);
  ctx->color = color;
  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_gradient_a(dd_ctx_t *ctx, dd_color_t color, float *pos)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(pos);
  ctx->gradient_a_col = color;
  ctx->gradient_a_pt = dd_vec3(pos[0], pos[1], pos[2]);
  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_gradient_b(dd_ctx_t *ctx, dd_color_t color, float *pos)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(pos);
  ctx->gradient_b_col = color;
  ctx->gradient_b_pt = dd_vec3(pos[0], pos[1], pos[2]);
  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_primitive_size(dd_ctx_t *ctx, float primitive_size)
{
  DBGDRAW_ASSERT(ctx);
  ctx->primitive_size = primitive_size;
  return DBGDRAW_ERR_OK;
}

int32_t
dd_begin_cmd(dd_ctx_t *ctx, dd_mode_t draw_mode)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT((int32_t)draw_mode >= 0 && (int32_t)draw_mode < (int32_t)DBGDRAW_MODE_COUNT);

  DBGDRAW_VALIDATE(ctx->cur_cmd == NULL, DBGDRAW_ERR_PREV_CMD_NOT_ENDED);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->commands, ctx->commands_len + 1, ctx->commands_cap, sizeof(dd_cmd_t));

  ctx->cur_cmd = &ctx->commands[ctx->commands_len];

  ctx->cur_cmd->xform = ctx->xform;
  ctx->cur_cmd->min_depth = 0.0f;
  ctx->cur_cmd->base_index = ctx->verts_len;
  ctx->cur_cmd->vertex_count = 0;
  ctx->cur_cmd->draw_mode = draw_mode;
  ctx->cur_cmd->shading_type = ctx->shading_type;

#if DBGDRAW_HAS_TEXT_SUPPORT
  ctx->cur_cmd->font_idx = -1;
#endif

  return DBGDRAW_ERR_OK;
}

int32_t
dd_set_instance_data(dd_ctx_t* ctx, int32_t instance_count, dd_instance_data_t* data)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  ctx->cur_cmd->instance_count = instance_count;
  ctx->cur_cmd->instance_data = data;
  return DBGDRAW_ERR_OK;
}

int32_t
dd_end_cmd(dd_ctx_t *ctx)
{
  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);

  ctx->commands_len++;
  ctx->cur_cmd = 0;

  return DBGDRAW_ERR_OK;
}

int32_t
dd__cmd_cmp(const void *a, const void *b)
{
  const dd_cmd_t *cmd_a = (const dd_cmd_t *)a;
  const dd_cmd_t *cmd_b = (const dd_cmd_t *)b;

  float key_a = (float)(cmd_a->draw_mode << 20) + cmd_a->min_depth;
  float key_b = (float)(cmd_b->draw_mode << 20) + cmd_b->min_depth;

  return (int32_t)DBGDRAW_ROUND(key_a - key_b);
}

void dd_sort_commands(dd_ctx_t *ctx)
{
  DBGDRAW_ASSERT(ctx);
  if (ctx->commands_len)
  {
    qsort(ctx->commands, ctx->commands_len, sizeof(ctx->commands[0]), dd__cmd_cmp);
  }
}

int32_t
dd_render(dd_ctx_t *ctx)
{
  DBGDRAW_ASSERT(ctx);
  return dd_backend_render(ctx);
}

int32_t
dd_new_frame(dd_ctx_t *ctx, dd_new_frame_info_t *info)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(info);
  DBGDRAW_ASSERT(info->view_matrix);
  DBGDRAW_ASSERT(info->projection_matrix);
  DBGDRAW_ASSERT(info->viewport_size);

  ctx->xform = dd_mat4_identity();
  ctx->verts_len = 0;
  ctx->commands_len = 0;
  ctx->drawcall_count = 0;
  ctx->is_ortho = (info->projection_type == DBGDRAW_ORTHOGRAPHIC);

  memcpy(ctx->view.data, info->view_matrix, sizeof(ctx->view));
  memcpy(ctx->proj.data, info->projection_matrix, sizeof(ctx->proj));
  memcpy(ctx->viewport.data, info->viewport_size, sizeof(dd_vec4_t));

  dd_mat4_t inv_view_matrix = dd_mat4_se3_inverse(ctx->view);
  memcpy(ctx->view_origin.data, inv_view_matrix.col[3].data, sizeof(dd_vec3_t));

  if (ctx->is_ortho)
  {
    ctx->proj_scale_y = 2.0f / ctx->proj.data[5];
  }
  else
  {
    ctx->proj_scale_y = 2.0f * tanf(info->vertical_fov * 0.5f);
  }

  if (ctx->frustum_cull)
  {
    dd_extract_frustum_planes(ctx);
  }

  return DBGDRAW_ERR_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Frustum culling for higher order primitives
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// After Fabian "rygorous" Giessen
void dd__normalize_plane(dd_vec4_t *plane)
{
  float mag = sqrtf(plane->x * plane->x + plane->y * plane->y + plane->z * plane->z);
  plane->x /= mag;
  plane->y /= mag;
  plane->z /= mag;
  plane->w /= mag;
}

void dd_extract_frustum_planes(dd_ctx_t *ctx)
{
  DBGDRAW_ASSERT(ctx);
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

int32_t
dd__frustum_sphere_test(dd_ctx_t *ctx, dd_vec3_t c, float radius)
{
  if (!ctx->frustum_cull)
  {
    return true;
  }
  dd_vec4_t xc = dd_mat4_vec4_mul(ctx->xform, dd_vec4(c.x, c.y, c.z, 1.0));
  for (int32_t i = 0; i < 6; ++i)
  {
    float dot = dd_vec4_dot(xc, ctx->frustum_planes[i]);
    if (dot <= -radius)
    {
      return false;
    }
  }
  return true;
}

int32_t
dd__frustum_aabb_test(dd_ctx_t *ctx, dd_vec3_t min, dd_vec3_t max)
{
  if (!ctx->frustum_cull)
  {
    return true;
  }
  dd_vec4_t min_pt = dd_mat4_vec4_mul(ctx->xform, dd_vec4(min.x, min.y, min.z, 1.0));
  dd_vec4_t max_pt = dd_mat4_vec4_mul(ctx->xform, dd_vec4(max.x, max.y, max.z, 1.0));
  for (int32_t i = 0; i < 6; ++i)
  {
    const dd_vec4_t plane = ctx->frustum_planes[i];
    float d = DD_MAX(min_pt.x * plane.x, max_pt.x * plane.x) +
              DD_MAX(min_pt.y * plane.y, max_pt.y * plane.y) +
              DD_MAX(min_pt.z * plane.z, max_pt.z * plane.z) +
              plane.w;

    if (d < 0.0f)
    {
      return false;
    }
  }
  return true;
}

int32_t
dd__frustum_obb_test(dd_ctx_t *ctx, dd_vec3_t c, dd_mat3_t axes)
{
  if (!ctx->frustum_cull)
  {
    return true;
  }

  dd_vec4_t xc = dd_mat4_vec4_mul(ctx->xform, dd_vec4(c.x, c.y, c.z, 1.0));
  dd_vec4_t xu = dd_mat4_vec4_mul(ctx->xform, dd_vec4(axes.col[0].x, axes.col[0].y, axes.col[0].z, 0.0));
  dd_vec4_t xv = dd_mat4_vec4_mul(ctx->xform, dd_vec4(axes.col[1].x, axes.col[1].y, axes.col[1].z, 0.0));
  dd_vec4_t xn = dd_mat4_vec4_mul(ctx->xform, dd_vec4(axes.col[2].x, axes.col[2].y, axes.col[2].z, 0.0));
  for (int32_t i = 0; i < 6; ++i)
  {
    const dd_vec4_t plane = ctx->frustum_planes[i];

    float pdotu = DD_ABS(dd_vec4_dot(plane, xu));
    float pdotv = DD_ABS(dd_vec4_dot(plane, xv));
    float pdotn = DD_ABS(dd_vec4_dot(plane, xn));

    float effective_radius = (pdotu + pdotv + pdotn);

    float dot = dd_vec4_dot(xc, ctx->frustum_planes[i]);
    if (dot <= -effective_radius)
    {
      return false;
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float dd__pixels_to_world_size(dd_ctx_t *ctx, dd_vec3_t pos, float pixels)
{
  DBGDRAW_ASSERT(ctx);

  dd_vec3_t v = dd_vec3_sub(pos, ctx->view_origin);
  float dist = ctx->is_ortho ? 1.0f : dd_vec3_norm(v);
  float viewport_height = ctx->viewport.w;
  float projected_size = ctx->proj_scale_y * (pixels / viewport_height);
  return dist * projected_size;
}

float dd__world_size_to_pixels(dd_ctx_t *ctx, dd_vec3_t pos, float size)
{
  DBGDRAW_ASSERT(ctx);

  float dist = ctx->is_ortho ? 1.0f : dd_vec3_norm(dd_vec3_sub(pos, ctx->view_origin));
  float viewport_height = ctx->viewport.w - ctx->viewport.y;
  return (size * viewport_height) / dist / ctx->proj_scale_y;
}

void dd__transform_verts(dd_mat4_t xform, dd_vertex_t *start, dd_vertex_t *end, bool normals)
{
  dd_mat4_t normal_mat = dd_mat4_identity();
  if (normals)
    normal_mat = dd_mat4_transpose(dd_mat4_se3_inverse(xform));
  for (dd_vertex_t *it = start; it != end; it++)
  {
    dd_vec3_t pos = dd_mat4_vec3_mul(xform, dd_vec4_to_vec3(it->pos_size), 1);
    it->pos_size = dd_vec4(pos.x, pos.y, pos.z, it->pos_size.w);
    if (normals)
    {
      dd_vec3_t normal = dd_mat4_vec3_mul(normal_mat, it->normal, 0);
      it->normal = normal;
    }
  }
}

dd_mat3_t
dd__get_view_aligned_basis(dd_ctx_t *ctx, dd_vec3_t p)
{
  DBGDRAW_ASSERT(ctx);
  dd_mat3_t m;
  dd_mat4_t inv_view = dd_mat4_se3_inverse(ctx->view);

  if (!ctx->is_ortho)
  {
    dd_vec3_t u = dd_vec4_to_vec3(inv_view.col[1]);
    dd_vec3_t v = dd_vec4_to_vec3(inv_view.col[3]);

    m.col[2] = dd_vec3_normalize(dd_vec3_sub(v, p));

    m.col[0] = dd_vec3_normalize(dd_vec3_cross(u, m.col[2]));
    m.col[1] = dd_vec3_cross(m.col[0], m.col[2]);
  }
  else
  {
    m = dd_mat4_to_mat3(inv_view);
    m.col[1] = dd_vec3_invert(m.col[1]);
  }
  return m;
}

// NOTE(maciej):
// Idea here is to calculate transformation that will map a cylinder with ends at (0,0,0) and (0,1,0) and radius 1
// to a cylinder with endpoints q0 and q1 and radius "radius"
dd_mat4_t
dd__generate_cone_orientation(dd_vec3_t q0, dd_vec3_t q1)
{
  dd_vec3_t n = dd_vec3_sub(q1, q0);
  dd_vec3_t u0 = dd_vec3(n.z, n.z, -n.x - n.y);
  dd_vec3_t u1 = dd_vec3(-n.y - n.z, n.x, n.x);
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

void dd__vertex(dd_ctx_t *ctx, dd_vec3_t *pt)
{
  float sz = ctx->primitive_size;
  dd_color_t out_color = ctx->color;

  if (ctx->fill_type == DBGDRAW_FILL_LINEAR_GRADIENT)
  {
    dd_vec3_t ba = dd_vec3_sub(ctx->gradient_b_pt, ctx->gradient_a_pt);
    dd_vec3_t pa = dd_vec3_sub(*pt, ctx->gradient_a_pt);
    float dot = dd_vec3_dot(pa, ba);
    float ba_norm = dd_vec3_norm_sq(ba);
    float t = DD_MAX(DD_MIN(dot / ba_norm, 1.0f), 0.0f);
    dd_color_t gradient_color = dd_interpolate_color(ctx->gradient_a_col, ctx->gradient_b_col, t);
    out_color = gradient_color;
  }
  ctx->verts_data[ctx->verts_len++] = (dd_vertex_t){.pos_size = {{pt->x, pt->y, pt->z, sz}}, .col = out_color};
  ctx->cur_cmd->vertex_count++;
}

void dd__vertex2d(dd_ctx_t *ctx, dd_vec2_t *pt)
{
  float sz = ctx->primitive_size;
  dd_color_t out_color = ctx->color;

  if (ctx->fill_type == DBGDRAW_FILL_LINEAR_GRADIENT)
  {
    dd_vec2_t grad_a_pt2d = dd_vec2(ctx->gradient_a_pt.x, ctx->gradient_a_pt.y);
    dd_vec2_t grad_b_pt2d = dd_vec2(ctx->gradient_b_pt.x, ctx->gradient_b_pt.y);
    dd_vec2_t ba = dd_vec2_sub(grad_b_pt2d, grad_a_pt2d);
    dd_vec2_t pa = dd_vec2_sub(*pt, grad_a_pt2d);
    float dot = dd_vec2_dot(pa, ba);
    float ba_norm = dd_vec2_norm_sq(ba);
    float t = DD_MAX(DD_MIN(dot / ba_norm, 1.0f), 0.0f);
    dd_color_t gradient_color = dd_interpolate_color(ctx->gradient_a_col, ctx->gradient_b_col, t);
    out_color = gradient_color;
  }
  ctx->verts_data[ctx->verts_len++] = (dd_vertex_t){.pos_size = {{pt->x, pt->y, 0.0, sz}}, .col = out_color};
  ctx->cur_cmd->vertex_count++;
}

void dd__vertex_normal(dd_ctx_t *ctx, dd_vec3_t *pt, dd_vec3_t *nor)
{
  float sz = ctx->primitive_size;
  ctx->verts_data[ctx->verts_len++] = (dd_vertex_t){
      .pos_size = {{pt->x, pt->y, pt->z, sz}},
      .normal = {{nor->x, nor->y, nor->z}},
      .col = ctx->color};
  ctx->cur_cmd->vertex_count++;
}

void dd__vertex_text(dd_ctx_t *ctx, dd_vec3_t *pt, dd_vec2_t *uv)
{
  dd_color_t c = ctx->color;
  c.a = 0;
  float sz = ctx->primitive_size;
  ctx->verts_data[ctx->verts_len++] = (dd_vertex_t){
      .pos_size = {{pt->x, pt->y, pt->z, sz}},
      .uv = {{uv->x, uv->y}},
      .col = c};
  ctx->cur_cmd->vertex_count++;
}

void dd__line(dd_ctx_t *ctx, dd_vec3_t *pt_a, dd_vec3_t *pt_b)
{
  dd__vertex(ctx, pt_a);
  dd__vertex(ctx, pt_b);
}

void dd__triangle(dd_ctx_t *ctx, dd_vec3_t *pt_a, dd_vec3_t *pt_b, dd_vec3_t *pt_c)
{
  dd__vertex(ctx, pt_a);
  dd__vertex(ctx, pt_b);
  dd__vertex(ctx, pt_c);
}

void dd__triangle_normal(dd_ctx_t *ctx, dd_vec3_t *pt_a, dd_vec3_t *pt_b, dd_vec3_t *pt_c, dd_vec3_t *normal)
{
  dd__vertex_normal(ctx, pt_a, normal);
  dd__vertex_normal(ctx, pt_b, normal);
  dd__vertex_normal(ctx, pt_c, normal);
}

void dd__quad_point(dd_ctx_t *ctx, dd_vec3_t *pt_a, dd_vec3_t *pt_b, dd_vec3_t *pt_c, dd_vec3_t *pt_d)
{
  dd__vertex(ctx, pt_a);
  dd__vertex(ctx, pt_b);
  dd__vertex(ctx, pt_c);
  dd__vertex(ctx, pt_d);
}

void dd__quad_stroke(dd_ctx_t *ctx, dd_vec3_t *pt_a, dd_vec3_t *pt_b, dd_vec3_t *pt_c, dd_vec3_t *pt_d)
{
  dd__line(ctx, pt_a, pt_b);
  dd__line(ctx, pt_b, pt_c);
  dd__line(ctx, pt_c, pt_d);
  dd__line(ctx, pt_d, pt_a);
}

void dd__quad_fill(dd_ctx_t *ctx, dd_vec3_t *pt_a, dd_vec3_t *pt_b, dd_vec3_t *pt_c, dd_vec3_t *pt_d)
{
  switch (ctx->cur_cmd->shading_type)
  {
  case DBGDRAW_SHADING_SOLID:
  {
    dd_vec3_t normal = dd_vec3_normalize(dd_vec3_cross(dd_vec3_sub(*pt_c, *pt_a),
                                                       dd_vec3_sub(*pt_b, *pt_a)));
    dd__triangle_normal(ctx, pt_a, pt_b, pt_c, &normal);
    dd__triangle_normal(ctx, pt_a, pt_c, pt_d, &normal);
  }
  break;

  default:
  {
    dd__triangle(ctx, pt_a, pt_b, pt_c);
    dd__triangle(ctx, pt_a, pt_c, pt_d);
  }
  break;
  }
}

void dd__quad(dd_ctx_t *ctx, dd_vec3_t *pt_a, dd_vec3_t *pt_b, dd_vec3_t *pt_c, dd_vec3_t *pt_d)
{
  switch (ctx->cur_cmd->draw_mode)
  {
  case DBGDRAW_MODE_POINT:
    dd__quad_point(ctx, pt_a, pt_b, pt_c, pt_d);
    break;
  case DBGDRAW_MODE_STROKE:
    dd__quad_stroke(ctx, pt_a, pt_b, pt_c, pt_d);
    break;
  case DBGDRAW_MODE_FILL:
    dd__quad_fill(ctx, pt_a, pt_b, pt_c, pt_d);
    break;
  default:
    break;
  }
}

void dd__box_point(dd_ctx_t *ctx, dd_vec3_t *pts)
{
  dd__vertex(ctx, pts + 0);
  dd__vertex(ctx, pts + 1);
  dd__vertex(ctx, pts + 2);
  dd__vertex(ctx, pts + 3);

  dd__vertex(ctx, pts + 4);
  dd__vertex(ctx, pts + 5);
  dd__vertex(ctx, pts + 6);
  dd__vertex(ctx, pts + 7);
}

void dd__box_stroke(dd_ctx_t *ctx, dd_vec3_t *pts)
{
  dd__quad_stroke(ctx, pts + 0, pts + 1, pts + 2, pts + 3);
  dd__quad_stroke(ctx, pts + 4, pts + 5, pts + 6, pts + 7);

  dd__line(ctx, pts + 0, pts + 5);
  dd__line(ctx, pts + 1, pts + 4);
  dd__line(ctx, pts + 2, pts + 7);
  dd__line(ctx, pts + 3, pts + 6);
}

void dd__box_fill(dd_ctx_t *ctx, dd_vec3_t *pts)
{
  dd__quad_fill(ctx, pts + 0, pts + 1, pts + 2, pts + 3);
  dd__quad_fill(ctx, pts + 4, pts + 5, pts + 6, pts + 7);

  dd__quad_fill(ctx, pts + 5, pts + 4, pts + 1, pts + 0);
  dd__quad_fill(ctx, pts + 5, pts + 0, pts + 3, pts + 6);
  dd__quad_fill(ctx, pts + 7, pts + 6, pts + 3, pts + 2);
  dd__quad_fill(ctx, pts + 1, pts + 4, pts + 7, pts + 2);
}

void dd__box(dd_ctx_t *ctx, dd_vec3_t *pts)
{
  switch (ctx->cur_cmd->draw_mode)
  {
  case DBGDRAW_MODE_POINT:
    dd__box_point(ctx, pts);
    break;
  case DBGDRAW_MODE_STROKE:
    dd__box_stroke(ctx, pts);
    break;
  case DBGDRAW_MODE_FILL:
    dd__box_fill(ctx, pts);
    break;
  default:
    break;
  }
}

void dd__arc_point(dd_ctx_t *ctx, dd_vec3_t *center, float radius, float theta, int32_t resolution)
{
  resolution = DD_MAX(4, resolution);
  float d_theta = theta / resolution;
  int32_t full_circle = (int32_t)(!(theta < DBGDRAW_TWO_PI));

  if (!full_circle)
  {
    dd__vertex(ctx, center);
  }

#if 0
//TODO(maciej): Since transcendentals are so slow, we could go ham and precompute some data
// using simd instructions
  __m128 wide_x = _mm_set_ps(0.0f, 1.57f, 3.143f, 0.707f);
  __m128 wide_cosx;
  __m128 wide_sinx = _mm_sincos_ps(&wide_cosx, wide_x);
  float sin_x[4];
  float cos_x[4];
  _mm_store_ps(sin_x, wide_sinx);
  _mm_store_ps(cos_x, wide_cosx);
#endif

  // TODO(maciej): Should the be precomputed in context?
  float theta1;
  float ox1 = 0.0f, oy1 = 0.0f;
  int32_t final_res = full_circle ? resolution : resolution + 1;
  dd_vec3_t pt = dd_vec3(0.0f, 0.0f, center->z);
  for (int32_t i = 0; i < final_res; ++i)
  {
    theta1 = i * d_theta;
    ox1 = radius * DBGDRAW_SIN(theta1);
    oy1 = radius * DBGDRAW_COS(theta1);

    pt.x = center->x + ox1;
    pt.y = center->y + oy1;
    dd__vertex(ctx, &pt);
  }
}

void dd__arc_stroke(dd_ctx_t *ctx, dd_vec3_t *center, float radius, float theta, int32_t resolution)
{
  dd_vec3_t pt_a = dd_vec3(center->x, center->y, center->z);
  dd_vec3_t pt_b = dd_vec3(center->x, center->y + radius, center->z);

  int32_t full_circle = (int32_t)(!(theta < DBGDRAW_TWO_PI));
  if (!full_circle)
  {
    dd__line(ctx, &pt_a, &pt_b);
  }
  else
  {
    resolution+=1;
  }
  
  int32_t mod = full_circle ? -1 : 0;
  float d_theta = theta / (resolution + mod);

  float theta1, theta2;
  float ox1, ox2, oy1, oy2;
  ox1 = ox2 = oy1 = oy2 = 0.0f;
  for (int32_t i = 0; i < resolution; ++i)
  {
    theta1 = i * d_theta;
    theta2 = (i + 1) * d_theta;
    ox1 = radius * DBGDRAW_SIN(theta1);
    ox2 = radius * DBGDRAW_SIN(theta2);
    oy1 = radius * DBGDRAW_COS(theta1);
    oy2 = radius * DBGDRAW_COS(theta2);

    pt_a.x = center->x + ox1;
    pt_a.y = center->y + oy1;
    pt_b.x = center->x + ox2;
    pt_b.y = center->y + oy2;
    dd__line(ctx, &pt_a, &pt_b);
  }

  if (!full_circle)
  {
    pt_a.x = center->x;
    pt_a.y = center->y;
    pt_b.x = center->x + ox2;
    pt_b.y = center->y + oy2;
    dd__line(ctx, &pt_b, &pt_a);
  }
  else
  {
    /* This is an ugly fix to allow non-broken lines.
       We add an extra segment, and we modify its positions to lie on a segment, not vertex. */
    dd_vertex_t* v1 = ctx->verts_data + ctx->cur_cmd->base_index;
    dd_vertex_t* v2 = v1 + 1;
    dd_vec3_t p1 = v1->pos;
    dd_vec3_t p2 = v2->pos;
    v1->pos = dd_vec3(0.5f*(p1.x+p2.x),0.5f*(p1.y+p2.y),0.5f*(p1.z+p2.z));

    v1 = ctx->verts_data + ctx->cur_cmd->base_index + ctx->cur_cmd->vertex_count - 2;
    v2 = v1 + 1;
    p1 = v1->pos;
    p2 = v2->pos;
    v2->pos = dd_vec3(0.5f*(p1.x+p2.x),0.5f*(p1.y+p2.y),0.5f*(p1.z+p2.z));
  }
}

void dd__arc_fill(dd_ctx_t *ctx, dd_vec3_t *center, float radius, float theta, int32_t resolution, uint8_t flip)
{
  float d_theta = theta / resolution;

  float theta1, theta2;
  float ox1, ox2, oy1, oy2;
  ox1 = ox2 = oy1 = oy2 = 0.0f;
  dd_vec3_t pt_a = dd_vec3(0.0f, 0.0f, center->z);
  dd_vec3_t pt_b = dd_vec3(0.0f, 0.0f, center->z);
  dd_vec3_t normal;
  for (int32_t i = 0; i < resolution; ++i)
  {
    theta1 = i * d_theta;
    theta2 = (i + 1) * d_theta;
    ox1 = radius * DBGDRAW_SIN(theta1);
    ox2 = radius * DBGDRAW_SIN(theta2);
    oy1 = radius * DBGDRAW_COS(theta1);
    oy2 = radius * DBGDRAW_COS(theta2);

    pt_a.x = center->x + ox1;
    pt_a.y = center->y + oy1;
    pt_b.x = center->x + ox2;
    pt_b.y = center->y + oy2;
    if (i == 0 && ctx->cur_cmd->shading_type)
    {
      normal = dd_vec3_normalize(dd_vec3_cross(dd_vec3_sub(pt_b, *center), dd_vec3_sub(pt_a, *center)));
      if (flip)
      {
        normal = dd_vec3(-normal.x, -normal.y, -normal.z);
      }
    }

    if (flip)
    {
      if (ctx->cur_cmd->shading_type)
      {
        dd__triangle_normal(ctx, center, &pt_b, &pt_a, &normal);
      }
      else
      {
        dd__triangle(ctx, center, &pt_b, &pt_a);
      }
    }
    else
    {
      if (ctx->cur_cmd->shading_type)
      {
        dd__triangle_normal(ctx, center, &pt_a, &pt_b, &normal);
      }
      else
      {
        dd__triangle(ctx, center, &pt_a, &pt_b);
      }
    }
  }
}

void dd__arc(dd_ctx_t *ctx, dd_vec3_t *center, float radius, float theta, int32_t resolution, uint8_t flip)
{
  switch (ctx->cur_cmd->draw_mode)
  {
  case DBGDRAW_MODE_POINT:
    dd__arc_point(ctx, center, radius, theta, resolution >> 1);
    break;
  case DBGDRAW_MODE_STROKE:
    dd__arc_stroke(ctx, center, radius, theta, resolution);
    break;
  case DBGDRAW_MODE_FILL:
    dd__arc_fill(ctx, center, radius, theta, resolution, flip);
    break;
  default:
    break;
  }
}

//NOTE(maciej): dd_arc(...) deals with both points and strokes
void dd__sphere_point_stroke(dd_ctx_t *ctx, dd_vec3_t *center, float radius, int32_t resolution)
{
  /* Store initial state */
  const int32_t n_rings = 3;
  dd_vertex_t *base_ptr = ctx->verts_data + ctx->verts_len;

  /* Create unit circle, and get data boundaries */
  dd_vertex_t *start_ptr = base_ptr;
  dd_vec3_t zero_pt = dd_vec3(0.0f, 0.0f, 0.0f);
  dd__arc(ctx, &zero_pt, 1.0f, (float)DBGDRAW_TWO_PI, resolution, 0);
  dd_vertex_t *end_ptr = ctx->verts_data + ctx->verts_len;
  int32_t len = (int32_t)(end_ptr - start_ptr);
  int32_t new_verts = n_rings * len;

  /* Copy the memory, so that we have all the rings we need. */
  for (int32_t i = 1; i < n_rings; ++i)
  {
    memcpy(end_ptr, start_ptr, len * sizeof(dd_vertex_t));
    end_ptr += len;
    start_ptr += len;
  }

  dd_mat4_t xform;

  /* Circle A */
  xform = dd_pre_scale(dd_mat4_identity(), dd_vec3(radius, radius, radius));
  xform = dd_pre_translate(xform, *center);
  start_ptr = base_ptr;
  end_ptr = start_ptr + len;
  dd__transform_verts(xform, start_ptr, end_ptr, 0);

  /* Circle B */
  xform = dd_pre_scale(dd_mat4_identity(), dd_vec3(radius, radius, radius));
  xform = dd_pre_rotate(xform, (float)DBGDRAW_PI_OVER_TWO, dd_vec3(1, 0, 0));
  xform = dd_pre_translate(xform, *center);
  start_ptr = end_ptr;
  end_ptr = start_ptr + len;
  dd__transform_verts(xform, start_ptr, end_ptr, 0);

  /* Circle C */
  xform = dd_pre_scale(dd_mat4_identity(), dd_vec3(radius, radius, radius));
  xform = dd_pre_rotate(xform, (float)DBGDRAW_PI_OVER_TWO, dd_vec3(0, 1, 0));
  xform = dd_pre_translate(xform, *center);
  start_ptr = end_ptr;
  end_ptr = start_ptr + len;
  dd__transform_verts(xform, start_ptr, end_ptr, 0);

  /* Tell the context that there is new vertex data */
  ctx->verts_len += (new_verts - (int32_t)len);
  ctx->cur_cmd->vertex_count += (new_verts - (int32_t)len);
}

void dd__sphere_fill(dd_ctx_t *ctx, dd_vec3_t *c, float radius, int32_t resolution)
{
  float half_pi = (float)DBGDRAW_PI_OVER_TWO;
  float half_res = (float)(resolution >> 1);
  float prev_y = -1.0;
  float prev_r = 0.0f;
  dd_vec3_t pt_a, pt_b, pt_c;
  dd_vec3_t normal;

  for (int32_t i = 1; i <= half_res; ++i)
  {
    float phi = ((i / half_res) * 2.0f - 1.0f) * half_pi;
    float curr_r = DBGDRAW_COS(phi) * radius;
    float curr_y = DBGDRAW_SIN(phi);

    float prev_x = 1.0f;
    float prev_z = 0.0f;

    for (int32_t j = 1; j <= resolution; ++j)
    {
      float theta = (j / (float)resolution) * (float)DBGDRAW_TWO_PI;
      float curr_z = DBGDRAW_SIN(theta);
      float curr_x = DBGDRAW_COS(theta);

      pt_a = dd_vec3(c->x + prev_x * prev_r, c->y + prev_y * radius, c->z + prev_z * prev_r);
      pt_b = dd_vec3(c->x + prev_x * curr_r, c->y + curr_y * radius, c->z + prev_z * curr_r);
      pt_c = dd_vec3(c->x + curr_x * curr_r, c->y + curr_y * radius, c->z + curr_z * curr_r);

      if (ctx->cur_cmd->shading_type != DBGDRAW_SHADING_NONE)
      {
        if (i != half_res)
        {
          normal = dd_vec3_normalize(dd_vec3_cross(dd_vec3_sub(pt_c, pt_a), dd_vec3_sub(pt_b, pt_a)));
        }
        dd__triangle_normal(ctx, &pt_a, &pt_b, &pt_c, &normal);
      }
      else
      {
        dd__triangle(ctx, &pt_a, &pt_b, &pt_c);
      }

      pt_a = dd_vec3(c->x + prev_x * prev_r, c->y + prev_y * radius, c->z + prev_z * prev_r);
      pt_b = dd_vec3(c->x + curr_x * curr_r, c->y + curr_y * radius, c->z + curr_z * curr_r);
      pt_c = dd_vec3(c->x + curr_x * prev_r, c->y + prev_y * radius, c->z + curr_z * prev_r);

      if (ctx->cur_cmd->shading_type != DBGDRAW_SHADING_NONE)
      {
        if (i == half_res)
        {
          normal = dd_vec3_normalize(dd_vec3_cross(dd_vec3_sub(pt_c, pt_a), dd_vec3_sub(pt_b, pt_a)));
        }
        dd__triangle_normal(ctx, &pt_a, &pt_b, &pt_c, &normal);
      }
      else
      {
        dd__triangle(ctx, &pt_a, &pt_b, &pt_c);
      }

      prev_x = curr_x;
      prev_z = curr_z;
    }
    prev_y = curr_y;
    prev_r = curr_r;
  }
}

void dd__sphere(dd_ctx_t *ctx, dd_vec3_t *center, float radius, int32_t resolution)
{
  switch (ctx->cur_cmd->draw_mode)
  {
  case DBGDRAW_MODE_POINT:
  case DBGDRAW_MODE_STROKE:
    dd__sphere_point_stroke(ctx, center, radius, resolution);
    break;
  case DBGDRAW_MODE_FILL:
    dd__sphere_fill(ctx, center, radius, resolution);
    break;
  default:
    break;
  }
}

// NOTE(maciej): This transformation will transform a cone with a radius 1 base at (0,0,0) and apex at (0, 0, 1) to a
//               cone that has base at p and oriented as specified by rot, with height and radius controlled by
//               parameters.
dd_mat4_t
dd__get_cone_xform(dd_vec3_t p, dd_mat4_t rot, float radius, float height)
{
  dd_mat4_t xform = dd_pre_rotate(dd_mat4_identity(), (float)DBGDRAW_PI_OVER_TWO, dd_vec3(-1.0f, 0.0f, 0.0f));
  xform = dd_pre_scale(xform, dd_vec3(radius, height, radius));
  xform = dd_mat4_mul(rot, xform);

  xform = dd_pre_translate(xform, p);
  return xform;
}

void dd__cone(dd_ctx_t *ctx, dd_vec3_t a, dd_vec3_t b, float radius, int32_t resolution)
{
  dd_mat4_t rot = dd__generate_cone_orientation(a, b);
  float height = dd_vec3_norm(dd_vec3_sub(a, b));
  bool has_normals = ctx->cur_cmd->draw_mode == DBGDRAW_MODE_FILL && ctx->cur_cmd->shading_type != DBGDRAW_SHADING_NONE;

  dd_mat4_t xform = dd__get_cone_xform(a, rot, radius, height);

  dd_vec3_t zero_pt = dd_vec3(0.0f, 0.0f, 0.0f);
  dd_vertex_t *start_ptr = ctx->verts_data + ctx->verts_len;
  dd__arc(ctx, &zero_pt, 1.0, (float)DBGDRAW_TWO_PI, resolution, 1);
  dd_vertex_t *end_ptr = ctx->verts_data + ctx->verts_len;
  dd__transform_verts(xform, start_ptr, end_ptr, has_normals);

  dd_vec3_t apex = dd_mat4_vec3_mul(xform, dd_vec3(0.0, 0.0, 1.0), 1);

  switch (ctx->cur_cmd->draw_mode)
  {
  case DBGDRAW_MODE_POINT:
    dd__vertex(ctx, &apex);
    break;
  case DBGDRAW_MODE_STROKE:
    for (int32_t i = 0; i < (resolution >> 1); ++i)
    {
      dd_vec3_t pt = dd_vec4_to_vec3(start_ptr->pos_size);
      dd__line(ctx, &pt, &apex);
      start_ptr += 4;
    }
    break;
  case DBGDRAW_MODE_FILL:
    for (int32_t i = 0; i < resolution; ++i)
    {
      start_ptr++;
      dd_vec3_t p1 = dd_vec4_to_vec3(start_ptr->pos_size);
      start_ptr++;
      dd_vec3_t p2 = dd_vec4_to_vec3(start_ptr->pos_size);
      start_ptr++;
      if (has_normals)
      {
        dd_vec3_t normal = dd_vec3_normalize(dd_vec3_cross(dd_vec3_sub(p1, apex), dd_vec3_sub(p2, apex)));
        dd__triangle_normal(ctx, &p2, &p1, &apex, &normal);
      }
      else
      {
        dd__triangle(ctx, &p2, &p1, &apex);
      }
    }
    break;
  default:
    break;
  }
}

void dd__conical_frustum(dd_ctx_t *ctx, dd_vec3_t a, dd_vec3_t b, float radius_a, float radius_b, int32_t resolution)
{
  dd_mat4_t rot = dd__generate_cone_orientation(a, b);
  float height = dd_vec3_norm(dd_vec3_sub(a, b));
  bool has_normals = ctx->cur_cmd->draw_mode == DBGDRAW_MODE_FILL && ctx->cur_cmd->shading_type != DBGDRAW_SHADING_NONE;
  dd_vec3_t pt_bottom = dd_vec3(0.0f, 0.0f, 0.0f);
  dd_vec3_t pt_top = dd_vec3(0.0f, 0.0f, 1.0f);

  dd_mat4_t xform_a = dd__get_cone_xform(a, rot, radius_a, height);
  dd_mat4_t xform_b = dd__get_cone_xform(a, rot, radius_b, height);

  dd_vertex_t *start_ptr_1 = ctx->verts_data + ctx->verts_len;
  dd__arc(ctx, &pt_bottom, 1.0f, (float)DBGDRAW_TWO_PI, resolution, 0);
  dd_vertex_t *end_ptr_1 = ctx->verts_data + ctx->verts_len;
  dd__transform_verts(xform_a, start_ptr_1, end_ptr_1, 0);

  // NOTE(maciej): We do not transform normals above, as it contains schale.
  //               Instead we will just recompute the normal and set it.
  if (has_normals)
  {
    dd_vec3_t pos_1_a = dd_vec4_to_vec3(start_ptr_1->pos_size);
    dd_vec3_t pos_1_b = dd_vec4_to_vec3((start_ptr_1 + 1)->pos_size);
    dd_vec3_t pos_1_c = dd_vec4_to_vec3((start_ptr_1 + 2)->pos_size);
    dd_vec3_t normal_1 = dd_vec3_normalize(dd_vec3_cross(dd_vec3_sub(pos_1_c, pos_1_a),
                                                         dd_vec3_sub(pos_1_b, pos_1_a)));
    for (dd_vertex_t *it = start_ptr_1; it != end_ptr_1; ++it)
    {
      it->normal = normal_1;
    }
  }

  dd_vertex_t *start_ptr_2 = ctx->verts_data + ctx->verts_len;
  dd__arc(ctx, &pt_top, 1.0f, (float)DBGDRAW_TWO_PI, resolution, 1);
  dd_vertex_t *end_ptr_2 = ctx->verts_data + ctx->verts_len;
  dd__transform_verts(xform_b, start_ptr_2, end_ptr_2, 0);

  if (has_normals)
  {
    dd_vec3_t pos_2_a = dd_vec4_to_vec3(start_ptr_2->pos_size);
    dd_vec3_t pos_2_b = dd_vec4_to_vec3((start_ptr_2 + 1)->pos_size);
    dd_vec3_t pos_2_c = dd_vec4_to_vec3((start_ptr_2 + 2)->pos_size);
    dd_vec3_t normal_2 = dd_vec3_normalize(dd_vec3_cross(dd_vec3_sub(pos_2_c, pos_2_a),
                                                         dd_vec3_sub(pos_2_b, pos_2_a)));
    for (dd_vertex_t *it = start_ptr_2; it != end_ptr_2; ++it)
    {
      it->normal = normal_2;
    }
  }

  switch (ctx->cur_cmd->draw_mode)
  {
  case DBGDRAW_MODE_POINT:
    break;
  case DBGDRAW_MODE_STROKE:
    for (int32_t i = 0; i<resolution>> 1; ++i)
    {
      dd_vec3_t p1 = dd_vec4_to_vec3(start_ptr_1->pos_size);
      dd_vec3_t p2 = dd_vec4_to_vec3(start_ptr_2->pos_size);
      dd__line(ctx, &p1, &p2);
      start_ptr_1 += 4;
      start_ptr_2 += 4;
    }
    break;
  case DBGDRAW_MODE_FILL:
    for (int32_t i = 0; i < resolution; ++i)
    {
      start_ptr_1++;
      start_ptr_2++;
      dd_vec3_t p1 = dd_vec4_to_vec3(start_ptr_1->pos_size);
      start_ptr_1++;
      dd_vec3_t p2 = dd_vec4_to_vec3(start_ptr_1->pos_size);
      dd_vec3_t p3 = dd_vec4_to_vec3(start_ptr_2->pos_size);
      start_ptr_2++;
      dd_vec3_t p4 = dd_vec4_to_vec3(start_ptr_2->pos_size);
      start_ptr_1++;
      start_ptr_2++;
      if (has_normals)
      {
        dd_vec3_t normal = dd_vec3_normalize(dd_vec3_cross(dd_vec3_sub(p2, p1), dd_vec3_sub(p3, p1)));
        dd__triangle_normal(ctx, &p2, &p1, &p3, &normal);
        dd__triangle_normal(ctx, &p3, &p1, &p4, &normal);
      }
      else
      {
        dd__triangle(ctx, &p2, &p1, &p3);
        dd__triangle(ctx, &p3, &p1, &p4);
      }
    }
    break;
  default:
    break;
  }
}

void dd__torus_point_stroke(dd_ctx_t *ctx, dd_vec3_t center, float radius_a, float radius_b,
                            int32_t resolution, int32_t n_small_rings)
{
  /* Store useful data */
  dd_vertex_t *base_ptr = ctx->verts_data + ctx->verts_len;
  dd_mat4_t xform = dd_mat4_identity();
  dd_mat4_t rotx = dd_pre_rotate(xform, (float)DBGDRAW_PI_OVER_TWO, dd_vec3(1.0f, 0.0f, 0.0f));
  float radius = 1.0f;
  dd_vertex_t *start_ptr = base_ptr;
  dd_vertex_t *end_ptr = base_ptr;
  size_t len;
  dd_vec3_t zero_pt = dd_vec3(0.0f, 0.0f, 0.0f);
  /* We do large circles only for stroke mode */
  if (ctx->cur_cmd->draw_mode == DBGDRAW_MODE_STROKE)
  {
    /* create unit circle, and get data boundaries */
    start_ptr = base_ptr;
    dd__arc(ctx, &zero_pt, 1.0f, (float)DBGDRAW_TWO_PI, resolution, 0);
    end_ptr = ctx->verts_data + ctx->verts_len;
    len = end_ptr - start_ptr;

    /* copy the memory, so that we have all the rings we need. Also make sure to move verts_len accordingly */
    for (int32_t i = 1; i < 4; ++i)
    {
      memcpy(end_ptr, start_ptr, len * sizeof(dd_vertex_t));
      end_ptr += len;
      start_ptr += len;
    }

    /* Large outer circle */
    radius = radius_a + radius_b;
    xform = dd_pre_scale(rotx, dd_vec3(radius, radius, radius));
    xform = dd_pre_translate(xform, center);
    start_ptr = base_ptr;
    end_ptr = start_ptr + len;
    dd__transform_verts(xform, start_ptr, end_ptr, 0);

    /* Smaller inner circle */
    radius = radius_a - radius_b;
    xform = dd_pre_scale(rotx, dd_vec3(radius, radius, radius));
    xform = dd_pre_translate(xform, center);
    start_ptr = end_ptr;
    end_ptr = start_ptr + len;
    dd__transform_verts(xform, start_ptr, end_ptr, 0);

    /* Medium top circle */
    xform = dd_pre_scale(rotx, dd_vec3(radius_a, radius_a, radius_a));
    xform = dd_pre_translate(xform, dd_vec3_add(dd_vec3(0.0f, radius_b, 0.0f), center));
    start_ptr = end_ptr;
    end_ptr = start_ptr + len;
    dd__transform_verts(xform, start_ptr, end_ptr, 0);

    /* Medium bottom circle */
    xform = dd_pre_scale(rotx, dd_vec3(radius_a, radius_a, radius_a));
    xform = dd_pre_translate(xform, dd_vec3_add(center, dd_vec3(0.0f, -radius_b, 0.0f)));
    start_ptr = end_ptr;
    end_ptr = start_ptr + len;
    dd__transform_verts(xform, start_ptr, end_ptr, 0);

    ctx->verts_len += (int32_t)len * 3;
    ctx->cur_cmd->vertex_count += (int32_t)len * 3;
  }

  /* create data for small circles, and get data boundaries */
  base_ptr = end_ptr;
  start_ptr = base_ptr;
  int32_t res = DD_MAX(resolution >> 1, 4);
  dd__arc(ctx, &zero_pt, 1.0f, (float)DBGDRAW_TWO_PI, res, 0);
  end_ptr = ctx->verts_data + ctx->verts_len;
  len = end_ptr - start_ptr;

  /* copy the memory, so that we have all the rings we need. Also make sure to move verts_len accordingly */
  for (int32_t i = 1; i < n_small_rings; ++i)
  {
    memcpy(end_ptr, start_ptr, len * sizeof(dd_vertex_t));
    end_ptr += len;
    start_ptr += len;
  }
  start_ptr = base_ptr;
  end_ptr = start_ptr + len;

  /* small circles around the torus */
  float theta = (float)DBGDRAW_TWO_PI / n_small_rings;
  xform = dd_pre_scale(rotx, dd_vec3(radius_b, radius_b, radius_b));
  xform = dd_pre_rotate(xform, (float)DBGDRAW_PI_OVER_TWO, dd_vec3(1.0f, 0.0f, 0.0f));
  xform = dd_pre_translate(xform, dd_vec3(radius_a, 0.0f, 0.0f));
  for (int32_t i = 0; i < n_small_rings; ++i)
  {
    xform = dd_pre_rotate(xform, theta, dd_vec3(0.0f, 1.0f, 0.0f));
    dd_mat4_t cxform = dd_pre_translate(xform, center);

    dd__transform_verts(cxform, start_ptr, end_ptr, 0);
    start_ptr = end_ptr;
    end_ptr = start_ptr + len;
  }

  /* Tell the context that there is new vertex data */
  ctx->verts_len += (int32_t)len * (n_small_rings - 1);
  ctx->cur_cmd->vertex_count += (int32_t)len * (n_small_rings - 1);
}

void dd__torus_fill(dd_ctx_t *ctx, dd_vec3_t center, float radius_a, float radius_b,
                    int32_t resolution, int32_t n_small_rings)
{
  (void)n_small_rings;

  bool has_normals = ctx->cur_cmd->draw_mode == DBGDRAW_MODE_FILL && ctx->cur_cmd->shading_type != DBGDRAW_SHADING_NONE;
  int32_t res_a = resolution;
  int32_t res_b = DD_MAX(4, resolution >> 1);
  float step_a = (float)DBGDRAW_TWO_PI / res_a;
  float step_b = (float)DBGDRAW_TWO_PI / res_b;
  float ax = radius_a;
  float ay = 0.0f;
  float az = 0.0f;
  dd_vec3_t p1, p2, p3;
  dd_vec3_t normal;
  for (int32_t i = 0; i < res_a; ++i)
  {
    float c1 = DBGDRAW_COS(i * step_a);
    float c2 = DBGDRAW_COS((i + 1) * step_a);
    float s1 = DBGDRAW_SIN(i * step_a);
    float s2 = DBGDRAW_SIN((i + 1) * step_a);

    float s1az = s1 * az;
    float c1az = c1 * az;
    float s2az = s2 * az;
    float c2az = c2 * az;

    for (int32_t j = 0; j < res_b; ++j)
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
      p1 = dd_vec3_add(p1, center);
      p2 = dd_vec3_add(p2, center);
      p3 = dd_vec3_add(p3, center);
      if (has_normals)
      {
        normal = dd_vec3_normalize(dd_vec3_cross(dd_vec3_sub(p2, p1), dd_vec3_sub(p3, p1)));
        dd__triangle_normal(ctx, &p2, &p1, &p3, &normal);
      }
      else
      {
        dd__triangle(ctx, &p2, &p1, &p3);
      }

      p1.x = bx3;
      p1.y = by1, p1.z = bz3;
      p2.x = bx4;
      p2.y = by0, p2.z = bz4;
      p3.x = bx5;
      p3.y = by1, p3.z = bz5;
      p1 = dd_vec3_add(p1, center);
      p2 = dd_vec3_add(p2, center);
      p3 = dd_vec3_add(p3, center);
      if (has_normals)
      {
        dd__triangle_normal(ctx, &p2, &p1, &p3, &normal);
      }
      else
      {
        dd__triangle(ctx, &p2, &p1, &p3);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Draw Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t
dd__point_ex(dd_ctx_t *ctx, float *a, uint8_t is_3d)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(a);

  int32_t new_verts = 1;
  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_vec3_t pt_a = dd_vec3(a[0], a[1], is_3d ? a[2] : 0.0f);
  dd__vertex(ctx, &pt_a);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_point(dd_ctx_t *ctx, float *a)
{
  return dd__point_ex(ctx, a, 1);
}

int32_t
dd_point2d(dd_ctx_t *ctx, float *a)
{
  return dd__point_ex(ctx, a, 0);
}

int32_t
dd__line_ex(dd_ctx_t *ctx, float *a, float *b, uint8_t is_3d)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(a);
  DBGDRAW_ASSERT(b);

  int32_t new_verts = 2;
  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_vec3_t pt_a = dd_vec3(a[0], a[1], is_3d ? a[2] : 0.0f);
  dd_vec3_t pt_b = dd_vec3(b[0], b[1], is_3d ? b[2] : 0.0f);
  dd__line(ctx, &pt_a, &pt_b);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_line(dd_ctx_t *ctx, float *a, float *b)
{
  return dd__line_ex(ctx, a, b, 1);
}

int32_t
dd_line2d(dd_ctx_t *ctx, float *a, float *b)
{
  return dd__line_ex(ctx, a, b, 0);
}

int32_t
dd__quad_ex(dd_ctx_t *ctx, float *a, float *b, float *c, float *d, uint8_t is_3d)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(a);
  DBGDRAW_ASSERT(b);
  DBGDRAW_ASSERT(c);
  DBGDRAW_ASSERT(d);

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = 4;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 8;
  mode_vert_count[DBGDRAW_MODE_FILL] = 6;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];
  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_vec3_t pt_a = dd_vec3(a[0], a[1], is_3d ? a[2] : 0.0f);
  dd_vec3_t pt_b = dd_vec3(b[0], b[1], is_3d ? b[2] : 0.0f);
  dd_vec3_t pt_c = dd_vec3(c[0], c[1], is_3d ? c[2] : 0.0f);
  dd_vec3_t pt_d = dd_vec3(d[0], d[1], is_3d ? d[2] : 0.0f);

  dd__quad(ctx, &pt_a, &pt_b, &pt_c, &pt_d);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_quad(dd_ctx_t *ctx, float *a, float *b, float *c, float *d)
{
  return dd__quad_ex(ctx, a, b, c, d, 1);
}

int32_t
dd_quad2d(dd_ctx_t *ctx, float *a, float *b, float *c, float *d)
{
  return dd__quad_ex(ctx, a, b, c, d, 0);
}

int32_t
dd__rect_ex(dd_ctx_t *ctx, float *a, float *b, uint8_t is_3d)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(a);
  DBGDRAW_ASSERT(b);

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = 4;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 8;
  mode_vert_count[DBGDRAW_MODE_FILL] = 6;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];
  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_vec3_t pt_a = dd_vec3(a[0], a[1], is_3d ? a[2] : 0.0f);
  dd_vec3_t pt_b = dd_vec3(a[0], b[1], is_3d ? a[2] : 0.0f);
  dd_vec3_t pt_d = dd_vec3(b[0], a[1], is_3d ? b[2] : 0.0f);
  dd_vec3_t pt_c = dd_vec3(b[0], b[1], is_3d ? b[2] : 0.0f);

  dd__quad(ctx, &pt_a, &pt_b, &pt_c, &pt_d);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_rect(dd_ctx_t *ctx, float *a, float *b)
{
  return dd__rect_ex(ctx, a, b, 1);
}

int32_t
dd_rect2d(dd_ctx_t *ctx, float *a, float *b)
{
  return dd__rect_ex(ctx, a, b, 0);
}

int32_t
dd_rounded_rect2d_ex(dd_ctx_t *ctx, float *a, float *b, float *radii)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(a);

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = resolution + 4;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 2 * resolution + 8;
  mode_vert_count[DBGDRAW_MODE_FILL] = 3 * resolution + 6;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  float d_theta = (float)DBGDRAW_TWO_PI / resolution;

  float w = (b[0] - a[0]);
  float h = (b[1] - a[1]);
  float offsets_x[4] = {w - (radii[0] + radii[3]), -radii[1], -(w - (radii[1] + radii[2])), radii[3]};
  float offsets_y[4] = {-radii[0], -(h - (radii[0] + radii[1])), radii[2], h - (radii[2] + radii[3])};
  dd_vec3_t pt0 = dd_vec3(a[0] + radii[3], b[1], 0.0f);

  if (ctx->cur_cmd->draw_mode == DBGDRAW_MODE_FILL)
  {
    dd_vec3_t corners[4] = {0};
    for (int i = 0; i < 4; ++i)
    {
      float r = radii[i];
      float init_theta = i * (float)DBGDRAW_PI_OVER_TWO;
      dd_vec3_t pt1 = dd_vec3(0.0f, 0.0f, 0.0f);
      corners[i] = dd_vec3(pt0.x, pt0.y, 0.0f);

      for (int32_t j = 0; j < resolution / 4; ++j)
      {
        float theta = init_theta + j * d_theta;
        float ox1 = offsets_x[i] + r * sinf(theta);
        float ox2 = offsets_x[i] + r * sinf(theta + d_theta);
        float oy1 = offsets_y[i] + r * cosf(theta);
        float oy2 = offsets_y[i] + r * cosf(theta + d_theta);

        dd__vertex(ctx, &pt0);
        pt1.x = pt0.x + ox1;
        pt1.y = pt0.y + oy1;
        dd__vertex(ctx, &pt1);
        pt1.x = pt0.x + ox2;
        pt1.y = pt0.y + oy2;
        dd__vertex(ctx, &pt1);
      }
      pt0 = pt1;
    }

    dd__triangle(ctx, &corners[0], &corners[1], &corners[2]);
    dd__triangle(ctx, &corners[2], &corners[3], &corners[0]);
  }
  else if (ctx->cur_cmd->draw_mode == DBGDRAW_MODE_STROKE)
  {
    for (int i = 0; i < 4; ++i)
    {
      float init_theta = i * (float)DBGDRAW_PI_OVER_TWO;
      dd_vec3_t pt1 = pt0;
      float r = radii[i];
      for (int32_t j = 0; j < resolution / 4 + 1; ++j)
      {
        float theta = init_theta + j * d_theta;
        float ox1 = offsets_x[i] + r * sinf(theta);
        float oy1 = offsets_y[i] + r * cosf(theta);
        dd__vertex(ctx, &pt1);
        pt1.x = pt0.x + ox1;
        pt1.y = pt0.y + oy1;
        dd__vertex(ctx, &pt1);
      }
      pt0 = pt1;
    }
  }
  else if (ctx->cur_cmd->draw_mode == DBGDRAW_MODE_POINT)
  {
    for (int i = 0; i < 4; ++i)
    {
      float r = radii[i];
      float init_theta = i * (float)DBGDRAW_PI_OVER_TWO;
      dd_vec3_t pt1 = dd_vec3(0.0f, 0.0f, 0.0f);
      for (int32_t j = 0; j < resolution / 4 + 1; ++j)
      {
        float theta = init_theta + j * d_theta;
        float ox1 = offsets_x[i] + r * sinf(theta);
        float oy1 = offsets_y[i] + r * cosf(theta);
        pt1.x = pt0.x + ox1;
        pt1.y = pt0.y + oy1;
        dd__vertex(ctx, &pt1);
      }
      pt0 = pt1;
    }
  }

  return DBGDRAW_ERR_OK;
}

int32_t
dd_rounded_rect2d(dd_ctx_t *ctx, float *a, float *b, float rounding)
{
  float all_corners_rounding[4] = {rounding, rounding, rounding, rounding};
  return dd_rounded_rect2d_ex(ctx, a, b, all_corners_rounding);
}

// TODO(maciej): Sizes should be in pixel size..
int32_t
dd_billboard_rect(dd_ctx_t *ctx, float *p, float width, float height)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(p);

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = 4;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 8;
  mode_vert_count[DBGDRAW_MODE_FILL] = 6;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];
  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_vec3_t pt = dd_vec3(p[0], p[1], p[2]);
  dd_mat3_t m = dd__get_view_aligned_basis(ctx, pt);
  m.col[0] = dd_vec3_scalar_mul(m.col[0], width * 0.5f);
  m.col[1] = dd_vec3_scalar_mul(m.col[1], height * 0.5f);

  dd_vec3_t pt_a = dd_vec3_sub(pt, dd_vec3_add(m.col[0], m.col[1]));
  dd_vec3_t pt_b = dd_vec3_add(pt, dd_vec3_sub(m.col[0], m.col[1]));
  dd_vec3_t pt_c = dd_vec3_add(pt, dd_vec3_add(m.col[0], m.col[1]));
  dd_vec3_t pt_d = dd_vec3_sub(pt, dd_vec3_sub(m.col[0], m.col[1]));

  dd__quad(ctx, &pt_a, &pt_b, &pt_c, &pt_d);

  return DBGDRAW_ERR_OK;
}

int32_t
dd__circle_ex(dd_ctx_t *ctx, float *center, float radius, uint8_t is_3d)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(center);

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = resolution;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 2 * resolution;
  mode_vert_count[DBGDRAW_MODE_FILL] = 3 * resolution;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_vec3_t center_pt = dd_vec3(center[0], center[1], is_3d ? center[2] : 0.0f);
  dd__arc(ctx, &center_pt, radius, (float)DBGDRAW_TWO_PI, resolution, 0);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_circle(dd_ctx_t *ctx, float *center, float radius)
{
  return dd__circle_ex(ctx, center, radius, 1);
}

int32_t
dd_circle2d(dd_ctx_t *ctx, float *center, float radius)
{
  return dd__circle_ex(ctx, center, radius, 0);
}

int32_t
dd_billboard_circle(dd_ctx_t *ctx, float *center, float radius)
{
  DBGDRAW_ASSERT(ctx);
  if (!ctx->cur_cmd)
  {
    return DBGDRAW_ERR_NO_ACTIVE_CMD;
  }
  bool has_normals = ctx->cur_cmd->draw_mode == DBGDRAW_MODE_FILL && ctx->cur_cmd->shading_type != DBGDRAW_SHADING_NONE;

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = resolution;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 2 * resolution;
  mode_vert_count[DBGDRAW_MODE_FILL] = 3 * resolution;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  if (ctx->verts_len + new_verts >= ctx->verts_cap)
  {
    return DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER;
  }

  dd_vec3_t zero_pt = dd_vec3(0.0f, 0.0f, 0.0f);
  dd_vertex_t *start = ctx->verts_data + ctx->verts_len;
  dd__arc(ctx, &zero_pt, radius, (float)DBGDRAW_TWO_PI, resolution, 0);
  dd_vertex_t *end = ctx->verts_data + ctx->verts_len;

  dd_vec3_t cp = dd_vec3(center[0], center[1], center[2]);
  dd_mat3_t m = dd__get_view_aligned_basis(ctx, cp);
  dd_mat4_t xform = dd_mat4_identity();
  xform.col[0] = dd_vec3_to_vec4(m.col[0]);
  xform.col[1] = dd_vec3_to_vec4(m.col[1]);
  xform.col[2] = dd_vec3_to_vec4(m.col[2]);
  xform.col[3] = dd_vec3_to_vec4(cp);
  xform.col[3].w = 1.0f;
  dd__transform_verts(xform, start, end, has_normals);

  return DBGDRAW_ERR_OK;
}

int32_t
dd__arc_ex(dd_ctx_t *ctx, float *center, float radius, float theta, uint8_t is_3d)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(center);

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = resolution + 1;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 2 * resolution;
  mode_vert_count[DBGDRAW_MODE_FILL] = 3 * resolution;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_vec3_t center_pt = dd_vec3(center[0], center[1], is_3d ? center[2] : 0.0f);
  dd__arc(ctx, &center_pt, radius, theta, resolution, 0);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_arc(dd_ctx_t *ctx, float *center, float radius, float theta)
{
  return dd__arc_ex(ctx, center, radius, theta, 1);
}

int32_t
dd_arc2d(dd_ctx_t *ctx, float *center, float radius, float theta)
{
  return dd__arc_ex(ctx, center, radius, theta, 0);
}

int32_t
dd_aabb(dd_ctx_t *ctx, float *a, float *b)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(a);
  DBGDRAW_ASSERT(b);

  dd_vec3_t pt_a = dd_vec3(a[0], a[1], a[2]);
  dd_vec3_t pt_b = dd_vec3(b[0], b[1], b[2]);
  if (!dd__frustum_aabb_test(ctx, pt_a, pt_b))
  {
    return DBGDRAW_ERR_CULLED;
  }

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = 8;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 24;
  mode_vert_count[DBGDRAW_MODE_FILL] = 36;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_vec3_t pts[8] =
      {
          dd_vec3(a[0], a[1], a[2]),
          dd_vec3(b[0], a[1], a[2]),
          dd_vec3(b[0], a[1], b[2]),
          dd_vec3(a[0], a[1], b[2]),

          dd_vec3(b[0], b[1], a[2]),
          dd_vec3(a[0], b[1], a[2]),
          dd_vec3(a[0], b[1], b[2]),
          dd_vec3(b[0], b[1], b[2]),

      };

  dd__box(ctx, pts);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_obb(dd_ctx_t *ctx, float *c, float *m)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(c);
  DBGDRAW_ASSERT(m);

  dd_vec3_t center_pt = dd_vec3(c[0], c[1], c[2]);
  dd_mat3_t axes;
  memcpy(axes.data, m, sizeof(axes)); // TODO(maciej): do we need all these memcpys?

  if (!dd__frustum_obb_test(ctx, center_pt, axes))
  {
    return DBGDRAW_ERR_CULLED;
  }

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = 8;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 24;
  mode_vert_count[DBGDRAW_MODE_FILL] = 36;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_vec3_t v1 = axes.col[0];
  dd_vec3_t v2 = axes.col[1];
  dd_vec3_t v3 = axes.col[2];

  dd_vec3_t pts[8] =
      {
          dd_vec3(center_pt.x + v1.x - v2.x + v3.x, center_pt.y + v1.y - v2.y + v3.y, center_pt.z + v1.z - v2.z + v3.z),
          dd_vec3(center_pt.x - v1.x - v2.x + v3.x, center_pt.y - v1.y - v2.y + v3.y, center_pt.z - v1.z - v2.z + v3.z),
          dd_vec3(center_pt.x - v1.x - v2.x - v3.x, center_pt.y - v1.y - v2.y - v3.y, center_pt.z - v1.z - v2.z - v3.z),
          dd_vec3(center_pt.x + v1.x - v2.x - v3.x, center_pt.y + v1.y - v2.y - v3.y, center_pt.z + v1.z - v2.z - v3.z),

          dd_vec3(center_pt.x - v1.x + v2.x + v3.x, center_pt.y - v1.y + v2.y + v3.y, center_pt.z - v1.z + v2.z + v3.z),
          dd_vec3(center_pt.x + v1.x + v2.x + v3.x, center_pt.y + v1.y + v2.y + v3.y, center_pt.z + v1.z + v2.z + v3.z),
          dd_vec3(center_pt.x + v1.x + v2.x - v3.x, center_pt.y + v1.y + v2.y - v3.y, center_pt.z + v1.z + v2.z - v3.z),
          dd_vec3(center_pt.x - v1.x + v2.x - v3.x, center_pt.y - v1.y + v2.y - v3.y, center_pt.z - v1.z + v2.z - v3.z),

      };

  dd__box(ctx, pts);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_frustum(dd_ctx_t *ctx, float *view_matrix, float *proj_matrix)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(view_matrix);
  DBGDRAW_ASSERT(proj_matrix);

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = 8;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 24;
  mode_vert_count[DBGDRAW_MODE_FILL] = 36;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_mat4_t proj, view;
  memcpy(proj.data, proj_matrix, sizeof(dd_mat4_t));
  memcpy(view.data, view_matrix, sizeof(dd_mat4_t));

  dd_mat4_t inv_clip = dd_mat4_inverse(dd_mat4_mul(proj, view));

  dd_vec4_t pts_a[8] =
      {
          dd_vec4(1.0f, -1.0f, 1.0f, 1.0f),
          dd_vec4(-1.0f, -1.0f, 1.0f, 1.0f),
          dd_vec4(-1.0f, 1.0f, 1.0f, 1.0f),
          dd_vec4(1.0f, 1.0f, 1.0f, 1.0f),

          dd_vec4(-1.0f, -1.0f, -1.0f, 1.0f),
          dd_vec4(1.0f, -1.0f, -1.0f, 1.0f),
          dd_vec4(1.0f, 1.0f, -1.0f, 1.0f),
          dd_vec4(-1.0f, 1.0f, -1.0f, 1.0f),

      };

  dd_vec3_t pts_b[8];
  for (int32_t i = 0; i < 8; ++i)
  {
    pts_a[i] = dd_mat4_vec4_mul(inv_clip, pts_a[i]);
    pts_b[i] = dd_vec4_to_vec3(pts_a[i]);
    pts_b[i] = dd_vec3_scalar_div(pts_b[i], pts_a[i].w);
  }

  dd__box(ctx, pts_b);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_sphere(dd_ctx_t *ctx, float *c, float radius)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(c);

  dd_vec3_t center_pt = dd_vec3(c[0], c[1], c[2]);
  if (!dd__frustum_sphere_test(ctx, center_pt, radius))
  {
    return DBGDRAW_ERR_CULLED;
  }

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t n_rings = 3;

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = n_rings * resolution;
  mode_vert_count[DBGDRAW_MODE_STROKE] = n_rings * resolution * 2;
  mode_vert_count[DBGDRAW_MODE_FILL] = resolution * resolution * 3;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd__sphere(ctx, &center_pt, radius, resolution);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_cone(dd_ctx_t *ctx, float *a, float *b, float radius)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(a);
  DBGDRAW_ASSERT(b);

  dd_vec3_t pt_a = dd_vec3(a[0], a[1], a[2]);
  dd_vec3_t pt_b = dd_vec3(b[0], b[1], b[2]);
  int32_t resolution = 1 << (ctx->detail_level + 2);

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = resolution / 2 + 1;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 3 * resolution;
  mode_vert_count[DBGDRAW_MODE_FILL] = 6 * resolution;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd__cone(ctx, pt_a, pt_b, radius, resolution);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_conical_frustum(dd_ctx_t *ctx, float *a, float *b, float radius_a, float radius_b)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(a);
  DBGDRAW_ASSERT(b);

  dd_vec3_t pt_a = dd_vec3(a[0], a[1], a[2]);
  dd_vec3_t pt_b = dd_vec3(b[0], b[1], b[2]);
  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = resolution;
  mode_vert_count[DBGDRAW_MODE_STROKE] = 5 * resolution;
  mode_vert_count[DBGDRAW_MODE_FILL] = 12 * resolution;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd__conical_frustum(ctx, pt_a, pt_b, radius_a, radius_b, resolution);

  return DBGDRAW_ERR_OK;
}

int32_t
dd_cylinder(dd_ctx_t *ctx, float *a, float *b, float radius)
{
  return dd_conical_frustum(ctx, a, b, radius, radius);
}

int32_t
dd_arrow(dd_ctx_t *ctx, float *a, float *b, float radius, float head_radius, float head_length)
{
  dd_vec3_t pt_a = dd_vec3(a[0], a[1], a[2]);
  dd_vec3_t pt_b = dd_vec3(b[0], b[1], b[2]);
  dd_vec3_t v = dd_vec3_sub(pt_b, pt_a);
  dd_vec3_t pt_c = dd_vec3_add(pt_a, dd_vec3_scalar_mul(v, head_length));

  bool error = dd_cone(ctx, pt_c.data, pt_b.data, head_radius);
  if (error)
  {
    return error;
  }
  return dd_conical_frustum(ctx, pt_a.data, pt_c.data, radius, radius);
}

int32_t
dd_torus(dd_ctx_t *ctx, float *center, float radius_a, float radius_b)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(center);

  dd_mat3_t axes = dd_mat3_identity();
  axes.col[0].x = radius_a * 2;
  axes.col[1].y = radius_b * 2;
  axes.col[2].z = radius_a * 2;
  dd_vec3_t center_pt = dd_vec3(center[0], center[1], center[2]);
  if (!dd__frustum_obb_test(ctx, center_pt, axes))
  {
    return DBGDRAW_ERR_CULLED;
  }

  int32_t resolution = 1 << (ctx->detail_level + 2);
  int32_t n_big_rings = 4;
  int32_t n_small_rings = resolution >> 1;
  int32_t n_rings = n_big_rings + n_small_rings;

  int32_t mode_vert_count[DBGDRAW_MODE_COUNT];
  mode_vert_count[DBGDRAW_MODE_POINT] = n_rings * resolution,
  mode_vert_count[DBGDRAW_MODE_STROKE] = n_rings * resolution * 2,
  mode_vert_count[DBGDRAW_MODE_FILL] = n_small_rings * resolution * 2 * 3;
  int32_t new_verts = mode_vert_count[ctx->cur_cmd->draw_mode];

  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  switch (ctx->cur_cmd->draw_mode)
  {
  case DBGDRAW_MODE_POINT:
    dd__torus_point_stroke(ctx, center_pt, radius_a, radius_b, resolution >> 1, n_small_rings);
    break;
  case DBGDRAW_MODE_STROKE:
    dd__torus_point_stroke(ctx, center_pt, radius_a, radius_b, resolution, n_small_rings);
    break;
  default:
    dd__torus_fill(ctx, center_pt, radius_a, radius_b, resolution, n_small_rings);
    break;
  }

  return DBGDRAW_ERR_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Font Loading
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if DBGDRAW_HAS_TEXT_SUPPORT

int32_t
  dd_init_font_from_memory(dd_ctx_t *ctx, const void *ttf_buf,
                         const char *name, int32_t font_size, int32_t width, int32_t height,
                         int32_t *font_idx)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(ttf_buf);
  DBGDRAW_ASSERT(name);
  DBGDRAW_ASSERT(font_idx);

  DBGDRAW_VALIDATE(ctx->fonts_len < ctx->fonts_cap, DBGDRAW_ERR_FONT_LIMIT_REACHED);
  dd_font_data_t *font = ctx->fonts + ctx->fonts_len;
  font->bitmap_width = width;
  font->bitmap_height = height;
  font->size = font_size;
  size_t name_len = strnlen(name, 4096);
  font->name = DBGDRAW_MALLOC(name_len);
  strncpy(font->name, name, name_len);

  uint8_t *pixel_buf = DBGDRAW_MALLOC(width * height);
  memset(pixel_buf, 0, width * height);

  stbtt_InitFont(&font->info, ttf_buf, 0);
  float to_pixel_scale = stbtt_ScaleForPixelHeight(&font->info, (float)font->size);

  int32_t ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
  font->ascent = to_pixel_scale * ascent;
  font->descent = to_pixel_scale * descent;
  font->line_gap = to_pixel_scale * line_gap;
  int32_t num_latin_chars = 127 - 32;
  int32_t num_greek_chars = 0x3C9 - 0x391;
  stbtt_pack_context spc = {0};
  stbtt_PackBegin(&spc, pixel_buf, width, height, 0, 1, NULL);
  stbtt_PackSetOversampling(&spc, 2, 2);
  stbtt_PackFontRange(&spc, (void *)ttf_buf, 0, (float)font->size, 32, num_latin_chars, font->char_data);
  stbtt_PackFontRange(&spc, (void *)ttf_buf, 0, (float)font->size, 0x391, num_greek_chars, font->char_data + num_latin_chars);
  stbtt_PackEnd(&spc);

  dd_backend_init_font_texture(ctx, pixel_buf, width, height, &font->tex_id);

  DBGDRAW_FREE(pixel_buf);

  *font_idx = ctx->fonts_len++;
  return DBGDRAW_ERR_OK;
}

/*UTF-8 decoder by Bjoern Hoehrmann. See end of file for licensing*/
#define DD_UTF8_ACCEPT 0
#define DD_UTF8_REJECT 12

static const uint8_t dd_utf8d_table[] = {
  // The first part of the table maps bytes to character classes that
  // to reduce the size of the transition table and create bitmasks.
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

  // The second part is a transition table that maps a combination
  // of a state of the automaton and a character class to a state.
   0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
  12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
  12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
  12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
  12,36,12,12,12,12,12,12,12,12,12,12, 
};

uint32_t inline
dd__utf8_decode(uint32_t* state, uint32_t* codep, uint32_t byte)
{
  uint32_t type = dd_utf8d_table[byte];

  *codep = (*state != DD_UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = dd_utf8d_table[256 + *state + type];
  return *state;
}

const char*
dd__decode_char(const char* str, uint32_t* cp)
{
  uint32_t state = 0;

  int32_t count = 0;
  const uint8_t* ustr = (const uint8_t*)str;
  for (; *ustr; ++ustr)
  {
    count++;
    if (!dd__utf8_decode(&state, cp, *ustr))
    {
      break;
    }
  }
  str += count;

  if (state != DD_UTF8_ACCEPT)
  {
    return NULL;
  }
  
  return str;
}

int32_t
dd__codepoint_to_index(uint32_t codepoint)
{
  int shift = 0xf000 - 151;
  if( codepoint < 0xf000 )
  {
    if ( codepoint < 128 )  { shift = 32; }
    else                    { shift = (0x391 - 95); }
  }
  // int32_t shift = (codepoint < 128) ? 32 : (0x391 - 95);
  return codepoint - shift;
}

int32_t
dd__strlen(const char* str)
{
  uint32_t cp;
  uint32_t state = 0;

  int32_t count = 0;
  const uint8_t* ustr = (const uint8_t*)str;
  for (; *ustr; ++ustr)
  {
    if (!dd__utf8_decode(&state, &cp, *ustr))
    {
      count++;
    }
  }

  if (state != DD_UTF8_ACCEPT) { count = 0; }

  return count;
}

void dd_get_text_size_font_space(dd_ctx_t *ctx, int32_t font_idx, const char *str, int32_t strlen, float *width, float *height)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(str);
  DBGDRAW_VALIDATE(font_idx < ctx->fonts_cap, DBGDRAW_ERR_OUT_OF_BOUNDS_ACCESS);

  dd_font_data_t *font = ctx->fonts + font_idx;
  *width = 0;
  *height = font->ascent - font->descent;
  for (int32_t i = 0; i < strlen; ++i)
  {
    uint32_t cp, idx;
    str = dd__decode_char(str, &cp);
    idx = dd__codepoint_to_index(cp);
    stbtt_aligned_quad q;
    float dummy;
    stbtt_GetPackedQuad(font->char_data, font->bitmap_width, font->bitmap_height, idx, width, &dummy, &q, 0);
  }
}

#ifndef DBGDRAW_NO_STDIO
int32_t
dd_init_font_from_file(dd_ctx_t *ctx, const char *font_path, const char* font_name,
                       int32_t font_size, int32_t width, int32_t height,
                       int32_t *font_idx)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(font_path);
  DBGDRAW_ASSERT(font_idx);
  if (font_path)
  {
    FILE *fp = fopen(font_path, "rb");
    if (!fp)
    {
      return DBGDRAW_ERR_FONT_FILE_NOT_FOUND;
    }
    else
    {
      fseek(fp, 0L, SEEK_END);
      size_t size = ftell(fp);
      rewind(fp);

      uint8_t *ttf_buffer = DBGDRAW_MALLOC(size);
      fread(ttf_buffer, 1, size, fp);
      fclose(fp);

      dd_init_font_from_memory(ctx, ttf_buffer, font_name, font_size, width, height, font_idx);
    }
  }
  return DBGDRAW_ERR_OK;
}
#endif

// NOTE(maciej): I do not expect to have more like 5 fonts, so it is just pure search over an array
int32_t
dd_find_font(dd_ctx_t *ctx, char *font_name)
{
  for (int32_t i = 0; i < ctx->fonts_len; ++i)
  {
    if (strncmp(font_name, ctx->fonts[i].name, 4096) == 0)
    {
      return i;
    }
  }
  return -1;
}

int32_t
dd_set_font(dd_ctx_t *ctx, int32_t font_idx)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_VALIDATE(ctx->cur_cmd == NULL, DBGDRAW_ERR_PREV_CMD_NOT_ENDED);
  DBGDRAW_VALIDATE(font_idx >= 0 && font_idx < ctx->fonts_len, DBGDRAW_ERR_INVALID_FONT_REQUESTED);
  ctx->active_font_idx = font_idx;
  return DBGDRAW_ERR_OK;
}

// NOTE(maciej): "Decoding" below" is even worse, as it relies on the fact that we only
// requested latin and greek chars. The real deal needs a hashtable most likely
int32_t
dd_text_line(dd_ctx_t *ctx, float *pos, const char *str, dd_text_info_t *info)
{
  DBGDRAW_ASSERT(ctx);
  DBGDRAW_ASSERT(pos);

  DBGDRAW_VALIDATE(ctx->cur_cmd->draw_mode == DBGDRAW_MODE_FILL, DBGDRAW_ERR_INVALID_MODE);
  DBGDRAW_VALIDATE(ctx->cur_cmd != NULL, DBGDRAW_ERR_NO_ACTIVE_CMD);
  ctx->cur_cmd->font_idx = ctx->active_font_idx;
  dd_font_data_t *font = ctx->fonts + ctx->active_font_idx;
  DBGDRAW_VALIDATE(font->name != NULL, DBGDRAW_ERR_USING_TEXT_WITHOUT_FONT);

  // NOTE(maciej): Only handles valid UTF-8 characters between cps U+0000 and U+07FF! It's awful!!
  int32_t n_chars = dd__strlen(str);
  uint8_t do_clipping = info && (info->clip_rect.w > 0 && info->clip_rect.h > 0);

  int32_t new_verts = 6 * n_chars;
  DBGDRAW_HANDLE_OUT_OF_MEMORY(ctx->verts_data, ctx->verts_len + new_verts, ctx->verts_cap, sizeof(dd_vertex_t));

  dd_vec3_t p = dd_vec3(pos[0], pos[1], pos[2]);
  float world_size = dd__pixels_to_world_size(ctx, p, (float)font->size);
  float scale = fabsf(world_size / font->size);
  // scale = 0.05f;

  ctx->cur_cmd->min_depth = dd_mat4_vec3_mul(ctx->cur_cmd->xform, p, 1).z;

  dd_mat3_t m;
  if( !ctx->is_ortho)
  {
    m  = dd__get_view_aligned_basis(ctx, p);
  }

  float width, height;
  if (info && info->width > 0)
  {
    width = info->width;
    height = info->height;
  }
  else
  {
    dd_get_text_size_font_space(ctx, ctx->cur_cmd->font_idx, str, n_chars, &width, &height);
    width *= scale;
    height *= scale;
  }

  dd_text_valign_t vert_align = DBGDRAW_TEXT_BASELINE;
  dd_text_halign_t horz_align = DBGDRAW_TEXT_LEFT;
  if (info)
  {
    vert_align = info->vert_align;
    horz_align = info->horz_align;
    info->width = width;
    info->height = height;
  }

  float vert_offset = 0.0, horz_offset = 0.0;
  switch (horz_align)
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

  float sign = 1.0;
  if( ctx->is_ortho && ctx->proj.col[1].y >= 0.0 ) { sign = -1.0f; }
  switch (vert_align)
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
      vert_offset = sign * scale * 0.5f * (font->ascent + font->descent);
      break;
  }

  p.x += horz_offset;
  p.y += vert_offset;

  float x = 0;
  float y = 0;
  float start_x = 1e9;

  dd_vec3_t pt_a, pt_b, pt_c;
  dd_vec2_t uv_a, uv_b, uv_c;
  dd_vertex_t *start = ctx->verts_data + ctx->verts_len;
  for (int32_t i = 0; i < n_chars; ++i)
  {
    uint32_t cp=0;
    str = dd__decode_char(str, &cp);
    int32_t idx = dd__codepoint_to_index(cp);

    stbtt_aligned_quad q;
    stbtt_GetPackedQuad(font->char_data, font->bitmap_width, font->bitmap_height, idx, &x, &y, &q, 0);

    dd_vec3_t min_pt = dd_vec3( scale * q.x0, sign * scale * q.y1, 0.0);
    dd_vec3_t max_pt = dd_vec3( scale * q.x1, sign * scale * q.y0, 0.0);
    if (ctx->is_ortho)
    {
      min_pt = dd_vec3_add(p, min_pt);
      max_pt = dd_vec3_add(p, max_pt);
    }

    if (do_clipping)
    {
      float min_x = min_pt.x;
      float max_x = max_pt.x;
      float min_y = min_pt.y;
      float max_y = max_pt.y;

      float w1 = max_x - min_x;
      float h1 = max_pt.y - min_pt.y;

      // Clip from top
      if (min_y >= info->clip_rect.y + info->clip_rect.h)
      {
        continue;
      }
      if (max_y >= info->clip_rect.y + info->clip_rect.h)
      {
        max_pt.y = DD_MIN(max_y, info->clip_rect.y + info->clip_rect.h);
        float t1 = max_pt.y - min_y;
        float h2 = q.t1 - q.t0;
        float t2 = (t1 * h2) / h1;
        q.t0 = q.t1 - t2;
      }

      // Clip from bottom
      if (max_y < info->clip_rect.y)
      {
        continue;
      }
      if (min_y < info->clip_rect.y)
      {
        min_pt.y = DD_MAX(min_y, info->clip_rect.y);
        float t1 = max_y - min_pt.y;
        float h2 = q.t1 - q.t0;
        float t2 = (t1 * h2) / h1;
        q.t1 = q.t0 + t2;
      }

      // Clip from right
      if (min_x >= info->clip_rect.x + info->clip_rect.w)
      {
        continue;
      }
      if (max_x >= info->clip_rect.x + info->clip_rect.w)
      {
        max_pt.x = DD_MIN(max_x, info->clip_rect.x + info->clip_rect.w);
        float t1 = max_pt.x - min_x;
        float w2 = q.s1 - q.s0;
        float t2 = (t1 * w2) / w1;
        q.s1 = q.s0 + t2;
      }

      // Clip from left
      if (max_x < info->clip_rect.x)
      {
        continue;
      }
      if (min_x < info->clip_rect.x)
      {
        min_pt.x = DD_MAX(min_x, info->clip_rect.x);
        float t1 = max_x - min_pt.x;
        float w2 = q.s1 - q.s0;
        float t2 = (t1 * w2) / w1;
        q.s0 = q.s1 - t2;
      }
    }

    start_x = DD_MIN(start_x, min_pt.x);

    pt_a = dd_vec3(min_pt.x, max_pt.y, 0.0);
    uv_a = dd_vec2(q.s0, q.t0);
    pt_b = dd_vec3(max_pt.x, min_pt.y, 0.0);
    uv_b = dd_vec2(q.s1, q.t1);
    pt_c = dd_vec3(max_pt.x, max_pt.y, 0.0);
    uv_c = dd_vec2(q.s1, q.t0);

    dd__vertex_text(ctx, &pt_a, &uv_a);
    dd__vertex_text(ctx, &pt_b, &uv_b);
    dd__vertex_text(ctx, &pt_c, &uv_c);

    pt_a = dd_vec3(min_pt.x, max_pt.y, 0.0);
    uv_a = dd_vec2(q.s0, q.t0);
    pt_b = dd_vec3(min_pt.x, min_pt.y, 0.0);
    uv_b = dd_vec2(q.s0, q.t1);
    pt_c = dd_vec3(max_pt.x, min_pt.y, 0.0);
    uv_c = dd_vec2(q.s1, q.t1);

    dd__vertex_text(ctx, &pt_a, &uv_a);
    dd__vertex_text(ctx, &pt_b, &uv_b);
    dd__vertex_text(ctx, &pt_c, &uv_c);
  }
  dd_vertex_t *end = ctx->verts_data + ctx->verts_len;

  if (!ctx->is_ortho)
  {
    dd_mat4_t xform = dd_mat4_identity();
    xform.col[0] = dd_vec3_to_vec4(m.col[0]);
    xform.col[1] = dd_vec3_to_vec4(m.col[1]);
    xform.col[2] = dd_vec3_to_vec4(m.col[2]);
    xform.col[3] = dd_vec3_to_vec4(p);
    xform.col[3].w = 1.0f;
    dd__transform_verts(xform, start, end, 0);
  }
  
  if (info)
  {
    info->anchor = dd_vec3(start_x, p.y + scale * font->descent, p.z);
    info->anchor.x += horz_offset;
    info->anchor.y += vert_offset;
  }

  return DBGDRAW_ERR_OK;
}

#endif /* DBGDRAW_HAS_TEXT_SUPPORT */

const char *dd_error_message(int32_t error_code)
{
  switch (error_code)
  {
  case DBGDRAW_ERR_OK:
    return "[DBGDRAW ERROR] No error";
    break;
  case DBGDRAW_ERR_FAILED_ALLOC:
    return "[DBGDRAW ERROR] Failed to allocate requested memory.";
    break;
  case DBGDRAW_ERR_NO_ACTIVE_CMD:
    return "[DBGDRAW ERROR] No active command. Make sure you're calling 'dd_begin_cmd' before any calls that require active command, like the commands for adding primitives.";
    break;
  case DBGDRAW_ERR_PREV_CMD_NOT_ENDED:
    return "[DBGDRAW ERROR] Previous command was not ended. You might be using a dbgdraw call which requires termination of previous command with 'dd_cmd_end'.";
    break;
  case DBGDRAW_ERR_OUT_OF_VERTEX_BUFFER:
    return "[DBGDRAW ERROR] Out of vertex buffer memory.";
    break;
  case DBGDRAW_ERR_OUT_OF_COMMAND_BUFFER:
    return "[DBGDRAW ERROR] Out of command buffer memory.";
    break;
  case DBGDRAW_ERR_OUT_OF_MEMORY:
    return "[DBGDRAW ERROR] Out of memory.";
    break;
  case DBGDRAW_ERR_CULLED:
    return "[DBGDRAW INFO] Primitive culled";
    break;
  case DBGDRAW_ERR_FONT_FILE_NOT_FOUND:
    return "[DBGDRAW ERROR] File not found";
    break;
  case DBGDRAW_ERR_FONT_LIMIT_REACHED:
    return "[DBGDRAW ERROR] Out of font texture slots.";
    break;
  case DBGDRAW_ERR_OUT_OF_BOUNDS_ACCESS:
    return "[DBGDRAW ERROR] Attmpting to access memory out of bounds.";
    break;
  case DBGDRAW_ERR_INVALID_FONT_REQUESTED:
    return "[DBGDRAW ERROR] The requested font does not exist. Please initialize font first and use the returned id";
    break;
  case DBGDRAW_ERR_USING_TEXT_WITHOUT_FONT:
    return "[DBGDRAW ERROR] The active font used in \"dd_text_line(...)\" is invalid. Check if the font was properly initialized.\n"
           "                 If you are trying to use the default font, ensure that it was requested during the context initialization";
    break;
  case DBGDRAW_ERR_INVALID_MODE:
    return "[DBGDRAW ERROR] Text rendering is only supported when using fill mode (DBGDRAW_MODE_FILL)";
    break;
  default:
    return "[DBGDRAW ERROR] Unknown error";
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vector Math
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline dd_vec2_t
dd_vec2(float x, float y)
{
  return (dd_vec2_t){{x, y}};
}

inline dd_vec3_t
dd_vec3(float x, float y, float z)
{
  return (dd_vec3_t){{x, y, z}};
}

inline dd_vec4_t
dd_vec4(float x, float y, float z, float w)
{
  return (dd_vec4_t){{x, y, z, w}};
}

inline dd_mat3_t
dd_mat3_identity()
{
  return (dd_mat3_t){{1, 0, 0, 0, 1, 0, 0, 0, 1}};
}

inline dd_mat4_t
dd_mat4_identity()
{
  return (dd_mat4_t){{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}};
}

dd_vec2_t
dd_vec2_add(dd_vec2_t a, dd_vec2_t b)
{
  return dd_vec2(a.x + b.x, a.y + b.y);
}

dd_vec2_t
dd_vec2_sub(dd_vec2_t a, dd_vec2_t b)
{
  return dd_vec2(a.x - b.x, a.y - b.y);
}

dd_vec3_t
dd_vec3_add(dd_vec3_t a, dd_vec3_t b)
{
  return dd_vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

dd_vec3_t
dd_vec3_sub(dd_vec3_t a, dd_vec3_t b)
{
  return dd_vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

dd_vec4_t
dd_vec4_add(dd_vec4_t a, dd_vec4_t b)
{
  return dd_vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

dd_vec4_t
dd_vec4_sub(dd_vec4_t a, dd_vec4_t b)
{
  return dd_vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

dd_vec3_t
dd_vec3_scalar_mul(dd_vec3_t a, float s)
{
  return dd_vec3(a.x * s, a.y * s, a.z * s);
}

dd_vec3_t
dd_vec3_scalar_div(dd_vec3_t a, float s)
{
  float denom = 1.0f / s;
  return dd_vec3(a.x * denom, a.y * denom, a.z * denom);
}

float dd_vec2_dot(dd_vec2_t a, dd_vec2_t b)
{
  return a.x * b.x + a.y * b.y;
}

float dd_vec3_dot(dd_vec3_t a, dd_vec3_t b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

dd_vec3_t
dd_vec3_cross(dd_vec3_t a, dd_vec3_t b)
{
  return dd_vec3((a.y * b.z - a.z * b.y), (a.z * b.x - a.x * b.z), (a.x * b.y - a.y * b.x));
}

dd_vec3_t
dd_vec3_invert(dd_vec3_t v)
{
  return dd_vec3(-v.x, -v.y, -v.z);
}

dd_vec2_t
dd_vec2_normalize(dd_vec2_t v)
{
  float denom = 1.0f / sqrtf(v.x * v.x + v.y * v.y);
  return dd_vec2(v.x * denom, v.y * denom);
}

dd_vec3_t
dd_vec3_normalize(dd_vec3_t v)
{
  float denom = 1.0f / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  return dd_vec3(v.x * denom, v.y * denom, v.z * denom);
}

float dd_vec2_norm_sq(dd_vec2_t v)
{
  return v.x * v.x + v.y * v.y;
}

float dd_vec2_norm(dd_vec2_t v)
{
  return (float)sqrt(v.x * v.x + v.y * v.y);
}

float dd_vec3_norm_sq(dd_vec3_t v)
{
  return v.x * v.x + v.y * v.y + v.z * v.z;
}

float dd_vec3_norm(dd_vec3_t v)
{
  return (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

dd_vec4_t
dd_vec3_to_vec4(dd_vec3_t v)
{
  return dd_vec4(v.x, v.y, v.z, 0.0f);
}

float dd_vec4_dot(dd_vec4_t a, dd_vec4_t b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

dd_vec4_t
dd_vec4_mul(dd_vec4_t a, dd_vec4_t b)
{
  return dd_vec4((a.x * b.x), (a.y * b.y), (a.z * b.z), (a.w * b.w));
}

dd_vec4_t
dd_vec4_scalar_mul(dd_vec4_t a, float s)
{
  return dd_vec4(a.x * s, a.y * s, a.z * s, a.w * s);
}

dd_vec3_t
dd_vec4_to_vec3(dd_vec4_t a)
{
  return dd_vec3(a.x, a.y, a.z);
}

dd_vec3_t
dd_mat3_vec3_mul(dd_mat3_t m, dd_vec3_t v)
{
  dd_vec3_t o;
  o.x = m.data[0] * v.x + m.data[3] * v.y + m.data[6] * v.z;
  o.y = m.data[1] * v.x + m.data[4] * v.y + m.data[7] * v.z;
  o.z = m.data[2] * v.x + m.data[5] * v.y + m.data[8] * v.z;
  return o;
}

float dd_mat4_determinant(dd_mat4_t m)
{
  float C[4];
  float coeffs[6];

  /* coeffs are determinants of 2x2 matrices */
  coeffs[0] = m.data[10] * m.data[15] - m.data[14] * m.data[11];
  coeffs[1] = m.data[6] * m.data[11] - m.data[10] * m.data[7];
  coeffs[2] = m.data[2] * m.data[7] - m.data[6] * m.data[3];
  coeffs[3] = m.data[6] * m.data[15] - m.data[14] * m.data[7];
  coeffs[4] = m.data[2] * m.data[11] - m.data[10] * m.data[3];
  coeffs[5] = m.data[2] * m.data[15] - m.data[14] * m.data[3];

  /* Cofactor matrix */
  /*00*/ C[0] = m.data[5] * coeffs[0] - m.data[9] * coeffs[3] + m.data[13] * coeffs[1];
  /*01*/ C[1] = m.data[9] * coeffs[5] - m.data[1] * coeffs[0] - m.data[13] * coeffs[4]; /* negated */
  /*02*/ C[2] = m.data[1] * coeffs[3] - m.data[5] * coeffs[5] + m.data[13] * coeffs[2];
  /*03*/ C[3] = m.data[5] * coeffs[4] - m.data[9] * coeffs[2] - m.data[1] * coeffs[1]; /* negated */

  /* determinant */
  float det = m.data[0] * C[0] + m.data[4] * C[1] +
              m.data[8] * C[2] + m.data[12] * C[3];
  return det;
}

dd_mat4_t
dd_mat4_mul(dd_mat4_t a, dd_mat4_t b)
{
  dd_mat4_t o;
  o.data[0] = b.data[0] * a.data[0] + b.data[1] * a.data[4] + b.data[2] * a.data[8] + b.data[3] * a.data[12];
  o.data[1] = b.data[0] * a.data[1] + b.data[1] * a.data[5] + b.data[2] * a.data[9] + b.data[3] * a.data[13];
  o.data[2] = b.data[0] * a.data[2] + b.data[1] * a.data[6] + b.data[2] * a.data[10] + b.data[3] * a.data[14];
  o.data[3] = b.data[0] * a.data[3] + b.data[1] * a.data[7] + b.data[2] * a.data[11] + b.data[3] * a.data[15];

  o.data[4] = b.data[4] * a.data[0] + b.data[5] * a.data[4] + b.data[6] * a.data[8] + b.data[7] * a.data[12];
  o.data[5] = b.data[4] * a.data[1] + b.data[5] * a.data[5] + b.data[6] * a.data[9] + b.data[7] * a.data[13];
  o.data[6] = b.data[4] * a.data[2] + b.data[5] * a.data[6] + b.data[6] * a.data[10] + b.data[7] * a.data[14];
  o.data[7] = b.data[4] * a.data[3] + b.data[5] * a.data[7] + b.data[6] * a.data[11] + b.data[7] * a.data[15];

  o.data[8] = b.data[8] * a.data[0] + b.data[9] * a.data[4] + b.data[10] * a.data[8] + b.data[11] * a.data[12];
  o.data[9] = b.data[8] * a.data[1] + b.data[9] * a.data[5] + b.data[10] * a.data[9] + b.data[11] * a.data[13];
  o.data[10] = b.data[8] * a.data[2] + b.data[9] * a.data[6] + b.data[10] * a.data[10] + b.data[11] * a.data[14];
  o.data[11] = b.data[8] * a.data[3] + b.data[9] * a.data[7] + b.data[10] * a.data[11] + b.data[11] * a.data[15];

  o.data[12] = b.data[12] * a.data[0] + b.data[13] * a.data[4] + b.data[14] * a.data[8] + b.data[15] * a.data[12];
  o.data[13] = b.data[12] * a.data[1] + b.data[13] * a.data[5] + b.data[14] * a.data[9] + b.data[15] * a.data[13];
  o.data[14] = b.data[12] * a.data[2] + b.data[13] * a.data[6] + b.data[14] * a.data[10] + b.data[15] * a.data[14];
  o.data[15] = b.data[12] * a.data[3] + b.data[13] * a.data[7] + b.data[14] * a.data[11] + b.data[15] * a.data[15];
  return o;
}

dd_mat3_t
dd_mat4_to_mat3(dd_mat4_t m)
{
  dd_mat3_t o;
  o.data[0] = m.data[0];
  o.data[1] = m.data[1];
  o.data[2] = m.data[2];
  o.data[3] = m.data[4];
  o.data[4] = m.data[5];
  o.data[5] = m.data[6];
  o.data[6] = m.data[8];
  o.data[7] = m.data[9];
  o.data[8] = m.data[10];
  return o;
}

dd_vec3_t
dd_mat4_vec3_mul(dd_mat4_t m, dd_vec3_t v, int32_t is_point)
{
  dd_vec3_t o;
  o.x = m.data[0] * v.x + m.data[4] * v.y + m.data[8] * v.z + (float)is_point * m.data[12];
  o.y = m.data[1] * v.x + m.data[5] * v.y + m.data[9] * v.z + (float)is_point * m.data[13];
  o.z = m.data[2] * v.x + m.data[6] * v.y + m.data[10] * v.z + (float)is_point * m.data[14];
  return o;
}

dd_vec4_t
dd_mat4_vec4_mul(dd_mat4_t m, dd_vec4_t v)
{
  dd_vec4_t o;
  o.x = m.data[0] * v.x + m.data[4] * v.y + m.data[8] * v.z + m.data[12] * v.w;
  o.y = m.data[1] * v.x + m.data[5] * v.y + m.data[9] * v.z + m.data[13] * v.w;
  o.z = m.data[2] * v.x + m.data[6] * v.y + m.data[10] * v.z + m.data[14] * v.w;
  o.w = m.data[3] * v.x + m.data[7] * v.y + m.data[11] * v.z + m.data[15] * v.w;
  return o;
}

dd_mat4_t
dd_mat4_transpose(dd_mat4_t m)
{
  dd_mat4_t mt;
  mt.data[0] = m.data[0];
  mt.data[1] = m.data[4];
  mt.data[2] = m.data[8];
  mt.data[3] = m.data[12];
  mt.data[4] = m.data[1];
  mt.data[5] = m.data[5];
  mt.data[6] = m.data[9];
  mt.data[7] = m.data[13];
  mt.data[8] = m.data[2];
  mt.data[9] = m.data[6];
  mt.data[10] = m.data[10];
  mt.data[11] = m.data[14];
  mt.data[12] = m.data[3];
  mt.data[13] = m.data[7];
  mt.data[14] = m.data[11];
  mt.data[15] = m.data[15];
  return mt;
}

dd_mat4_t
dd_mat4_se3_inverse(dd_mat4_t m)
{
  dd_vec3_t t = dd_vec4_to_vec3(m.col[3]);
  m.col[3] = dd_vec4(0.0f, 0.0f, 0.0f, 1.0f);
  m = dd_mat4_transpose(m);
  t = dd_vec3_invert(dd_mat4_vec3_mul(m, t, 0));
  m.col[3] = dd_vec3_to_vec4(t);
  m.col[3].w = 1.0f;
  return m;
}

dd_mat4_t
dd_mat4_inverse(dd_mat4_t m)
{
  /* Calulate cofactor matrix */
  float C[16];
  float dets[6];

  /* dets are determinants of 2x2 matrices */
  dets[0] = m.data[10] * m.data[15] - m.data[14] * m.data[11];
  dets[1] = m.data[6] * m.data[11] - m.data[10] * m.data[7];
  dets[2] = m.data[2] * m.data[7] - m.data[6] * m.data[3];
  dets[3] = m.data[6] * m.data[15] - m.data[14] * m.data[7];
  dets[4] = m.data[2] * m.data[11] - m.data[10] * m.data[3];
  dets[5] = m.data[2] * m.data[15] - m.data[14] * m.data[3];

  /* Cofactor matrix */
  /*00*/ C[0] = m.data[5] * dets[0] - m.data[9] * dets[3] + m.data[13] * dets[1];
  /*01*/ C[1] = m.data[9] * dets[5] - m.data[1] * dets[0] - m.data[13] * dets[4]; /* negated */
  /*02*/ C[2] = m.data[1] * dets[3] - m.data[5] * dets[5] + m.data[13] * dets[2];
  /*03*/ C[3] = m.data[5] * dets[4] - m.data[9] * dets[2] - m.data[1] * dets[1]; /* negated */

  /*10*/ C[4] = m.data[8] * dets[3] - m.data[4] * dets[0] - m.data[12] * dets[1]; /* negated */
  /*11*/ C[5] = m.data[0] * dets[0] - m.data[8] * dets[5] + m.data[12] * dets[4];
  /*12*/ C[6] = m.data[4] * dets[5] - m.data[0] * dets[3] - m.data[12] * dets[2]; /* negated */
  /*13*/ C[7] = m.data[0] * dets[1] - m.data[4] * dets[4] + m.data[8] * dets[2];

  /* dets are determinants of 2x2 matrices */
  dets[0] = m.data[8] * m.data[13] - m.data[12] * m.data[9];
  dets[1] = m.data[4] * m.data[9] - m.data[8] * m.data[5];
  dets[2] = m.data[0] * m.data[5] - m.data[4] * m.data[1];
  dets[3] = m.data[4] * m.data[13] - m.data[12] * m.data[5];
  dets[4] = m.data[0] * m.data[9] - m.data[8] * m.data[1];
  dets[5] = m.data[0] * m.data[13] - m.data[12] * m.data[1];

  /* actual coefficient matrix */
  /*20*/ C[8] = m.data[7] * dets[0] - m.data[11] * dets[3] + m.data[15] * dets[1];
  /*21*/ C[9] = m.data[11] * dets[5] - m.data[3] * dets[0] - m.data[15] * dets[4]; /* negated */
  /*22*/ C[10] = m.data[3] * dets[3] - m.data[7] * dets[5] + m.data[15] * dets[2];
  /*23*/ C[11] = m.data[7] * dets[4] - m.data[3] * dets[1] - m.data[11] * dets[2]; /* negated */

  /*30*/ C[12] = m.data[10] * dets[3] - m.data[6] * dets[0] - m.data[14] * dets[1]; /* negated */
  /*31*/ C[13] = m.data[2] * dets[0] - m.data[10] * dets[5] + m.data[14] * dets[4];
  /*32*/ C[14] = m.data[6] * dets[5] - m.data[2] * dets[3] - m.data[14] * dets[2]; /* negated */
  /*33*/ C[15] = m.data[2] * dets[1] - m.data[6] * dets[4] + m.data[10] * dets[2];

  /* determinant */
  float det = m.data[0] * C[0] + m.data[4] * C[1] +
              m.data[8] * C[2] + m.data[12] * C[3];
  float denom = 1.0f / det;

  /* calculate inverse */
  dd_mat4_t mi;
  mi.data[0] = denom * C[0];
  mi.data[1] = denom * C[1];
  mi.data[2] = denom * C[2];
  mi.data[3] = denom * C[3];
  mi.data[4] = denom * C[4];
  mi.data[5] = denom * C[5];
  mi.data[6] = denom * C[6];
  mi.data[7] = denom * C[7];
  mi.data[8] = denom * C[8];
  mi.data[9] = denom * C[9];
  mi.data[10] = denom * C[10];
  mi.data[11] = denom * C[11];
  mi.data[12] = denom * C[12];
  mi.data[13] = denom * C[13];
  mi.data[14] = denom * C[14];
  mi.data[15] = denom * C[15];
  return mi;
}

dd_mat4_t
dd_pre_scale(dd_mat4_t m, dd_vec3_t s)
{
  dd_vec4_t s4 = dd_vec4(s.x, s.y, s.z, 1.0);
  m.col[0] = dd_vec4_mul(m.col[0], s4);
  m.col[1] = dd_vec4_mul(m.col[1], s4);
  m.col[2] = dd_vec4_mul(m.col[2], s4);
  m.col[3] = dd_vec4_mul(m.col[3], s4);
  return m;
}

dd_mat4_t
dd_pre_translate(dd_mat4_t m, dd_vec3_t t)
{
  m.col[3] = dd_vec4(m.col[3].x + t.x, m.col[3].y + t.y, m.col[3].z + t.z, 1.0f);
  return m;
}

dd_mat4_t
dd_pre_rotate(dd_mat4_t m, float angle, dd_vec3_t v)
{
  float c = cosf(angle);
  float s = sinf(angle);
  float t = 1.0f - c;

  dd_vec3_t axis = dd_vec3_normalize(v);

  dd_mat4_t rotate = dd_mat4_identity();
  rotate.data[0] = c + axis.x * axis.x * t;
  rotate.data[5] = c + axis.y * axis.y * t;
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

  return dd_mat4_mul(rotate, m);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Inflate for the default, compressed font
// By Micha Mettke(vurtun): https://gist.github.com/vurtun/760a6a2a198b706a7b1a6197aa5ac747
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef DBGDRAW_USE_DEFAULT_FONT

#define dbgdraw__infl_rev16(n) ((dbgdraw__defl_mirror[(n)&0xff] << 8) | dbgdraw__defl_mirror[((n)>>8)&0xff])
static const unsigned char dbgdraw__defl_mirror[256] = {
    #define R2(n) n, n + 128, n + 64, n + 192
    #define R4(n) R2(n), R2(n + 32), R2(n + 16), R2(n + 48)
    #define R6(n) R4(n), R4(n +  8), R4(n +  4), R4(n + 12)
    R6(0), R6(2), R6(1), R6(3),
};

typedef struct dbgdraw__infl
{
  int bits, bitcnt;
  unsigned lits[288];
  unsigned dsts[32];
  unsigned lens[19];
  int tlit, tdist, tlen;
} dbgdraw_infl;

static int
dbgdraw__infl_get(const unsigned char **src, const unsigned char *end,
                  struct dbgdraw__infl *s, int n)
{
  const unsigned char *in = *src;
  int v = s->bits & ((1 << n) - 1);
  s->bits >>= n;
  s->bitcnt = s->bitcnt - n;
  s->bitcnt = s->bitcnt < 0 ? 0 : s->bitcnt;
  while (s->bitcnt < 16 && in < end)
  {
    s->bits |= (*in++) << s->bitcnt;
    s->bitcnt += 8;
  }
  *src = in;
  return v;
}
static int
dbgdraw__infl_build(unsigned *tree, unsigned char *lens, int symcnt)
{
  int n, cnt[16], first[16], codes[16];
  DBGDRAW_MEMSET(cnt, 0, sizeof(cnt));
  cnt[0] = first[0] = codes[0] = 0;
  for (n = 0; n < symcnt; ++n) { cnt[lens[n]]++; }

  for (n = 1; n <= 15; n++)
  {
    codes[n] = (codes[n - 1] + cnt[n - 1]) << 1;
    first[n] = first[n - 1] + cnt[n - 1];
  }
  
  for (n = 0; n < symcnt; n++)
  {
    int slot, code, len = lens[n];
    if (!len)
      continue;
    code = codes[len]++;
    slot = first[len]++;
    tree[slot] = (unsigned)((code << (32 - len)) | (n << 4) | len);
  }
  
  return first[15];
}

int32_t
dbgdraw__infl_decode(const unsigned char **in, const unsigned char *end,
                     struct dbgdraw__infl *s, unsigned *tree, int max)
{
  /* bsearch next prefix code */
  unsigned key, lo = 0, hi = (unsigned)max;
  unsigned search = (unsigned)(dbgdraw__infl_rev16(s->bits) << 16) | 0xffff;
  while (lo < hi)
  {
    unsigned guess = (lo + hi) / 2;
    if (search < tree[guess])
      hi = guess;
    else
      lo = guess + 1;
  }
  /* pull out and check key */
  key = tree[lo - 1];
  DBGDRAW_ASSERT(((search ^ key) >> (32 - (key & 0xf))) == 0);
  dbgdraw__infl_get(in, end, s, key & 0x0f);
  return (key >> 4) & 0x0fff;
}

int32_t
dbgdraw__inflate(unsigned char *out, const unsigned char *in, int size)
{
  static const char order[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
  static const short dbase[30 + 2] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
                                      257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
  static const unsigned char dbits[30 + 2] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,
                                              10, 10, 11, 11, 12, 12, 13, 13, 0, 0};
  static const short lbase[29 + 2] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35,
                                      43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
  static const unsigned char lbits[29 + 2] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4,
                                              4, 4, 4, 5, 5, 5, 5, 0, 0, 0};

  const unsigned char *e = in + size, *o = out;
  enum dbgdraw__infl_states
  {
    hdr,
    stored,
    fixed,
    dyn,
    blk
  };
  enum dbgdraw__infl_states state = hdr;
  struct dbgdraw__infl s;
  int last = 0;

  DBGDRAW_MEMSET(&s, 0, sizeof(s));
  dbgdraw__infl_get(&in, e, &s, 0); /* buffer input */
  while (in < e || s.bitcnt)
  {
    switch (state)
    {
    case hdr:
    {
      int type = 0; /* block header */
      last = dbgdraw__infl_get(&in, e, &s, 1);
      type = dbgdraw__infl_get(&in, e, &s, 2);

      switch (type)
      {
      default:
        return (int)(out - o);
      case 0x00:
        state = stored;
        break;
      case 0x01:
        state = fixed;
        break;
      case 0x02:
        state = dyn;
        break;
      }
    }
    break;
    case stored:
    {
      int len, nlen; /* uncompressed block */
      dbgdraw__infl_get(&in, e, &s, s.bitcnt & 7);
      len = dbgdraw__infl_get(&in, e, &s, 16);
      nlen = dbgdraw__infl_get(&in, e, &s, 16);
      in -= 2;
      s.bitcnt = 0;

      if (len > (e - in) || !len)
        return (int)(out - o);
      memcpy(out, in, (size_t)len);
      in += len, out += len;
      state = hdr;
    }
    break;
    case fixed:
    {
      /* fixed huffman codes */
      int n;
      unsigned char lens[288 + 32];
      for (n = 0; n <= 143; n++)
        lens[n] = 8;
      for (n = 144; n <= 255; n++)
        lens[n] = 9;
      for (n = 256; n <= 279; n++)
        lens[n] = 7;
      for (n = 280; n <= 287; n++)
        lens[n] = 8;
      for (n = 0; n < 32; n++)
        lens[288 + n] = 5;

      /* build trees */
      s.tlit = dbgdraw__infl_build(s.lits, lens, 288);
      s.tdist = dbgdraw__infl_build(s.dsts, lens + 288, 32);
      state = blk;
    }
    break;
    case dyn:
    {
      /* dynamic huffman codes */
      int n, i, nlit, ndist, nlen;
      unsigned char nlens[19] = {0}, lens[288 + 32];
      nlit = 257 + dbgdraw__infl_get(&in, e, &s, 5);
      ndist = 1 + dbgdraw__infl_get(&in, e, &s, 5);
      nlen = 4 + dbgdraw__infl_get(&in, e, &s, 4);
      for (n = 0; n < nlen; n++)
        nlens[order[n]] = (unsigned char)dbgdraw__infl_get(&in, e, &s, 3);
      s.tlen = dbgdraw__infl_build(s.lens, nlens, 19);

      /* decode code lengths */
      for (n = 0; n < nlit + ndist;)
      {
        int sym = dbgdraw__infl_decode(&in, e, &s, s.lens, s.tlen);
        switch (sym)
        {
        default:
          lens[n++] = (unsigned char)sym;
          break;
        case 16:
          for (i = 3 + dbgdraw__infl_get(&in, e, &s, 2); i; i--, n++)
            lens[n] = lens[n - 1];
          break;
        case 17:
          for (i = 3 + dbgdraw__infl_get(&in, e, &s, 3); i; i--, n++)
            lens[n] = 0;
          break;
        case 18:
          for (i = 11 + dbgdraw__infl_get(&in, e, &s, 7); i; i--, n++)
            lens[n] = 0;
          break;
        }
      }
      /* build lit/dist trees */
      s.tlit = dbgdraw__infl_build(s.lits, lens, nlit);
      s.tdist = dbgdraw__infl_build(s.dsts, lens + nlit, ndist);
      state = blk;
    }
    break;
    case blk:
    {
      /* decompress block */
      int sym = dbgdraw__infl_decode(&in, e, &s, s.lits, s.tlit);
      if (sym > 256)
      {
        sym -= 257; /* match symbol */
        {
          int len = dbgdraw__infl_get(&in, e, &s, lbits[sym]) + lbase[sym];
          int dsym = dbgdraw__infl_decode(&in, e, &s, s.dsts, s.tdist);
          int offs = dbgdraw__infl_get(&in, e, &s, dbits[dsym]) + dbase[dsym];
          if (offs > (int)(out - o))
            return (int)(out - o);
          while (len--)
            *out = *(out - offs), out++;
        }
      }
      else if (sym == 256)
      {
        if (last)
          return (int)(out - o);
        state = hdr;
      }
      else
        *out++ = (unsigned char)sym;
    }
    break;
    }
  }
  return (int)(out - o);
}

/* Actual default font, compressed ProggySquare.ttf */
static unsigned char dd_proggy_square[7976] = {
99,96,100,96,96,224,97,104,96,96,102,112,240,15,214,55,234,200,46,17,
96,96,96,244,0,138,250,37,231,38,22,48,9,41,151,50,48,48,47,0,
138,5,37,151,149,40,48,128,1,203,31,32,197,148,158,83,153,86,29,199,
90,197,192,192,206,194,192,48,249,71,70,106,98,202,245,137,25,167,128,114,
103,128,216,44,3,40,192,126,136,201,25,168,23,40,207,160,146,145,91,82,
81,219,80,13,148,98,156,193,192,192,196,150,147,159,156,184,200,251,120,7,
3,3,43,200,56,150,220,196,138,2,198,85,12,64,253,140,26,64,1,133,
188,196,220,212,187,23,158,25,49,48,204,2,218,199,184,176,32,191,184,100,
217,154,230,247,12,12,115,128,238,97,189,84,80,148,90,144,201,196,40,196,
192,192,242,5,168,158,3,236,23,160,127,94,38,229,238,136,231,183,249,202,
192,204,193,0,2,219,211,203,91,64,244,222,73,43,206,49,48,252,103,96,
110,96,105,0,242,153,25,152,24,32,0,168,135,197,225,223,1,160,63,65,
226,64,121,176,73,12,72,128,9,36,194,200,8,12,39,72,8,48,49,112,
48,56,48,112,1,85,148,49,64,44,1,234,98,156,0,242,8,211,30,166,
46,160,120,63,132,102,60,202,96,4,177,134,133,147,1,43,112,204,41,41,
6,154,197,160,176,6,98,16,75,3,40,74,152,27,64,174,97,110,160,20,
50,130,77,99,0,249,171,1,196,6,154,221,192,0,134,8,22,3,84,4,
42,222,0,81,205,128,76,195,85,192,117,49,160,232,98,0,235,129,144,32,
27,193,152,1,20,104,40,102,51,64,196,97,110,66,55,21,110,38,35,216,
110,72,24,32,187,26,98,59,35,40,28,193,62,66,184,20,44,3,137,54,
132,233,160,56,129,217,193,192,8,97,33,220,205,128,112,31,68,142,145,1,
28,90,13,112,191,52,96,248,17,85,4,157,7,113,23,46,53,8,155,129,
46,4,218,133,37,116,24,208,109,4,153,200,204,0,1,50,16,239,49,248,
0,243,42,40,8,100,24,88,24,12,128,82,28,192,20,8,74,98,245,12,
255,21,214,252,255,15,86,220,8,102,1,45,185,127,21,170,11,44,204,200,
198,48,156,65,35,113,158,219,200,192,216,187,131,241,127,43,84,245,6,6,
202,225,45,70,38,198,22,198,95,76,25,76,119,152,190,48,91,48,215,48,
239,99,254,194,34,196,98,192,226,196,210,196,242,141,213,141,117,22,235,27,
54,63,182,13,236,98,236,49,236,167,56,12,56,252,56,42,56,54,113,124,
226,212,225,204,226,252,193,149,195,245,140,219,132,123,25,15,15,79,2,207,
49,94,51,222,26,222,67,124,26,124,121,252,12,252,117,252,47,4,188,4,
246,9,26,8,118,8,94,19,178,17,154,36,44,37,92,37,124,68,68,77,
164,66,100,139,40,155,168,155,104,142,104,157,232,13,49,59,177,38,177,87,
226,118,226,93,226,191,36,34,36,122,36,174,72,234,73,166,73,222,146,210,
147,106,146,122,37,29,36,221,39,125,73,70,70,166,64,102,131,172,132,108,
132,236,1,57,62,185,24,185,89,114,207,228,197,128,176,65,190,65,126,134,
252,7,5,9,5,61,133,42,133,91,10,159,20,195,20,119,41,222,83,74,
80,58,162,108,161,188,64,133,67,69,78,197,76,37,65,165,75,101,159,202,
29,85,22,85,37,213,20,213,67,170,207,212,18,212,78,168,107,168,151,0,
225,60,245,15,26,54,26,75,52,57,52,141,52,103,105,174,209,114,209,90,
163,245,74,59,72,187,64,71,72,199,74,39,66,103,153,46,159,110,153,110,
151,238,59,189,48,189,34,189,46,189,101,122,183,244,37,244,27,244,223,24,
132,24,204,48,100,48,204,48,188,96,100,97,180,198,88,193,120,147,137,148,
73,147,201,43,211,32,211,93,102,74,102,93,102,159,204,163,204,15,89,56,
88,172,176,20,176,172,176,124,96,229,97,53,201,234,155,117,140,245,17,27,
29,155,73,54,191,108,195,108,143,216,105,216,245,217,253,179,207,179,191,228,
224,228,176,203,145,207,177,200,241,154,147,139,211,26,167,39,206,114,206,49,
206,83,156,255,185,100,184,28,115,85,115,237,114,253,226,22,228,86,231,246,
198,221,199,125,141,135,144,71,145,199,45,79,31,207,63,12,10,240,90,129,
153,129,157,129,155,129,159,65,152,65,156,65,154,65,158,65,153,65,157,65,
155,65,159,193,152,193,156,193,154,193,158,193,153,193,157,193,155,193,159,33,
152,33,156,33,154,33,158,33,153,33,157,33,155,33,159,161,152,161,156,161,
154,161,158,129,65,208,212,88,212,16,7,102,5,138,43,2,49,53,104,92,
118,52,64,192,159,134,6,166,134,6,50,72,136,118,22,134,6,170,1,96,
46,99,3,150,175,192,202,150,25,212,202,64,132,47,3,35,48,44,216,209,
48,51,144,207,136,108,55,51,154,83,254,3,249,12,96,51,25,152,128,126,
4,54,44,144,227,12,108,166,49,52,140,145,105,70,144,49,255,64,6,131,
9,102,52,239,49,48,72,128,12,5,182,12,128,213,46,137,105,0,171,157,
216,226,7,20,239,232,110,35,70,29,35,220,217,127,161,206,254,139,71,4,
221,107,13,196,2,6,6,81,134,134,255,13,204,12,104,97,138,63,31,128,
125,15,242,25,178,79,240,249,148,80,40,176,66,82,192,127,112,188,35,60,
10,230,255,135,199,34,36,36,128,124,114,188,11,138,106,97,72,124,163,165,
73,220,126,101,16,6,186,75,17,41,101,33,179,145,99,21,27,27,164,22,
38,14,98,3,115,39,3,40,119,54,0,91,2,13,127,193,62,109,248,135,
66,253,5,53,149,26,32,89,152,161,1,61,19,224,143,82,6,17,6,112,
235,145,132,120,4,199,162,33,212,119,138,104,180,33,146,207,209,125,194,138,
36,7,75,1,32,159,130,242,219,63,176,247,254,129,125,2,202,117,64,49,
152,183,64,116,67,3,48,18,129,105,27,234,113,146,35,18,152,83,25,129,
37,0,19,34,189,50,32,151,40,208,82,4,97,44,3,3,55,35,176,245,
207,4,106,253,99,47,231,225,105,153,29,26,2,104,165,147,44,212,124,48,
205,4,204,44,160,40,131,36,84,120,140,16,46,58,137,115,7,138,93,232,
165,36,40,212,97,110,100,132,164,27,152,3,96,249,132,40,119,128,218,155,
204,192,230,36,3,129,240,0,197,40,44,253,194,98,25,150,143,97,226,160,
152,132,56,2,28,161,176,228,12,174,136,112,166,87,6,6,78,60,110,128,
199,39,200,46,152,189,200,246,131,194,0,81,58,54,52,128,203,118,38,28,
182,1,187,137,160,40,3,183,241,97,254,101,80,5,198,52,44,116,65,182,
64,67,19,45,82,25,64,93,84,96,255,9,220,101,68,132,21,184,76,64,
119,23,196,118,70,36,71,48,48,129,154,235,112,123,225,118,34,197,28,3,
176,47,9,41,123,27,176,198,5,74,72,176,35,165,78,228,148,0,18,7,
123,254,63,60,93,34,88,4,210,3,180,204,96,32,167,204,48,132,186,7,
189,220,64,78,53,200,108,108,229,11,200,12,112,120,128,138,11,88,201,0,
41,228,27,16,20,35,74,153,65,70,137,193,192,192,7,46,27,9,249,19,
94,18,192,252,102,140,148,78,144,211,11,122,236,3,93,8,10,243,127,200,
105,16,202,33,202,185,192,182,39,168,236,38,28,15,232,105,15,86,94,136,
178,66,99,131,21,41,149,160,187,18,230,186,255,224,50,236,63,180,36,3,
137,18,25,166,12,212,113,39,208,93,178,200,101,54,200,205,200,110,133,185,
19,30,156,144,164,64,172,35,5,73,168,7,177,198,56,200,61,184,82,43,
178,59,89,161,169,3,152,251,224,177,255,15,92,239,65,170,56,104,10,110,
0,73,54,144,144,108,129,229,142,16,3,113,233,1,210,55,193,90,34,177,
34,165,93,36,87,163,164,24,152,47,97,242,144,144,135,185,27,204,131,120,
5,92,174,147,224,133,6,160,243,73,244,3,40,69,192,220,129,195,237,162,
216,98,6,221,15,176,242,4,154,138,208,74,22,96,78,37,199,47,60,4,
227,3,107,28,224,43,179,97,169,252,63,106,177,77,48,144,129,189,84,82,
210,134,55,3,56,141,35,135,44,174,180,77,74,232,162,150,201,212,8,100,
80,146,97,160,48,205,160,251,12,61,109,24,34,229,9,144,111,97,161,130,
173,14,130,165,20,112,60,145,147,102,88,160,253,95,68,223,18,94,159,51,
67,221,193,8,75,4,208,214,11,180,196,99,96,96,131,180,89,152,81,251,
165,24,250,97,233,11,169,77,1,76,78,168,134,65,172,96,0,181,53,24,
64,125,10,102,236,125,93,120,42,129,133,11,148,150,133,230,75,24,45,10,
108,1,131,123,45,255,160,45,80,38,152,31,48,104,6,240,28,3,35,112,
60,150,169,1,71,91,83,159,1,107,190,1,197,11,114,121,0,139,39,16,
13,181,230,47,204,122,6,100,123,255,163,240,192,237,43,6,208,104,58,14,
63,131,122,119,64,51,49,252,136,22,6,162,80,43,64,94,6,97,188,94,
6,181,233,64,214,177,16,8,103,144,95,64,246,40,66,211,2,136,205,10,
101,131,199,67,64,94,1,218,6,78,35,255,225,149,53,206,242,1,228,117,
6,41,96,152,51,144,53,158,1,26,211,194,91,82,128,220,11,107,245,195,
218,117,184,248,32,63,129,212,67,253,40,139,30,151,200,249,13,218,229,133,
116,140,65,1,253,7,22,161,127,192,61,201,6,88,75,144,164,234,167,1,
13,0,195,69,132,164,58,21,52,222,135,55,60,96,113,7,162,97,126,69,
242,179,40,182,178,8,71,57,3,173,112,33,85,45,50,73,178,151,65,209,
47,70,178,63,65,227,154,24,249,16,217,127,216,202,81,108,242,216,212,129,
35,2,146,142,225,94,3,138,161,137,48,194,162,153,172,88,38,165,125,143,
28,87,172,72,245,1,59,130,141,61,189,194,210,33,34,101,17,229,86,114,
250,88,208,114,73,148,80,122,162,36,14,24,145,250,91,200,108,160,247,72,
79,118,194,36,166,57,82,90,172,162,232,177,132,158,219,32,17,242,23,30,
47,255,144,43,1,18,253,194,64,126,31,7,28,91,200,110,197,151,210,112,
186,153,72,247,2,221,73,65,91,9,228,50,244,80,133,149,217,216,210,20,
178,79,32,109,13,120,104,131,184,200,69,22,36,240,73,10,117,160,95,200,
74,63,216,74,88,144,235,209,211,7,54,31,33,235,5,86,51,24,121,1,
20,63,127,177,73,0,179,10,233,73,138,129,7,52,22,4,28,131,100,192,
221,14,194,213,255,65,42,151,68,145,125,7,142,7,148,246,14,152,67,208,
113,224,117,31,12,196,187,133,29,123,25,137,233,150,6,36,0,78,5,132,
157,194,64,82,95,29,121,44,30,61,238,97,45,17,88,220,227,138,115,240,
24,60,98,28,154,1,50,0,15,110,119,255,131,15,92,131,83,3,19,168,
225,216,64,44,96,128,172,167,97,192,59,143,2,46,245,112,133,39,72,28,
61,229,98,179,157,136,80,101,160,180,29,8,155,207,68,46,19,64,110,67,
230,19,211,6,68,142,37,244,24,3,241,193,25,12,220,242,3,102,43,156,
45,64,70,240,172,9,35,184,161,136,60,185,73,98,62,68,13,76,6,6,
10,218,73,48,191,192,194,4,150,47,97,226,176,176,1,137,35,179,65,242,
32,61,176,212,137,204,134,21,54,224,201,161,6,112,90,132,22,178,8,10,
86,216,50,192,202,43,178,130,128,178,186,3,87,206,194,39,14,242,39,8,
51,34,250,52,184,10,87,136,56,113,229,7,114,140,50,64,203,18,226,198,
178,25,72,105,97,193,92,15,139,45,88,14,6,91,15,116,41,35,182,218,
3,82,15,146,56,236,7,106,43,2,231,98,27,168,57,30,15,75,147,184,
104,168,239,224,109,94,210,226,168,161,129,244,36,8,76,127,100,244,197,40,
137,49,108,126,39,20,123,104,177,74,178,55,25,4,136,110,19,51,160,247,
143,89,33,117,46,114,63,4,204,134,165,60,144,60,114,170,4,199,25,106,
255,4,82,104,16,237,106,6,208,28,9,144,32,56,118,64,112,173,11,174,
250,13,154,89,255,33,55,206,137,76,61,100,228,109,66,169,158,64,110,16,
133,149,198,152,121,27,77,132,212,146,138,164,112,38,228,11,88,171,7,70,
131,194,30,177,4,8,99,186,29,220,204,33,174,149,8,138,45,6,138,214,
199,128,218,16,200,238,199,198,134,213,141,200,254,128,137,193,210,56,136,15,
43,121,145,205,128,250,147,17,214,44,128,51,32,9,13,62,138,4,29,40,
68,174,111,200,170,53,193,33,2,201,211,196,148,207,12,248,90,171,48,255,
96,243,23,172,30,133,181,84,129,237,208,191,224,214,0,40,246,64,145,247,
15,214,54,0,201,48,52,144,210,78,5,182,253,9,229,113,6,244,120,131,
185,7,68,195,226,9,57,143,163,167,55,112,134,128,247,75,8,6,53,3,
116,204,159,216,58,15,235,200,129,33,180,143,2,114,35,58,6,185,21,36,
143,142,27,96,224,63,210,116,40,220,221,164,85,107,12,124,192,53,38,13,
120,214,152,64,214,18,50,16,211,199,67,14,91,152,155,33,117,114,3,22,
64,228,154,61,96,172,19,90,107,32,140,84,231,0,221,0,171,111,208,105,
81,116,71,16,90,98,64,78,216,176,227,232,115,178,35,197,51,74,216,160,
59,10,92,210,145,22,54,140,184,251,110,224,120,3,217,141,156,7,96,121,
1,185,76,98,4,185,3,156,134,192,14,0,102,5,70,34,90,234,192,200,
97,7,174,69,109,0,173,124,71,27,31,96,16,53,197,189,178,180,1,43,
0,173,59,97,6,150,10,44,144,57,23,144,219,193,113,8,74,67,208,16,
97,128,181,77,152,113,143,71,192,214,190,34,167,89,176,57,172,104,46,2,
241,145,195,0,93,30,156,118,161,235,220,144,11,97,72,67,0,247,84,10,
138,231,64,233,151,140,57,113,88,122,65,206,251,216,220,139,207,253,112,119,
32,199,230,95,212,246,65,3,137,205,96,6,232,124,50,51,145,227,65,32,
247,179,162,230,9,89,172,115,42,168,109,64,2,129,203,0,43,123,137,93,
255,131,146,15,40,9,83,176,195,32,9,224,47,142,150,22,68,150,164,154,
154,1,62,118,75,48,93,51,160,151,196,176,20,96,136,37,117,3,83,17,
74,104,51,162,244,98,65,233,227,47,52,145,16,149,154,65,105,153,151,136,
190,1,3,182,218,130,21,187,251,68,97,41,29,92,2,129,195,14,82,149,
253,131,37,95,34,66,18,232,48,17,80,93,136,103,109,32,230,154,120,6,
244,116,72,106,254,130,149,171,176,144,135,148,23,248,147,69,3,164,60,33,
178,240,104,128,3,82,250,100,194,72,249,141,212,180,14,74,77,112,75,241,
148,27,140,36,148,27,12,28,224,49,100,108,121,21,28,7,176,120,96,71,
45,39,192,41,3,104,207,127,148,126,23,40,53,160,138,64,157,203,0,30,
199,252,79,196,248,48,62,251,208,227,148,9,62,125,141,28,42,16,207,99,
117,6,92,89,3,137,243,49,48,223,163,151,232,176,218,26,22,147,232,169,
20,98,33,180,182,198,53,42,76,100,105,4,12,67,78,156,113,165,140,88,
143,131,45,166,96,98,140,168,221,228,6,188,9,133,1,56,110,4,180,20,
184,107,144,112,125,14,219,203,130,210,143,71,239,99,33,135,30,177,108,72,
8,254,5,119,199,24,81,71,108,25,49,250,104,16,17,18,243,47,3,124,
29,39,129,242,157,164,209,61,100,255,129,253,128,63,191,18,225,102,226,221,
137,94,194,163,167,90,116,62,74,25,137,203,157,224,210,159,56,103,130,250,
94,192,157,177,12,68,180,3,65,233,134,172,112,69,47,59,65,41,156,80,
56,55,144,83,192,3,131,157,84,255,80,163,238,2,249,7,146,91,9,213,
89,248,23,197,55,160,3,144,127,184,32,237,4,28,241,3,142,15,228,188,
11,10,107,88,154,65,46,93,96,225,13,155,91,0,243,241,36,17,216,28,
33,51,137,243,149,64,251,101,145,235,5,228,214,10,35,180,213,4,245,38,
49,169,148,129,129,23,84,142,18,216,155,4,174,253,96,190,197,150,218,160,
114,152,109,56,120,176,64,221,212,64,84,35,137,15,111,156,192,246,13,50,
224,27,255,193,149,175,65,174,199,209,30,134,36,46,112,168,53,16,151,185,
137,72,59,232,238,128,213,146,48,26,20,114,72,238,1,219,14,110,81,130,
43,109,124,9,136,1,212,183,0,18,68,215,73,12,228,140,23,130,194,11,
189,238,34,48,46,136,50,28,72,68,64,194,146,6,3,108,93,31,190,188,
8,11,79,228,240,3,229,1,24,31,36,15,14,79,112,72,130,3,241,63,
162,51,65,48,60,27,72,45,171,209,227,23,23,31,57,215,128,226,28,57,
223,18,145,30,201,43,173,73,200,71,134,104,189,30,152,27,65,238,132,97,
116,53,176,136,251,143,52,188,7,18,35,42,243,240,130,52,17,24,207,131,
247,124,88,209,90,220,64,62,184,172,97,71,136,195,203,30,80,255,236,31,
146,211,96,204,6,226,6,171,192,117,2,35,48,29,48,225,234,179,163,148,
134,236,88,250,2,48,49,70,184,213,72,12,2,5,32,3,81,225,34,12,
137,45,172,97,128,37,172,192,235,166,144,220,240,31,214,244,5,133,19,145,
193,194,1,58,213,1,52,234,197,128,190,167,13,165,205,130,156,254,97,233,
5,100,51,35,172,189,10,78,236,232,1,3,12,116,200,190,81,6,226,247,
141,226,106,217,33,167,82,108,41,23,36,6,115,37,76,45,35,162,136,128,
148,94,192,128,129,4,210,63,68,97,66,226,102,81,80,209,12,76,71,13,
224,35,78,32,97,134,178,43,14,61,117,0,213,243,131,246,81,2,235,98,
220,107,185,33,99,134,24,249,130,21,146,30,80,86,51,98,75,153,32,117,
192,158,34,100,63,31,220,131,136,216,0,10,17,57,164,219,0,218,76,192,
6,25,147,103,68,109,195,128,125,9,43,143,145,105,72,40,131,108,251,135,
158,0,192,198,1,83,22,120,223,5,3,204,255,12,32,221,48,140,172,5,
24,86,160,124,210,64,74,187,5,61,140,64,113,207,138,37,247,66,227,5,
234,68,36,151,18,211,55,5,207,163,18,189,215,27,94,146,224,114,27,49,
110,102,68,10,208,127,208,173,154,112,138,180,81,83,6,112,154,5,230,65,
164,114,25,236,70,144,59,64,241,0,180,11,156,106,224,137,4,50,190,9,
140,7,34,243,45,70,255,6,22,11,132,234,81,67,180,20,14,73,13,127,
113,87,157,32,5,36,228,88,248,190,24,66,121,15,186,31,12,84,250,192,
210,54,43,246,220,135,220,26,134,151,212,232,106,193,121,226,63,108,240,230,
47,34,145,67,152,196,231,70,144,78,80,252,177,130,199,69,26,80,246,131,
192,227,16,104,59,200,37,96,215,48,193,106,109,240,52,104,3,50,0,153,
3,61,183,129,232,242,24,113,118,7,214,56,198,23,207,176,184,5,133,13,
122,58,64,143,119,16,31,234,84,200,230,133,127,224,145,15,8,217,0,218,
243,207,8,30,91,2,145,80,117,164,149,219,136,112,128,143,221,147,219,119,
199,231,103,100,191,26,146,147,182,73,105,88,139,130,51,54,3,11,113,105,
27,114,14,11,74,239,10,230,19,116,151,26,66,203,79,144,111,208,49,54,
181,13,160,238,49,168,72,253,7,107,129,52,64,210,254,127,68,98,4,70,
0,105,169,30,18,99,32,47,130,231,175,152,26,200,45,139,176,165,63,92,
229,18,178,239,192,14,64,78,135,88,217,64,85,164,164,196,225,232,31,96,
70,98,2,166,0,248,218,59,112,143,20,216,78,1,151,71,160,80,68,84,
177,160,182,0,80,45,98,205,2,74,93,9,43,53,145,212,3,247,243,97,
59,183,6,215,24,1,36,37,34,149,29,232,181,59,40,252,105,97,38,39,
232,132,50,38,212,242,25,118,38,18,3,122,155,22,150,202,88,145,74,8,
176,223,193,205,18,48,193,4,207,73,13,72,128,178,115,21,128,115,229,140,
13,88,218,250,40,35,218,134,88,75,173,6,44,205,90,6,22,208,14,69,
148,118,45,3,114,253,9,109,89,64,251,184,72,153,31,20,7,60,12,160,
19,142,240,245,11,48,92,5,171,151,141,145,92,8,10,65,16,31,132,33,
225,244,23,133,194,151,53,33,235,94,9,174,61,129,156,99,133,81,110,50,
162,133,19,200,37,236,16,49,89,152,28,136,143,28,199,136,114,18,236,72,
68,105,9,102,33,133,208,127,172,177,143,156,16,112,181,7,224,237,0,144,
189,32,12,203,83,255,193,11,158,26,144,1,40,30,196,64,4,73,243,31,
40,123,222,96,113,1,178,9,84,166,34,151,171,134,104,241,132,44,7,99,
99,79,109,104,115,33,13,224,115,125,24,97,13,0,32,3,58,22,69,66,
101,9,243,55,200,187,195,173,62,17,132,206,137,19,211,14,64,201,161,232,
105,24,20,27,176,52,11,75,63,172,104,233,28,164,6,150,166,254,193,66,
21,185,182,7,137,193,147,50,193,132,12,234,111,242,64,210,32,158,118,12,
60,247,129,92,140,156,142,64,174,3,165,65,16,13,114,57,12,55,64,7,
236,153,32,109,71,104,25,4,119,13,196,125,248,28,7,114,18,27,164,159,
143,214,78,4,135,32,108,31,58,178,157,40,69,36,40,105,162,155,15,50,
19,60,118,71,176,45,3,182,131,21,45,255,192,252,137,78,195,242,16,72,
61,172,39,7,46,80,16,181,9,216,37,255,137,59,24,139,1,52,86,5,
44,157,241,183,183,24,96,105,7,22,242,232,174,133,165,35,228,28,14,10,
33,176,163,96,238,105,128,28,241,65,196,129,93,195,49,223,10,65,210,61,
177,237,90,108,235,75,97,105,193,16,45,151,130,98,131,21,75,206,5,137,
53,160,102,10,104,45,244,15,121,140,129,180,22,45,27,214,113,85,112,10,
129,229,15,228,252,130,146,79,16,237,51,68,39,121,56,198,53,176,61,199,
194,0,111,107,192,203,51,112,57,5,41,141,24,24,100,32,233,129,132,61,
44,232,231,106,50,64,71,144,69,145,211,3,40,206,209,235,101,228,50,4,
38,7,18,195,165,14,38,142,158,206,32,21,0,176,58,102,130,213,209,13,
96,70,3,168,193,8,94,73,206,136,42,1,73,123,32,109,232,45,243,6,
82,1,195,112,171,199,161,243,142,56,206,252,0,231,39,244,178,31,27,31,
156,191,254,65,135,190,161,37,44,184,192,101,194,217,170,100,24,118,97,201,
202,64,254,249,115,114,20,231,67,208,249,182,4,243,162,49,180,124,70,206,
127,32,49,228,56,5,229,55,92,242,68,230,201,6,112,25,3,223,213,1,
97,48,66,207,211,4,229,185,63,224,229,21,208,38,53,197,185,18,100,34,
48,252,216,65,199,7,50,99,158,29,73,76,95,147,5,45,157,130,162,3,
216,215,100,98,64,105,55,193,219,72,176,240,98,68,164,117,228,250,139,232,
181,79,160,22,13,182,86,11,40,14,64,101,40,46,57,72,126,131,148,94,
136,38,23,81,107,86,134,95,190,27,110,101,50,19,120,196,164,1,105,79,
0,120,255,62,100,148,29,154,80,65,233,83,16,52,210,73,228,250,95,6,
98,87,184,192,82,30,172,214,133,165,116,80,74,4,215,176,127,177,146,12,
224,173,19,160,4,9,110,83,49,65,146,38,49,228,240,75,143,44,224,113,
57,208,213,21,240,51,76,145,123,46,160,144,5,245,102,65,245,35,35,34,
132,128,51,159,224,54,45,112,222,18,58,47,206,0,42,1,88,161,37,54,
19,108,98,7,162,3,81,62,33,143,103,193,123,72,176,54,48,35,180,88,
67,41,156,56,64,245,20,214,254,32,3,178,59,97,101,61,114,153,15,50,
239,31,184,112,255,7,25,240,4,186,6,107,195,29,228,62,2,109,11,80,
138,68,174,119,64,62,197,198,7,121,24,209,121,67,52,50,240,53,45,24,
72,236,231,32,231,14,116,55,32,135,3,44,119,24,99,233,251,128,212,193,
130,166,1,62,47,14,12,177,191,96,81,248,110,143,63,224,89,50,210,186,
59,12,36,159,133,45,136,165,166,71,15,97,88,30,7,137,163,199,182,34,
146,15,49,125,5,138,147,191,224,209,189,191,160,196,212,0,33,27,72,61,
5,123,248,229,125,110,2,101,50,56,135,193,122,166,160,112,7,97,80,110,
133,133,191,33,82,184,67,75,135,255,208,172,207,8,29,82,2,117,166,161,
131,24,13,216,0,40,239,141,206,95,225,237,143,226,74,241,172,104,249,26,
28,188,208,162,14,82,224,97,35,27,72,75,248,163,241,131,209,79,193,23,
31,232,37,147,33,74,201,212,0,206,20,140,240,153,122,42,197,143,8,9,
227,219,144,251,70,192,57,27,84,47,48,34,185,143,216,116,6,171,87,144,
253,10,174,187,33,141,123,72,25,219,0,26,224,128,55,191,64,73,19,185,
45,70,120,124,185,1,13,128,210,33,153,254,4,249,11,86,79,178,18,240,
175,33,90,158,66,15,19,70,68,31,10,92,205,255,67,245,45,118,127,54,
144,0,64,254,148,6,17,100,159,181,153,141,251,228,83,92,49,12,243,53,
43,1,223,195,244,99,164,106,212,9,40,72,25,212,0,189,137,2,179,68,
106,32,109,121,22,150,208,27,237,23,18,25,83,216,98,28,35,246,70,235,
12,252,25,116,180,14,28,252,117,224,104,27,114,180,13,9,44,19,209,107,
48,112,198,30,45,223,70,94,249,38,70,114,155,24,188,110,7,212,30,70,
30,83,129,181,143,145,123,188,48,54,72,29,178,90,24,91,17,90,55,35,
183,150,32,237,99,208,0,23,168,145,12,25,103,65,218,51,249,23,18,65,
136,193,23,144,122,164,97,254,255,104,35,254,120,226,147,97,180,109,52,218,
54,18,69,78,175,176,246,222,104,89,216,64,24,12,199,182,30,100,253,24,
3,158,245,252,12,216,214,206,24,34,213,167,232,253,8,208,225,112,144,210,
10,84,158,129,138,39,200,72,251,95,240,120,49,129,25,21,80,24,11,130,
8,102,226,246,189,225,28,233,71,46,125,145,75,101,24,27,57,23,128,74,
84,184,27,193,197,49,120,70,23,84,20,131,10,87,16,221,0,91,166,79,
252,120,55,3,169,107,11,193,107,95,177,245,202,208,221,140,205,15,160,24,
129,249,3,60,135,197,136,155,252,7,242,21,11,241,245,70,3,226,44,76,
162,215,73,178,66,83,136,34,137,52,172,68,98,2,143,166,252,195,54,98,
132,230,51,50,124,51,252,234,65,146,210,26,108,150,8,212,130,33,55,126,
96,115,41,140,36,36,54,226,154,42,195,110,61,13,244,76,94,34,214,42,
130,251,210,236,36,180,81,12,145,212,130,114,28,180,18,251,135,99,160,245,
31,113,203,88,65,166,0,203,96,146,239,71,32,52,98,140,92,38,99,43,
231,140,145,124,3,46,134,65,254,64,26,17,135,206,70,254,67,77,123,32,
215,146,184,244,6,154,255,153,137,221,255,5,170,101,192,165,51,146,15,225,
165,53,122,28,224,42,157,97,37,52,36,23,128,75,45,208,92,42,60,3,
65,68,160,197,50,145,173,122,72,94,33,201,31,224,53,24,148,250,3,50,
139,79,85,159,128,206,147,2,239,117,38,97,79,30,70,237,79,141,248,249,
7,175,235,27,112,158,32,73,122,255,139,129,108,255,33,231,26,138,253,7,
202,44,224,132,246,15,92,84,16,72,133,32,53,196,166,68,68,158,34,238,
60,48,108,243,76,100,231,169,6,252,222,129,20,15,196,250,132,186,115,73,
228,249,9,109,46,137,152,132,216,64,28,0,221,65,1,76,140,212,219,183,
196,138,86,95,193,202,67,244,18,30,86,46,26,162,169,135,56,27,146,235,
254,66,27,165,127,224,69,60,3,120,40,4,34,76,194,106,44,88,88,128,
186,18,252,68,174,47,99,64,202,93,224,17,83,144,15,96,43,144,128,180,
44,54,121,152,239,193,245,21,234,73,168,255,33,235,29,152,136,141,23,33,
146,198,165,176,213,73,162,200,97,142,30,206,80,223,96,248,2,228,114,112,
214,248,7,107,53,128,28,252,23,230,106,18,43,36,6,114,234,86,244,58,
137,44,127,32,213,73,212,241,9,153,229,0,40,220,97,241,192,136,214,62,
195,149,7,240,197,13,82,125,132,215,95,160,8,35,186,176,30,110,115,34,
176,179,47,177,245,79,49,114,10,82,158,70,156,97,203,0,45,114,27,136,
73,248,12,16,251,176,223,239,15,46,71,64,49,10,139,125,108,246,33,183,
160,136,178,17,120,22,24,228,28,11,28,237,35,184,173,160,246,2,62,155,
25,161,30,253,7,247,41,136,1,111,209,96,77,66,112,255,98,171,219,193,
54,19,180,21,84,132,163,90,137,167,86,134,183,107,73,104,75,32,231,59,
86,28,249,14,87,111,23,86,86,194,70,80,24,32,13,138,127,56,134,31,
254,129,60,67,82,159,131,129,188,114,17,71,219,79,20,221,175,184,252,5,
18,103,4,69,47,216,197,96,162,1,151,167,64,99,118,13,13,164,181,108,
225,103,27,19,30,23,194,200,133,196,250,1,57,110,80,235,41,180,177,7,
176,247,136,175,174,72,116,59,184,78,98,38,55,93,193,11,151,127,248,146,
20,9,142,103,32,115,76,14,57,159,146,28,3,200,37,7,49,30,105,192,
183,188,19,86,252,144,211,135,197,149,47,96,245,46,62,26,57,61,65,218,
213,68,167,42,144,147,137,168,98,25,224,119,29,16,177,31,17,163,244,36,
39,95,16,93,92,17,227,122,118,96,59,180,1,203,126,12,176,75,209,219,
192,32,215,130,196,24,161,131,51,96,10,212,228,69,182,9,196,134,247,127,
137,60,59,8,227,46,100,228,112,129,177,65,233,0,150,22,64,98,200,237,
43,100,182,33,52,223,194,91,233,32,6,180,137,11,162,128,92,36,138,168,
253,71,13,72,128,129,220,114,16,95,185,141,156,134,13,145,202,29,88,29,
69,96,124,191,129,164,194,68,128,232,190,7,56,21,128,220,6,138,117,178,
220,223,0,61,225,136,145,138,62,32,249,28,7,144,15,96,101,33,249,190,
104,32,80,155,130,171,36,68,157,74,211,178,131,220,180,68,148,23,136,30,
65,97,24,126,115,43,34,144,241,3,82,238,71,4,205,176,192,90,253,198,
208,188,203,10,165,9,197,19,114,94,7,233,1,153,3,47,107,64,9,10,
82,88,97,230,158,6,232,120,37,73,13,83,136,201,12,10,192,187,128,24,
176,236,57,149,103,80,102,80,103,128,157,47,13,57,207,7,178,78,28,125,
255,60,104,223,110,53,67,61,3,222,93,170,32,255,16,155,78,73,9,39,
228,48,131,134,21,100,107,46,25,36,68,63,82,59,184,129,82,192,0,2,
162,140,76,12,48,160,194,16,14,101,50,2,79,46,104,134,178,153,128,55,
202,195,84,48,35,137,179,32,177,89,25,68,224,170,216,192,226,140,80,45,
66,12,74,12,16,54,35,80,133,61,148,205,196,192,206,96,2,101,51,3,
91,90,214,80,54,11,146,26,86,224,41,211,190,80,113,54,176,56,51,3,
35,11,39,208,92,144,59,33,108,136,59,33,108,136,59,33,108,102,6,21,
134,106,6,8,27,226,78,8,27,226,78,8,27,226,206,32,134,84,134,116,
134,82,134,28,134,68,134,34,6,35,6,3,32,52,97,208,135,146,134,12,
166,73,149,10,33,69,153,197,37,137,121,10,238,69,153,185,185,169,69,65,
169,233,165,57,137,69,33,33,17,10,1,69,249,233,233,149,193,133,165,137,
69,169,33,33,70,6,6,38,250,64,100,104,202,144,196,80,9,76,188,33,
64,35,51,25,138,25,74,128,134,231,1,249,238,96,126,46,67,46,208,210,
34,134,16,32,140,0,138,6,0,69,243,129,142,72,7,234,9,102,40,4,
58,6,228,148,84,176,60,3,232,80,45,88,232,139,48,128,142,246,101,64,
5,140,160,208,101,100,100,98,100,102,100,97,100,101,100,99,100,103,228,96,
228,100,228,98,228,102,228,97,228,101,228,99,228,103,20,96,20,100,20,98,
20,102,20,97,20,101,20,99,20,103,148,96,148,100,148,98,148,102,148,97,
148,101,148,99,148,103,84,96,0,29,207,202,202,192,6,140,21,14,6,78,
96,184,115,3,79,189,231,5,134,57,63,176,5,33,8,140,31,97,96,252,
138,2,111,9,22,103,144,96,144,4,222,168,44,13,140,99,89,6,57,6,
121,160,243,21,129,49,172,12,12,111,85,6,53,96,158,212,96,208,100,208,
2,230,76,29,6,93,6,61,112,56,26,2,67,213,24,24,219,166,12,102,
12,230,12,22,12,150,12,86,12,214,12,54,12,182,12,118,12,246,12,14,
12,142,12,78,12,206,12,46,12,174,12,110,12,238,12,30,12,158,12,94,
12,222,12,62,12,190,12,126,12,254,12,1,12,129,12,65,12,193,192,192,
8,101,8,3,166,207,8,134,72,134,40,134,104,134,24,134,88,134,56,134,
120,134,4,134,68,70,69,70,37,70,101,70,21,70,85,70,53,70,117,70,
13,70,77,70,45,70,109,70,29,70,93,70,61,70,125,70,3,70,67,70,
35,70,99,70,19,70,83,70,51,70,115,70,11,70,75,70,43,70,107,70,
27,70,91,70,59,70,123,70,7,70,71,134,53,12,139,25,90,24,90,25,
246,50,76,99,120,193,208,198,208,199,208,205,48,151,97,37,195,18,134,247,
12,93,12,183,24,154,25,38,51,124,98,248,204,208,203,48,157,161,131,225,
48,195,61,134,143,12,243,24,86,49,124,101,248,194,240,141,97,17,195,90,
134,147,12,199,25,214,1,163,61,153,97,2,67,10,195,105,96,4,159,96,
56,197,112,158,225,12,195,89,134,115,12,47,25,210,24,46,51,92,96,184,
200,176,30,24,209,31,24,38,50,92,99,184,194,112,149,33,131,225,53,195,
91,134,78,134,44,134,76,134,108,96,178,200,97,200,99,88,192,144,15,76,
4,5,192,68,81,12,76,10,37,12,101,12,229,12,175,24,42,24,170,128,
201,163,154,161,150,161,134,97,7,195,66,96,137,86,7,108,27,53,50,188,
97,120,199,176,139,175,52,47,51,57,63,37,85,217,160,194,192,192,192,16,
149,107,132,202,53,70,229,154,160,114,77,81,185,102,168,92,115,84,174,5,
42,215,18,149,155,136,202,77,66,229,38,163,114,83,80,185,169,168,220,52,
20,174,161,1,42,23,213,191,134,168,254,53,68,245,175,33,170,127,13,81,
253,107,136,234,95,67,84,255,26,162,250,215,16,213,191,134,168,254,53,68,
245,175,33,170,127,13,81,253,107,136,234,95,195,52,182,148,212,156,212,146,
84,22,215,210,162,124,20,207,90,160,122,214,2,213,179,22,168,158,181,64,
245,172,5,170,103,45,80,61,107,129,234,89,11,84,207,90,160,122,214,2,
213,179,22,168,158,181,64,245,172,5,170,103,45,80,61,107,129,26,185,150,
168,145,107,137,234,95,75,84,255,90,162,250,215,18,213,191,150,168,254,181,
68,245,175,37,170,127,45,81,253,107,137,234,95,75,84,255,90,162,250,215,
18,213,191,150,168,254,181,68,245,175,101,26,3,3,64,0
};

#endif /* DBGDRAW_USE_DEFAULT_FONT */


#endif /* DBGDRAW_IMPLEMENTATION */

/*
LICENSE INFO
========================================================================
Dbgdraw Library:


UTF-8 Decode:
Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and 
to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/