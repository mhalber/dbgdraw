/* This file is modelled after the d3d11entry.c from sokol_gfx, by Andre Weissflog (@floooh)*/

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

#include "d3d11_window.h"

#include <assert.h>
#include <stdio.h>

static LRESULT CALLBACK d3d11_window_procedure(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param);
static void d3d11_create_default_render_target(d3d11_t* d3d11);
static void d3d11_update_default_render_target(d3d11_t* d3d11);
static void d3d11_destroy_default_render_target(d3d11_t* d3d11);

d3d11_t*
d3d11_init(const d3d11_desc_t* desc)
{
  d3d11_t* d3d11 = calloc(1, sizeof(d3d11_t));

  d3d11->sample_count = desc->sample_count;

  HRESULT hr;
  
  // Register the window class
  WNDCLASSW win_class =
  {
    .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC, /* Some window options */
    .lpfnWndProc = d3d11_window_procedure,       /* Window procedure handles messages that arrive in the window */
    .hInstance = GetModuleHandleW(NULL),         /* Returns a handle to the .exe file */
    .hCursor = LoadCursor(NULL, IDC_ARROW),      /* Default cursor for this window */
    .hIcon = LoadIcon(NULL, IDI_APPLICATION),    /* Default icon */
    .lpszClassName = L"dbgdraw_win32_win_class"  /* Name of the window class */
  };
  RegisterClassW(&win_class);

  // Create window
  const DWORD win_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX;
  const DWORD win_style_ex = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
  RECT win_rect = {.left=desc->win_x, .top=desc->win_y, .right=desc->win_x+desc->win_w, .bottom=desc->win_y+desc->win_h};
  AdjustWindowRectEx(&win_rect, win_style, FALSE, win_style_ex);
  d3d11->render_target_width = win_rect.right - win_rect.left;
  d3d11->render_target_height = win_rect.bottom - win_rect.top;

  int32_t win_title_len = (int32_t)strlen(desc->win_title);
  wchar_t win_title_wide[512] = {0};
  assert(win_title_len < 512);
  for (int32_t i = 0; i < win_title_len; ++i) { win_title_wide[i] = desc->win_title[i]; }

  d3d11->win_handle = CreateWindowExW(win_style_ex,                  /* dwExStyle */
                                     L"dbgdraw_win32_win_class",     /* lpClassName */
                                     win_title_wide,                 /* lpWindowName */
                                     win_style,                      /* dwStyle */
                                     desc->win_x,                    /* X */
                                     desc->win_y,                    /* Y */
                                     d3d11->render_target_width,     /* nWidth */
                                     d3d11->render_target_height,    /* nHeight */
                                     NULL,                           /* hWndParent */
                                     NULL,                           /* hMenu */
                                     GetModuleHandleW(NULL),         /* hInstance */
                                     NULL);                          /* lpParam */
  ShowWindow(d3d11->win_handle, SW_SHOW);

  // Create Swap chain
  DXGI_SWAP_CHAIN_DESC swapchain_desc = 
  {
    .BufferDesc = 
    {
      .Width = d3d11->render_target_width,
      .Height = d3d11->render_target_height,
      .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
      .RefreshRate.Numerator = 0,
      .RefreshRate.Denominator = 1,
    },
    .OutputWindow = d3d11->win_handle,
    .Windowed = true,
    .SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
    .BufferCount = 1,
    .SampleDesc =
    {
      .Count = (UINT)d3d11->sample_count,
      .Quality = d3d11->sample_count > 1 ? (UINT)D3D11_STANDARD_MULTISAMPLE_PATTERN : 0,
    },
    .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT
  };

  //D3D11_CREATE_DEVICE_DEBUG
  int32_t create_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
  D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_1 };
  hr = D3D11CreateDeviceAndSwapChain(NULL,                           /* pAdapter (use default) */
                                     D3D_DRIVER_TYPE_HARDWARE,       /* DriverType */
                                     NULL,                           /* Software */
                                     create_flags,                   /* Flags */
                                     NULL,                           /* pFeatureLevels */
                                     0,                              /* FeatureLevels */
                                     D3D11_SDK_VERSION,              /* SDKVersion */
                                     &swapchain_desc,                /* pSwapChainDesc */
                                     &d3d11->swap_chain,             /* ppSwapChain */
                                     &d3d11->device,                 /* ppDevice */
                                     feature_levels,                 /* pFeatureLevel */
                                     &d3d11->device_context);        /* ppImmediateContext */
  assert(SUCCEEDED(hr));

  d3d11_create_default_render_target(d3d11);

  return d3d11;
}

void
d3d11_terminate(d3d11_t* d3d11)
{
  d3d11_destroy_default_render_target(d3d11);
  IDXGISwapChain_Release(d3d11->swap_chain);
  ID3D11DeviceContext_Release(d3d11->device_context);
  ID3D11Device_Release(d3d11->device);
  DestroyWindow(d3d11->win_handle);
  UnregisterClassW( L"dbgdraw_win32_win_class", GetModuleHandleW(NULL));
  free(d3d11);
}

