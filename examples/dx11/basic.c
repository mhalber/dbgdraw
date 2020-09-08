
#include <stdbool.h>
#include <stdint.h>
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


//TODO(maciej): Handle events WM_MOUSELEAVE, WM_MOUSEWHEEL, WM_MOUSEHWHEEL, WM_CHAR
//TODO(maciej): Handle resizing

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

LRESULT CALLBACK
d3d11_window_procedure(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
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

int32_t
d3d11_init(d3d11_t* d3d11, const d3d11_desc_t* desc)
{
  d3d11->sample_count = desc->sample_count;

  HRESULT hr;
  // Register window class
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

  int32_t create_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
  D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
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

  return 1;
}

void
d3d11_terminate(d3d11_t* d3d11)
{
 (void) d3d11;
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

ID3DBlob*
d3d11_complie_shader(const char* src, const char* target)
{
  ID3DBlob* output = NULL;
  ID3DBlob* errors = NULL;
  UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3;
  D3DCompile(src, strlen(src), NULL, NULL, NULL, "main", target, flags, 0, &output, &errors);
  if( errors )
  {
    printf("%s\n", (char*)ID3D10Blob_GetBufferPointer(errors));
    ID3D10Blob_Release(errors); errors = NULL;
    return NULL;
  }
  return output;
}

const char* vs_src = 
 "struct vs_in {\n"
            "  float4 pos: POS;\n"
            "  float4 color: COL;\n"
            "};\n"
            "struct vs_out {\n"
            "  float4 color: COL0;\n"
            "  float4 pos: SV_Position;\n"
            "};\n"
            "vs_out main(vs_in inp) {\n"
            "  vs_out outp;\n"
            "  outp.pos = inp.pos;\n"
            "  outp.color = inp.color;\n"
            "  return outp;\n"
            "}\n";

const char* ps_src = 
            "float4 main(float4 color: COL0): SV_Target0 {\n"
            "  return color;\n"
            "}\n";

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;
  HRESULT hr;
  d3d11_desc_t d3d11_info = 
  {
    .win_title = "TestApp",
    .win_x = 100,
    .win_y = 100,
    .win_w = 1280,
    .win_h = 720,
    .sample_count = 1
  };
  d3d11_t d3d11 = {0};
  d3d11_init(&d3d11, &d3d11_info); // NOTE(maciej): This function should return opaque resource, like a decorated void ptr.
  d3d11_create_default_render_target(&d3d11);

  // Compile vertex shader
  ID3D10Blob* vs_binary = d3d11_complie_shader(vs_src, "vs_5_0");
  const void* vs_ptr = ID3D10Blob_GetBufferPointer(vs_binary);
  SIZE_T vs_length = ID3D10Blob_GetBufferSize(vs_binary);
  ID3D11VertexShader* vs;
  hr = ID3D11Device_CreateVertexShader(d3d11.device, vs_ptr, vs_length, NULL, &vs);
  assert(SUCCEEDED(hr));
  // ID3D10Blob_Release(vs_binary); vs_binary = 0;

  // Compile pixel shader
  ID3D10Blob* ps_binary = d3d11_complie_shader(ps_src, "ps_5_0");
  const void* ps_ptr = ID3D10Blob_GetBufferPointer(ps_binary);
  SIZE_T ps_length = ID3D10Blob_GetBufferSize(ps_binary);
  ID3D11PixelShader* ps;
  hr = ID3D11Device_CreatePixelShader(d3d11.device, ps_ptr, ps_length, NULL, &ps);
  assert(SUCCEEDED(hr));
  ID3D10Blob_Release(ps_binary); ps_binary = 0;
  
  // Start with creating input layout - we will need to apply this on each frame.
  // NOTE(maciej): The input elements can be taken from the blob. This seems like a good idea, so there is less code mirroring?
  // Link: https://gist.github.com/mobius/b678970c61a93c81fffef1936734909f
  ID3D11InputLayout* d3d11_input_layout;
  D3D11_INPUT_ELEMENT_DESC element_descs[2] = 
  {
    [0] = {.SemanticName="POS", .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .AlignedByteOffset =  0 },
    [1] = {.SemanticName="COL", .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .AlignedByteOffset = 16 },
  };
  hr = ID3D11Device_CreateInputLayout(d3d11.device, element_descs, 2, vs_ptr, vs_length, &d3d11_input_layout );
  assert(SUCCEEDED(hr));

  // Then we create Vertex Buffer Object and Index Buffer Object
  float vertices[3*2*4] = 
  { 
    0.0f, 0.5f, 0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,
    0.5f,-0.5f, 0.0f, 1.0f,   1.0f, 1.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.0f, 1.0f,   1.0f, 0.0f, 1.0f, 1.0f,
  };
  
  D3D11_SUBRESOURCE_DATA init_data = {0};
  init_data.pSysMem = vertices;
  D3D11_BUFFER_DESC vb_desc = 
  {
    .ByteWidth = sizeof(vertices),
    .Usage = D3D11_USAGE_IMMUTABLE,
    .BindFlags = D3D11_BIND_VERTEX_BUFFER,
    .CPUAccessFlags = 0
  };
  ID3D11Buffer *vertex_buffer;
  hr = ID3D11Device_CreateBuffer(d3d11.device, &vb_desc, &init_data, &vertex_buffer);
  assert(SUCCEEDED(hr));

  while (d3d11_process_events())
  {
    d3d11_clear(&d3d11, 0.2f, 0.2f, 0.2f, 1.0f);

    ID3D11DeviceContext_OMSetRenderTargets(d3d11.device_context, 1, &d3d11.render_target_view, NULL);

    D3D11_VIEWPORT vp = {0.0f, 0.0f, (FLOAT)d3d11.render_target_width, (FLOAT)d3d11.render_target_height, 0.0f, 1.0f};
    ID3D11DeviceContext_RSSetViewports(d3d11.device_context, 1, &vp);
    D3D11_RECT scissor_rect;
    scissor_rect.left = 0;
    scissor_rect.top = 0;
    scissor_rect.right = 1280;
    scissor_rect.bottom = 720;
    ID3D11DeviceContext_RSSetScissorRects(d3d11.device_context, 1, &scissor_rect);

    ID3D11DeviceContext_IASetPrimitiveTopology(d3d11.device_context, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetInputLayout(d3d11.device_context, d3d11_input_layout);
    UINT strides = 16*2;
    UINT offsets = 0;
    ID3D11DeviceContext_IASetVertexBuffers(d3d11.device_context, 0, 1, &vertex_buffer, &strides, &offsets );

    ID3D11DeviceContext_VSSetShader(d3d11.device_context, vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(d3d11.device_context, ps, NULL, 0);

    ID3D11DeviceContext_Draw(d3d11.device_context, 3, 0);
    // NOTE(maciej): End frame rendering

    d3d11_present(&d3d11);
  }

  d3d11_destroy_default_render_target(&d3d11);
  d3d11_terminate(&d3d11);
  return 0;
}