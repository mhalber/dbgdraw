
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

#include "d3d11_utils.h"
//TODO(maciej): Handle events WM_MOUSELEAVE, WM_MOUSEWHEEL, WM_MOUSEHWHEEL, WM_CHAR
//TODO(maciej): Handle resizing

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
  d3d11_t* d3d11 = d3d11_init(&d3d11_info); // NOTE(maciej): This function should return opaque resource, like a decorated void ptr.

  // Compile vertex shader
  ID3D10Blob* vs_binary = d3d11_complie_shader(vs_src, "vs_5_0");
  const void* vs_ptr = ID3D10Blob_GetBufferPointer(vs_binary);
  SIZE_T vs_length = ID3D10Blob_GetBufferSize(vs_binary);
  ID3D11VertexShader* vs;
  hr = ID3D11Device_CreateVertexShader(d3d11->device, vs_ptr, vs_length, NULL, &vs);
  assert(SUCCEEDED(hr));
  // ID3D10Blob_Release(vs_binary); vs_binary = 0;

  // Compile pixel shader
  ID3D10Blob* ps_binary = d3d11_complie_shader(ps_src, "ps_5_0");
  const void* ps_ptr = ID3D10Blob_GetBufferPointer(ps_binary);
  SIZE_T ps_length = ID3D10Blob_GetBufferSize(ps_binary);
  ID3D11PixelShader* ps;
  hr = ID3D11Device_CreatePixelShader(d3d11->device, ps_ptr, ps_length, NULL, &ps);
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
  hr = ID3D11Device_CreateInputLayout(d3d11->device, element_descs, 2, vs_ptr, vs_length, &d3d11_input_layout );
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
  hr = ID3D11Device_CreateBuffer(d3d11->device, &vb_desc, &init_data, &vertex_buffer);
  assert(SUCCEEDED(hr));

  while (d3d11_process_events())
  {
    d3d11_clear(d3d11, 0.2f, 0.2f, 0.2f, 1.0f);

    ID3D11DeviceContext_OMSetRenderTargets(d3d11->device_context, 1, &d3d11->render_target_view, NULL);
    D3D11_VIEWPORT vp = {0.0f, 0.0f, (FLOAT)d3d11->render_target_width, (FLOAT)d3d11->render_target_height, 0.0f, 1.0f};
    ID3D11DeviceContext_RSSetViewports(d3d11->device_context, 1, &vp);
    D3D11_RECT scissor_rect;
    scissor_rect.left = 0;
    scissor_rect.top = 0;
    scissor_rect.right = 1280;
    scissor_rect.bottom = 720;
    ID3D11DeviceContext_RSSetScissorRects(d3d11->device_context, 1, &scissor_rect);

    ID3D11DeviceContext_IASetPrimitiveTopology(d3d11->device_context, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetInputLayout(d3d11->device_context, d3d11_input_layout);
    UINT strides = 16*2;
    UINT offsets = 0;
    ID3D11DeviceContext_IASetVertexBuffers(d3d11->device_context, 0, 1, &vertex_buffer, &strides, &offsets );

    ID3D11DeviceContext_VSSetShader(d3d11->device_context, vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(d3d11->device_context, ps, NULL, 0);

    ID3D11DeviceContext_Draw(d3d11->device_context, 3, 0);

    d3d11_present(d3d11);
  }

  d3d11_terminate(d3d11);
  return 0;
}