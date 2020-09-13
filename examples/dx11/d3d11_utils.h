/* This file is modelled after the d3d11entry.h from sokol_gfx, by Andre Weissflog (@floooh)*/

#ifndef D3D11_UTILS
#define D3D11_UTILS

#include <stdint.h>
#include <stdbool.h>

#define STRINGIFY(x) #x

typedef struct d3d11_desc
{
  const char* win_title;
  int32_t win_x, win_y, win_w, win_h;

  int32_t sample_count;
} d3d11_desc_t;

typedef struct d3d11
{
  HWND win_handle;
  ID3D11Device* device;
  ID3D11DeviceContext* device_context;
  IDXGISwapChain* swap_chain;

  ID3D11Texture2D* render_target;
  ID3D11Texture2D* depth_stencil;
  ID3D11RenderTargetView* render_target_view;
  ID3D11DepthStencilView* depth_stencil_view;
 
  int32_t render_target_width;
  int32_t render_target_height;
  int32_t sample_count;
} d3d11_t;

d3d11_t* d3d11_init(const d3d11_desc_t* desc);
void d3d11_terminate(d3d11_t* d3d11);

bool d3d11_process_events();
void d3d11_clear(d3d11_t* d3d11, float r, float g, float b, float a);
void d3d11_present(d3d11_t* d3d11);

ID3DBlob* d3d11_complie_shader(const char* source, const char* target);
//TODO(maciej): Upload texture?



#endif /*D3D11_UTILS*/
