
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

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

const char* vs_src = 
 "struct vs_in {\n"
            "  float4 pos: POS;\n"
            "  float4 color: COLOR;\n"
            "};\n"
            "struct vs_out {\n"
            "  float4 color: COLOR0;\n"
            "  float4 pos: SV_Position;\n"
            "};\n"
            "vs_out main(vs_in inp) {\n"
            "  vs_out outp;\n"
            "  outp.pos = inp.pos;\n"
            "  outp.color = inp.color;\n"
            "  return outp;\n"
            "}\n";

const char* ps_src = 
            "float4 main(float4 color: COLOR0): SV_Target0 {\n"
            "  return color;\n"
            "}\n";


ID3DBlob*
d3d11_complie_shader(const char* src, const char* target)
{
  ID3DBlob* output = NULL;
  ID3DBlob* errors = NULL;
  D3DCompile( src, strlen(src), NULL, NULL, NULL, "main", target, D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &output, &errors);
  if( errors )
  {
    printf("%s\n", (char*)ID3D10Blob_GetBufferPointer(errors));
    ID3D10Blob_Release(errors); errors = NULL;
    return NULL;
  }
  return output;
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
  ID3D11Texture2D* d3d11_render_target = NULL;
  ID3D11RenderTargetView* d3d11_render_target_view = NULL;
  hr = IDXGISwapChain_GetBuffer(dxgi_swap_chain, 0, &IID_ID3D11Texture2D, (void**)&d3d11_render_target);
  assert(SUCCEEDED(hr));
  hr = ID3D11Device_CreateRenderTargetView(d3d11_device, (ID3D11Resource*)d3d11_render_target, NULL, &d3d11_render_target_view);
  assert(SUCCEEDED(hr));

  D3D11_TEXTURE2D_DESC depth_stencil_desc;
  ID3D11Texture2D* d3d11_depth_stencil;
  ID3D11DepthStencilView* d3d11_depth_stencil_view;
  memset(&depth_stencil_desc, 0, sizeof(depth_stencil_desc));
  depth_stencil_desc.Width = 1280;
  depth_stencil_desc.Height = 720;
  depth_stencil_desc.MipLevels = 1;
  depth_stencil_desc.ArraySize = 1;
  depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depth_stencil_desc.SampleDesc = swapchain_desc.SampleDesc;
  depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
  depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  hr = ID3D11Device_CreateTexture2D(d3d11_device, &depth_stencil_desc, NULL, &d3d11_depth_stencil); 
  assert(SUCCEEDED(hr));
  D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc;
  memset(&depth_stencil_view_desc, 0, sizeof(depth_stencil_view_desc));
  depth_stencil_view_desc.Format = depth_stencil_desc.Format;
  depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
  hr = ID3D11Device_CreateDepthStencilView(d3d11_device, (ID3D11Resource*)d3d11_depth_stencil, &depth_stencil_view_desc, &d3d11_depth_stencil_view);
  assert(SUCCEEDED(hr));

  // TODO(maciej): Compile shaders
  // Compile vertex shader
  ID3D10Blob* vs_binary = d3d11_complie_shader(vs_src, "vs_5_0");
  const void* vs_ptr = ID3D10Blob_GetBufferPointer(vs_binary);
  SIZE_T vs_length = ID3D10Blob_GetBufferSize(vs_binary);
  ID3D11VertexShader* vs;
  hr = ID3D11Device_CreateVertexShader(d3d11_device, vs_ptr, vs_length, NULL, &vs);
  assert(SUCCEEDED(hr));
  ID3D10Blob_Release(vs_binary); vs_binary = 0;

  // Compile pixel shader
  ID3D10Blob* ps_binary = d3d11_complie_shader(ps_src, "ps_5_0");
  const void* ps_ptr = ID3D10Blob_GetBufferPointer(ps_binary);
  SIZE_T ps_length = ID3D10Blob_GetBufferSize(ps_binary);
  ID3D11PixelShader* ps;
  hr = ID3D11Device_CreatePixelShader(d3d11_device, ps_ptr, ps_length, NULL, &ps);
  assert(SUCCEEDED(hr));
  ID3D10Blob_Release(ps_binary); ps_binary = 0;
  
  // TODO(maciej): Create some meshes

  // Start with creating input layout - we will need to apply this/
  // NOTE(maciej): The input elements can be taken from the blob. This seems like a good idea, so there is less code mirroring?
  // Link: https://gist.github.com/mobius/b678970c61a93c81fffef1936734909f
  ID3D11InputLayout* d3d11_input_layout = NULL;
  
  D3D11_INPUT_ELEMENT_DESC element_descs[2] = {0};
  element_descs[0].SemanticName = "POS";
  element_descs[0].SemanticIndex = 0;
  element_descs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
  element_descs[0].InputSlot = 0;
  element_descs[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
  element_descs[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA; // Could be per instance data
  element_descs[0].InstanceDataStepRate = 0;

  element_descs[1].SemanticName = "COL";
  element_descs[1].SemanticIndex = 1;
  element_descs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  element_descs[1].InputSlot = 0;
  element_descs[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
  element_descs[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA; // Could be per instance data
  element_descs[1].InstanceDataStepRate = 0;


  // hr = ID3D11Device_CreateInputLayout(d3d11_device, /*This should be a pointer to input elemens*/NULL, 0, vs_binary, vs_length, &d3d11_input_layout );



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

      // NOTE(maciej): Render frame
      FLOAT clear_color[4] = {0.2f, 0.2f, 0.2f, 1.0f};
      UINT depth_stencil_flags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
      ID3D11DeviceContext_ClearRenderTargetView(d3d11_device_context, d3d11_render_target_view, clear_color);
      ID3D11DeviceContext_ClearDepthStencilView(d3d11_device_context, d3d11_depth_stencil_view, depth_stencil_flags, 0.0f, 1);

      // NOTE(maciej): End frame rendering

      IDXGISwapChain_Present(dxgi_swap_chain, 1, 0);

    }
  }

  // TODO(maciej): Be a good citizen and clean-up everything.

  DestroyWindow(window_handle); 
  window_handle = 0;
  UnregisterClassW(L"debugdraw_app", GetModuleHandleW(NULL));
  return 0;
}