bool d3d11_process_events() {
  static BOOL quit_requested = FALSE;
  MSG msg;
  while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (WM_QUIT == msg.message) {
          quit_requested = true;
      }
      else {
          TranslateMessage(&msg);
          DispatchMessageW(&msg);
      }
  } 
  return !quit_requested;
}

void
d3d11_clear(d3d11_t* d3d11, float r, float g, float b, float a)
{
  FLOAT clear_color[4] = {r,g,b,a};
  ID3D11DeviceContext_ClearRenderTargetView(d3d11->device_context, d3d11->render_target_view, clear_color);
  UINT depth_stencil_flags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
  ID3D11DeviceContext_ClearDepthStencilView(d3d11->device_context, d3d11->depth_stencil_view, depth_stencil_flags, 1.0f, 0);
}

void d3d11_viewport(d3d11_t* d3d11, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  D3D11_VIEWPORT vp = {(FLOAT)x, (FLOAT)y, (FLOAT)w, (FLOAT)h, 0.0f, 1.0f};
  ID3D11DeviceContext_OMSetRenderTargets(d3d11->device_context, 1, &d3d11->render_target_view, d3d11->depth_stencil_view);
  ID3D11DeviceContext_RSSetViewports(d3d11->device_context, 1, &vp);
}

void
d3d11_present(d3d11_t* d3d11)
{
  IDXGISwapChain_Present(d3d11->swap_chain, 1, 0);
  RECT win_rect;
  if (GetClientRect(d3d11->win_handle, &win_rect))
  {
    const int32_t new_width = win_rect.right - win_rect.left;
    const int32_t new_height = win_rect.bottom - win_rect.top;
    if (new_width != d3d11->render_target_width ||
        new_height != d3d11->render_target_height )
    {
      d3d11->render_target_width = new_width;
      d3d11->render_target_height = new_height;
      d3d11_update_default_render_target(d3d11);
    }
  }
}

static LRESULT CALLBACK d3d11_window_procedure(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
  switch (message)
  {
    case WM_CLOSE:
      PostQuitMessage(0);
      break;
    default:
      break;
  }
  return DefWindowProcW(window_handle, message, w_param, l_param);
}

void
d3d11_create_default_render_target(d3d11_t* d3d11)
{
  HRESULT hr;

  hr = IDXGISwapChain_GetBuffer(d3d11->swap_chain, 0, &IID_ID3D11Texture2D, (void**)&d3d11->render_target);
  assert(SUCCEEDED(hr));
  hr = ID3D11Device_CreateRenderTargetView(d3d11->device, (ID3D11Resource*)d3d11->render_target, 
                                           NULL, &d3d11->render_target_view);
  assert(SUCCEEDED(hr));

  D3D11_TEXTURE2D_DESC depth_stencil_desc = 
  {
    .Width = d3d11->render_target_width,
    .Height = d3d11->render_target_height,
    .MipLevels = 1,
    .ArraySize = 1,
    .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
    .SampleDesc =
    {
      .Count = (UINT)d3d11->sample_count,
      .Quality = d3d11->sample_count > 1 ? (UINT)D3D11_STANDARD_MULTISAMPLE_PATTERN : 0
    },
    .Usage = D3D11_USAGE_DEFAULT,
    .BindFlags = D3D11_BIND_DEPTH_STENCIL
  };

  hr = ID3D11Device_CreateTexture2D(d3d11->device, &depth_stencil_desc, NULL, &d3d11->depth_stencil); 
  assert(SUCCEEDED(hr));

  D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = 
  {
    .Format = depth_stencil_desc.Format,
    .ViewDimension = d3d11->sample_count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D
  };
  hr = ID3D11Device_CreateDepthStencilView(d3d11->device, (ID3D11Resource*)d3d11->depth_stencil, 
                                           &depth_stencil_view_desc, &d3d11->depth_stencil_view);
  assert(SUCCEEDED(hr));
}

void
d3d11_destroy_default_render_target(d3d11_t* d3d11)
{
  assert(d3d11);
  if (d3d11->render_target) { ID3D11Texture2D_Release(d3d11->render_target); d3d11->render_target = NULL; }
  if (d3d11->depth_stencil) { ID3D11Texture2D_Release(d3d11->depth_stencil); d3d11->depth_stencil = NULL; }
  if (d3d11->render_target_view) { ID3D11RenderTargetView_Release(d3d11->render_target_view); d3d11->render_target_view = NULL; }
  if (d3d11->depth_stencil_view) { ID3D11DepthStencilView_Release(d3d11->depth_stencil_view); d3d11->depth_stencil_view = NULL; }
}

void
d3d11_update_default_render_target(d3d11_t* d3d11)
{
  assert(d3d11);
  if (d3d11->swap_chain)
  {
    d3d11_destroy_default_render_target(d3d11);
    int32_t w = d3d11->render_target_width;
    int32_t h = d3d11->render_target_height;
    IDXGISwapChain_ResizeBuffers(d3d11->swap_chain, 1, w, h, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    d3d11_create_default_render_target(d3d11);
  }
}