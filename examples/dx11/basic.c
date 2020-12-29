
#define MSH_VEC_MATH_INCLUDE_LIBC_HEADERS
#define MSH_VEC_MATH_IMPLEMENTATION
#include "msh_vec_math.h"

#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#define DBGDRAW_USE_DEFAULT_FONT
#include "stb_truetype.h"
#include "dbgdraw.h"
#include "d3d11_window.h"
#include "dbgdraw_d3d11.h"

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;
  
  int32_t error = 0;

  int32_t w = 1280;
  int32_t h = 720;
  float fovy = 1.0472f;
  d3d11_desc_t d3d11_info = 
  {
    .win_title = "TestApp",
    .win_x = 100,
    .win_y = 100,
    .win_w = w,
    .win_h = h,
    .sample_count = 1
  };
  d3d11_t* d3d11 = d3d11_init(&d3d11_info); // NOTE(maciej): This function should return opaque resource, like a decorated void ptr.

  dd_ctx_desc_t desc = 
  { 
    .max_vertices = 32,
    .max_commands = 16,
    .detail_level = 2,
    .enable_frustum_cull = false,
    .enable_depth_test = true,
    .enable_default_font = true,
    .line_antialias_radius = 2.0
  };
  
  dd_render_backend_t* backend = calloc(1, sizeof(dd_render_backend_t));
  backend->d3d11 = d3d11;
  dd_ctx_t dd_ctx = {0};
  dd_ctx.render_backend = backend;
  error = dd_init(&dd_ctx, &desc);
  if(error)
  {
    fprintf(stderr, "[ERROR] Failed to initialize dbgdraw library!\n");
    return 1;
  }



  while (d3d11_process_events())
  {
    // Begin default pass? How does sokol share that between _app and _gfx?
    D3D11_VIEWPORT vp = {0.0f, 0.0f, (FLOAT)d3d11->render_target_width, (FLOAT)d3d11->render_target_height, 0.0f, 1.0f};
    ID3D11DeviceContext_OMSetRenderTargets(d3d11->device_context, 1, &d3d11->render_target_view, d3d11->depth_stencil_view);
    ID3D11DeviceContext_RSSetViewports(d3d11->device_context, 1, &vp);
    d3d11_clear(d3d11, 0.2f, 0.2f, 0.2f, 1.0f);

    msh_vec3_t cam_pos = msh_vec3(0.8f, 2.6f, 3.0f);
    msh_mat4_t view = msh_look_at(cam_pos, msh_vec3_zeros(), msh_vec3_posy());
    msh_vec4_t viewport = msh_vec4(0.0f, 0.0f, (float)w, (float)h);
    msh_mat4_t proj = msh_perspective(fovy, (float)w/h, 0.1f, 10.0f);
    
    dd_new_frame_info_t frame_info = { 
      .view_matrix       = view.data,
      .projection_matrix = proj.data,
      .viewport_size     = viewport.data,
      .vertical_fov      = fovy,
      .projection_type   = DBGDRAW_PERSPECTIVE };

    msh_vec3_t zero_pt = msh_vec3_zeros();

    dd_instance_data_t instances[] = 
    {
      [0] = { .position = { 1.0, 0.0,  1.0}, .color = DBGDRAW_CYAN },  
      [1] = { .position = {-1.0, 0.0,  1.0}, .color = DBGDRAW_MAGENTA },  
      [2] = { .position = {-1.0, 0.0, -1.0}, .color = DBGDRAW_YELLOW },  
      [3] = { .position = { 1.0, 0.0, -1.0}, .color = DBGDRAW_GRAY },  
    };

    dd_new_frame(&dd_ctx, &frame_info);

    dd_begin_cmd(&dd_ctx, DBGDRAW_MODE_STROKE);
    dd_set_color(&dd_ctx, DBGDRAW_ORANGE);
    dd_set_primitive_size(&dd_ctx, 16.0);
    
    dd_circle(&dd_ctx, zero_pt.data, 1.2f);

    dd_end_cmd(&dd_ctx);


    dd_set_detail_level(&dd_ctx, 2);
    dd_set_shading_type(&dd_ctx, DBGDRAW_SHADING_SOLID);
    dd_begin_cmd(&dd_ctx, DBGDRAW_MODE_FILL);
    dd_set_instance_data(&dd_ctx, sizeof(instances)/sizeof(instances[0]), instances);
    dd_set_color(&dd_ctx, DBGDRAW_BLACK);
    dd_sphere(&dd_ctx, zero_pt.data, 0.05f);
    dd_end_cmd(&dd_ctx);


    dd_set_detail_level(&dd_ctx, 2);
    dd_set_primitive_size(&dd_ctx, 6.0f);
    dd_set_shading_type(&dd_ctx, DBGDRAW_SHADING_SOLID);
    dd_begin_cmd(&dd_ctx, DBGDRAW_MODE_POINT);
    dd_set_instance_data(&dd_ctx, sizeof(instances)/sizeof(instances[0]), instances);
    dd_set_color(&dd_ctx, DBGDRAW_BLACK);
    dd_circle2d(&dd_ctx, zero_pt.data, 0.1f);
    dd_end_cmd(&dd_ctx);

    dd_text_info_t text_info = {.vert_align = DBGDRAW_TEXT_TOP, .horz_align=DBGDRAW_TEXT_CENTER};
    dd_set_shading_type(&dd_ctx, DBGDRAW_SHADING_NONE);
    dd_begin_cmd(&dd_ctx, DBGDRAW_MODE_FILL);
    dd_set_instance_data(&dd_ctx, sizeof(instances)/sizeof(instances[0]), instances);
    dd_text_line(&dd_ctx, zero_pt.data, "This is some text", &text_info);
    dd_end_cmd(&dd_ctx);

    dd_render(&dd_ctx);

    d3d11_present(d3d11);
  }

  d3d11_terminate(d3d11);
  return 0;
}