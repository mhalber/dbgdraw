/* This file is modelled after the d3d11entry.c from sokol_gfx, by Andre Weissflog (@floooh)*/


// TODO(maciej): Fix mouse controls.

#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include "d3d11_app.h"

#pragma comment (lib, "user32.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include <assert.h>
#include <stdio.h>

static LRESULT CALLBACK d3d11_window_procedure(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param);
static void d3d11_create_default_render_target(d3d11_ctx_t* d3d11);
static void d3d11_update_default_render_target(d3d11_ctx_t* d3d11);
static void d3d11_destroy_default_render_target(d3d11_ctx_t* d3d11);

static d3d11_app_input_t d3d11_input = {0};
static uint16_t _d3d11_keycode_map[512];
static uint8_t _d3d11_keycode_map_initialized = 0;
void _d3d11_init_keycode_table(void);

int32_t
d3d11_init(d3d11_ctx_t* d3d11, const d3d11_ctx_desc_t* desc)
{
  assert(d3d11);
  assert(desc);

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
    .SwapEffect = DXGI_SWAP_EFFECT_DISCARD, //DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
    .BufferCount = 2,
    .SampleDesc =
    {
      .Count = (UINT)d3d11->sample_count,
      .Quality = (d3d11->sample_count > 1) ? (UINT)D3D11_STANDARD_MULTISAMPLE_PATTERN : 0,
    },
    .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT
  };

  //D3D11_CREATE_DEVICE_DEBUG
  int32_t create_flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_DEBUG;
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
  if (!SUCCEEDED(hr))
  {
    return 1;
  }

  d3d11_create_default_render_target(d3d11);
  _d3d11_init_keycode_table();
  return 0;
}

void
d3d11_terminate(d3d11_ctx_t* d3d11)
{
  d3d11_destroy_default_render_target(d3d11);
  IDXGISwapChain_Release(d3d11->swap_chain);
  ID3D11DeviceContext_Release(d3d11->device_context);
  ID3D11Device_Release(d3d11->device);
  DestroyWindow(d3d11->win_handle);
  UnregisterClassW( L"dbgdraw_win32_win_class", GetModuleHandleW(NULL));
}

