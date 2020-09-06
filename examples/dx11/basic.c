
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

LRESULT CALLBACK
window_procedure(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
  //TODO(maciej): Switch cluses for WM_MOUSELEAVE, WM_MOUSEWHEEL, WM_MOUSEHWHEEL, WM_CHAR
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

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  // TODO(maciej): Probably need some way to store keyboard and mouse events.

  WNDCLASSW window_class;
  memset(&window_class, 0, sizeof(window_class));
  window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; /* Some window options */
  window_class.lpfnWndProc = window_procedure;             /* Window procedure handles messages that arrive in the window */
  window_class.hInstance = GetModuleHandleW(NULL);         /* Returns a handle to the .exe file */
  window_class.hCursor = LoadCursor(NULL, IDC_ARROW);      /* Default cursor for this window */
  window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);    /* Default icon */
  window_class.lpszClassName = L"debugdraw_app";           /* Name of the window class */
  RegisterClassW(&window_class);

  const DWORD window_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  const DWORD window_style_ex = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
  RECT rect = {100, 100, 1380, 820};
  AdjustWindowRectEx(&rect, window_style, FALSE, window_style_ex);
  HWND window_handle = CreateWindowExW(
                                  window_style_ex,        /* dwExStyle */
                                  L"debugdraw_app",       /* lpClassName */
                                  L"debugdraw_basic",     /* lpWindowName */
                                  window_style,           /* dwStyle */
                                  rect.left,              /* X */
                                  rect.top,               /* Y */
                                  rect.right-rect.left,   /* nWidth */
                                  rect.bottom - rect.top, /* nHeight */
                                  NULL,                   /* hWndParent */
                                  NULL,                   /* hMenu */
                                  GetModuleHandleW(NULL), /* hInstance */
                                  NULL );                 /* lpParam */

  // TODO(maciej): Create directX device and swapchain
  DXGI_SWAP_CHAIN_DESC swapchain_desc = 
  {
    .BufferDesc = 
    {
      .Width = 1280,
      .Height = 720,
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .RefreshRate.Numerator = 60,
      .RefreshRate.Denominator = 1,
    },
    .OutputWindow = window_handle,
    .Windowed = true,
    .SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
    .BufferCount = 1,
    .SampleDesc =
    {
      .Count = (UINT)4,
      .Quality = (UINT)D3D11_STANDARD_MULTISAMPLE_PATTERN
    },
    .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT
  };

  int create_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
  D3D_FEATURE_LEVEL feature_level; // This should be d3d11
  ID3D11Device* d3d11_device = NULL;
  ID3D11DeviceContext* d3d11_device_context = NULL;
  IDXGISwapChain* dxgi_swap_chain = NULL;
  HRESULT hr;
  hr = D3D11CreateDeviceAndSwapChain(NULL,                           /* pAdapter (use default) */
                                     D3D_DRIVER_TYPE_HARDWARE,       /* DriverType */
                                     NULL,                           /* Software */
                                     create_flags,                   /* Flags */
                                     NULL,                           /* pFeatureLevels */
                                     0,                              /* FeatureLevels */
                                     D3D11_SDK_VERSION,              /* SDKVersion */
                                     &swapchain_desc,                /* pSwapChainDesc */
                                     &dxgi_swap_chain,               /* ppSwapChain */
                                     &d3d11_device,                  /* ppDevice */
                                     &feature_level,                 /* pFeatureLevel */
                                     &d3d11_device_context);         /* ppImmediateContext */
  assert(SUCCEEDED(hr) && dxgi_swap_chain && d3d11_device && d3d11_device_context);
  // TODO(maciej): Create directX render target  

  ShowWindow(window_handle, SW_SHOW);

  bool should_quit = false;
  while (!should_quit)
  {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if (WM_QUIT == msg.message)
      {
        should_quit = true;
        continue;
      }
      else
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

      // TODO(maciej): Render frame

      // TODO(maciej): Swapchain present

    }
  }

  DestroyWindow(window_handle); 
  window_handle = 0;
  UnregisterClassW(L"debugdraw_app", GetModuleHandleW(NULL));
  return 0;
}