bool 
d3d11_process_events(d3d11_app_input_t** input) {
  if (*input == NULL)
  {
    *input = &d3d11_input;
  }
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
d3d11_clear(d3d11_ctx_t* d3d11, float r, float g, float b, float a)
{
  FLOAT clear_color[4] = {r,g,b,a};
  ID3D11DeviceContext_ClearRenderTargetView(d3d11->device_context, d3d11->render_target_view, clear_color);
  UINT depth_stencil_flags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
  ID3D11DeviceContext_ClearDepthStencilView(d3d11->device_context, d3d11->depth_stencil_view, depth_stencil_flags, 1.0f, 0);
}

void d3d11_viewport(d3d11_ctx_t* d3d11, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  D3D11_VIEWPORT vp = {(FLOAT)x, (FLOAT)y, (FLOAT)w, (FLOAT)h, 0.0f, 1.0f};
  ID3D11DeviceContext_OMSetRenderTargets(d3d11->device_context, 1, &d3d11->render_target_view, d3d11->depth_stencil_view);
  ID3D11DeviceContext_RSSetViewports(d3d11->device_context, 1, &vp);
}

void
d3d11_present(d3d11_ctx_t* d3d11)
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

static LRESULT CALLBACK 
d3d11_window_procedure(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
  int32_t keycode;
  int32_t z_delta;
  switch (message)
  {
    case WM_CLOSE:
      PostQuitMessage(0);
      break;
    case WM_KEYDOWN:
      if (w_param == VK_ESCAPE) { PostQuitMessage(0); }
      keycode = _d3d11_keycode_map[(int)(HIWORD(l_param)&0x1FF)];
      d3d11_input.keys[keycode] = 1;
      break;
    case WM_KEYUP:
      keycode = _d3d11_keycode_map[(int)(HIWORD(l_param)&0x1FF)];
      d3d11_input.keys[keycode] = 0;
      break;
    case WM_LBUTTONDOWN:
      d3d11_input.mouse.buttons[D3D11_APP_MOUSE_BUTTON_LEFT] = 1;
      break;
    case WM_LBUTTONUP:
      d3d11_input.mouse.buttons[D3D11_APP_MOUSE_BUTTON_LEFT] = 0;
      break;
    case WM_RBUTTONDOWN:
      d3d11_input.mouse.buttons[D3D11_APP_MOUSE_BUTTON_RIGHT] = 1;
      break;
    case WM_RBUTTONUP:
      d3d11_input.mouse.buttons[D3D11_APP_MOUSE_BUTTON_RIGHT] = 0;
      break;
    case WM_MBUTTONDOWN:
      d3d11_input.mouse.buttons[D3D11_APP_MOUSE_BUTTON_MIDDLE] = 1;
      break;
    case WM_MBUTTONUP:
      d3d11_input.mouse.buttons[D3D11_APP_MOUSE_BUTTON_MIDDLE] = 0;
      break;
    case WM_MOUSEMOVE:
      d3d11_input.mouse.prev_x = d3d11_input.mouse.cur_x;
      d3d11_input.mouse.prev_y = d3d11_input.mouse.cur_y;
      d3d11_input.mouse.cur_x = (float)GET_X_LPARAM(l_param); 
      d3d11_input.mouse.cur_y = (float)GET_Y_LPARAM(l_param);
      break;
    case WM_MOUSEWHEEL:
      z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
      d3d11_input.mouse.scroll_y = (float)(z_delta / abs(z_delta));
      break;
    default:
      break;
  }
  uint32_t mods = 0;
  if (GetKeyState(VK_SHIFT) & (1<<31))    { mods |= D3D11_APP_MODIFIER_SHIFT; }
  if (GetKeyState(VK_CONTROL) & (1<<31))  { mods |= D3D11_APP_MODIFIER_CTRL; }
  if (GetKeyState(VK_MENU) & (1<<31))     { mods |= D3D11_APP_MODIFIER_ALT; }
  if (GetKeyState(VK_LWIN) & (1<<31))     { mods |= D3D11_APP_MODIFIER_SUPER; }
  if (GetKeyState(VK_RWIN) & (1<<31))     { mods |= D3D11_APP_MODIFIER_SUPER; }
  d3d11_input.mods = mods;
  return DefWindowProcW(window_handle, message, w_param, l_param);
}

void
d3d11_create_default_render_target(d3d11_ctx_t* d3d11)
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
      .Quality = (d3d11->sample_count > 1) ? (UINT)D3D11_STANDARD_MULTISAMPLE_PATTERN : 0
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
d3d11_destroy_default_render_target(d3d11_ctx_t* d3d11)
{
  assert(d3d11);
  if (d3d11->render_target) { ID3D11Texture2D_Release(d3d11->render_target); d3d11->render_target = NULL; }
  if (d3d11->depth_stencil) { ID3D11Texture2D_Release(d3d11->depth_stencil); d3d11->depth_stencil = NULL; }
  if (d3d11->render_target_view) { ID3D11RenderTargetView_Release(d3d11->render_target_view); d3d11->render_target_view = NULL; }
  if (d3d11->depth_stencil_view) { ID3D11DepthStencilView_Release(d3d11->depth_stencil_view); d3d11->depth_stencil_view = NULL; }
}

void
d3d11_update_default_render_target(d3d11_ctx_t* d3d11)
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

void _d3d11_init_keycode_table(void)
{
  if (_d3d11_keycode_map_initialized) {return;}
  _d3d11_keycode_map_initialized = 1;
  /* same as GLFW */
  _d3d11_keycode_map[0x00B] = D3D11_APP_KEYCODE_0;
  _d3d11_keycode_map[0x002] = D3D11_APP_KEYCODE_1;
  _d3d11_keycode_map[0x003] = D3D11_APP_KEYCODE_2;
  _d3d11_keycode_map[0x004] = D3D11_APP_KEYCODE_3;
  _d3d11_keycode_map[0x005] = D3D11_APP_KEYCODE_4;
  _d3d11_keycode_map[0x006] = D3D11_APP_KEYCODE_5;
  _d3d11_keycode_map[0x007] = D3D11_APP_KEYCODE_6;
  _d3d11_keycode_map[0x008] = D3D11_APP_KEYCODE_7;
  _d3d11_keycode_map[0x009] = D3D11_APP_KEYCODE_8;
  _d3d11_keycode_map[0x00A] = D3D11_APP_KEYCODE_9;
  _d3d11_keycode_map[0x01E] = D3D11_APP_KEYCODE_A;
  _d3d11_keycode_map[0x030] = D3D11_APP_KEYCODE_B;
  _d3d11_keycode_map[0x02E] = D3D11_APP_KEYCODE_C;
  _d3d11_keycode_map[0x020] = D3D11_APP_KEYCODE_D;
  _d3d11_keycode_map[0x012] = D3D11_APP_KEYCODE_E;
  _d3d11_keycode_map[0x021] = D3D11_APP_KEYCODE_F;
  _d3d11_keycode_map[0x022] = D3D11_APP_KEYCODE_G;
  _d3d11_keycode_map[0x023] = D3D11_APP_KEYCODE_H;
  _d3d11_keycode_map[0x017] = D3D11_APP_KEYCODE_I;
  _d3d11_keycode_map[0x024] = D3D11_APP_KEYCODE_J;
  _d3d11_keycode_map[0x025] = D3D11_APP_KEYCODE_K;
  _d3d11_keycode_map[0x026] = D3D11_APP_KEYCODE_L;
  _d3d11_keycode_map[0x032] = D3D11_APP_KEYCODE_M;
  _d3d11_keycode_map[0x031] = D3D11_APP_KEYCODE_N;
  _d3d11_keycode_map[0x018] = D3D11_APP_KEYCODE_O;
  _d3d11_keycode_map[0x019] = D3D11_APP_KEYCODE_P;
  _d3d11_keycode_map[0x010] = D3D11_APP_KEYCODE_Q;
  _d3d11_keycode_map[0x013] = D3D11_APP_KEYCODE_R;
  _d3d11_keycode_map[0x01F] = D3D11_APP_KEYCODE_S;
  _d3d11_keycode_map[0x014] = D3D11_APP_KEYCODE_T;
  _d3d11_keycode_map[0x016] = D3D11_APP_KEYCODE_U;
  _d3d11_keycode_map[0x02F] = D3D11_APP_KEYCODE_V;
  _d3d11_keycode_map[0x011] = D3D11_APP_KEYCODE_W;
  _d3d11_keycode_map[0x02D] = D3D11_APP_KEYCODE_X;
  _d3d11_keycode_map[0x015] = D3D11_APP_KEYCODE_Y;
  _d3d11_keycode_map[0x02C] = D3D11_APP_KEYCODE_Z;
  _d3d11_keycode_map[0x028] = D3D11_APP_KEYCODE_APOSTROPHE;
  _d3d11_keycode_map[0x02B] = D3D11_APP_KEYCODE_BACKSLASH;
  _d3d11_keycode_map[0x033] = D3D11_APP_KEYCODE_COMMA;
  _d3d11_keycode_map[0x00D] = D3D11_APP_KEYCODE_EQUAL;
  _d3d11_keycode_map[0x029] = D3D11_APP_KEYCODE_GRAVE_ACCENT;
  _d3d11_keycode_map[0x01A] = D3D11_APP_KEYCODE_LEFT_BRACKET;
  _d3d11_keycode_map[0x00C] = D3D11_APP_KEYCODE_MINUS;
  _d3d11_keycode_map[0x034] = D3D11_APP_KEYCODE_PERIOD;
  _d3d11_keycode_map[0x01B] = D3D11_APP_KEYCODE_RIGHT_BRACKET;
  _d3d11_keycode_map[0x027] = D3D11_APP_KEYCODE_SEMICOLON;
  _d3d11_keycode_map[0x035] = D3D11_APP_KEYCODE_SLASH;
  _d3d11_keycode_map[0x056] = D3D11_APP_KEYCODE_WORLD_2;
  _d3d11_keycode_map[0x00E] = D3D11_APP_KEYCODE_BACKSPACE;
  _d3d11_keycode_map[0x153] = D3D11_APP_KEYCODE_DELETE;
  _d3d11_keycode_map[0x14F] = D3D11_APP_KEYCODE_END;
  _d3d11_keycode_map[0x01C] = D3D11_APP_KEYCODE_ENTER;
  _d3d11_keycode_map[0x001] = D3D11_APP_KEYCODE_ESCAPE;
  _d3d11_keycode_map[0x147] = D3D11_APP_KEYCODE_HOME;
  _d3d11_keycode_map[0x152] = D3D11_APP_KEYCODE_INSERT;
  _d3d11_keycode_map[0x15D] = D3D11_APP_KEYCODE_MENU;
  _d3d11_keycode_map[0x151] = D3D11_APP_KEYCODE_PAGE_DOWN;
  _d3d11_keycode_map[0x149] = D3D11_APP_KEYCODE_PAGE_UP;
  _d3d11_keycode_map[0x045] = D3D11_APP_KEYCODE_PAUSE;
  _d3d11_keycode_map[0x146] = D3D11_APP_KEYCODE_PAUSE;
  _d3d11_keycode_map[0x039] = D3D11_APP_KEYCODE_SPACE;
  _d3d11_keycode_map[0x00F] = D3D11_APP_KEYCODE_TAB;
  _d3d11_keycode_map[0x03A] = D3D11_APP_KEYCODE_CAPS_LOCK;
  _d3d11_keycode_map[0x145] = D3D11_APP_KEYCODE_NUM_LOCK;
  _d3d11_keycode_map[0x046] = D3D11_APP_KEYCODE_SCROLL_LOCK;
  _d3d11_keycode_map[0x03B] = D3D11_APP_KEYCODE_F1;
  _d3d11_keycode_map[0x03C] = D3D11_APP_KEYCODE_F2;
  _d3d11_keycode_map[0x03D] = D3D11_APP_KEYCODE_F3;
  _d3d11_keycode_map[0x03E] = D3D11_APP_KEYCODE_F4;
  _d3d11_keycode_map[0x03F] = D3D11_APP_KEYCODE_F5;
  _d3d11_keycode_map[0x040] = D3D11_APP_KEYCODE_F6;
  _d3d11_keycode_map[0x041] = D3D11_APP_KEYCODE_F7;
  _d3d11_keycode_map[0x042] = D3D11_APP_KEYCODE_F8;
  _d3d11_keycode_map[0x043] = D3D11_APP_KEYCODE_F9;
  _d3d11_keycode_map[0x044] = D3D11_APP_KEYCODE_F10;
  _d3d11_keycode_map[0x057] = D3D11_APP_KEYCODE_F11;
  _d3d11_keycode_map[0x058] = D3D11_APP_KEYCODE_F12;
  _d3d11_keycode_map[0x064] = D3D11_APP_KEYCODE_F13;
  _d3d11_keycode_map[0x065] = D3D11_APP_KEYCODE_F14;
  _d3d11_keycode_map[0x066] = D3D11_APP_KEYCODE_F15;
  _d3d11_keycode_map[0x067] = D3D11_APP_KEYCODE_F16;
  _d3d11_keycode_map[0x068] = D3D11_APP_KEYCODE_F17;
  _d3d11_keycode_map[0x069] = D3D11_APP_KEYCODE_F18;
  _d3d11_keycode_map[0x06A] = D3D11_APP_KEYCODE_F19;
  _d3d11_keycode_map[0x06B] = D3D11_APP_KEYCODE_F20;
  _d3d11_keycode_map[0x06C] = D3D11_APP_KEYCODE_F21;
  _d3d11_keycode_map[0x06D] = D3D11_APP_KEYCODE_F22;
  _d3d11_keycode_map[0x06E] = D3D11_APP_KEYCODE_F23;
  _d3d11_keycode_map[0x076] = D3D11_APP_KEYCODE_F24;
  _d3d11_keycode_map[0x038] = D3D11_APP_KEYCODE_LEFT_ALT;
  _d3d11_keycode_map[0x01D] = D3D11_APP_KEYCODE_LEFT_CONTROL;
  _d3d11_keycode_map[0x02A] = D3D11_APP_KEYCODE_LEFT_SHIFT;
  _d3d11_keycode_map[0x15B] = D3D11_APP_KEYCODE_LEFT_SUPER;
  _d3d11_keycode_map[0x137] = D3D11_APP_KEYCODE_PRINT_SCREEN;
  _d3d11_keycode_map[0x138] = D3D11_APP_KEYCODE_RIGHT_ALT;
  _d3d11_keycode_map[0x11D] = D3D11_APP_KEYCODE_RIGHT_CONTROL;
  _d3d11_keycode_map[0x036] = D3D11_APP_KEYCODE_RIGHT_SHIFT;
  _d3d11_keycode_map[0x15C] = D3D11_APP_KEYCODE_RIGHT_SUPER;
  _d3d11_keycode_map[0x150] = D3D11_APP_KEYCODE_DOWN;
  _d3d11_keycode_map[0x14B] = D3D11_APP_KEYCODE_LEFT;
  _d3d11_keycode_map[0x14D] = D3D11_APP_KEYCODE_RIGHT;
  _d3d11_keycode_map[0x148] = D3D11_APP_KEYCODE_UP;
  _d3d11_keycode_map[0x052] = D3D11_APP_KEYCODE_KP_0;
  _d3d11_keycode_map[0x04F] = D3D11_APP_KEYCODE_KP_1;
  _d3d11_keycode_map[0x050] = D3D11_APP_KEYCODE_KP_2;
  _d3d11_keycode_map[0x051] = D3D11_APP_KEYCODE_KP_3;
  _d3d11_keycode_map[0x04B] = D3D11_APP_KEYCODE_KP_4;
  _d3d11_keycode_map[0x04C] = D3D11_APP_KEYCODE_KP_5;
  _d3d11_keycode_map[0x04D] = D3D11_APP_KEYCODE_KP_6;
  _d3d11_keycode_map[0x047] = D3D11_APP_KEYCODE_KP_7;
  _d3d11_keycode_map[0x048] = D3D11_APP_KEYCODE_KP_8;
  _d3d11_keycode_map[0x049] = D3D11_APP_KEYCODE_KP_9;
  _d3d11_keycode_map[0x04E] = D3D11_APP_KEYCODE_KP_ADD;
  _d3d11_keycode_map[0x053] = D3D11_APP_KEYCODE_KP_DECIMAL;
  _d3d11_keycode_map[0x135] = D3D11_APP_KEYCODE_KP_DIVIDE;
  _d3d11_keycode_map[0x11C] = D3D11_APP_KEYCODE_KP_ENTER;
  _d3d11_keycode_map[0x037] = D3D11_APP_KEYCODE_KP_MULTIPLY;
  _d3d11_keycode_map[0x04A] = D3D11_APP_KEYCODE_KP_SUBTRACT;
}