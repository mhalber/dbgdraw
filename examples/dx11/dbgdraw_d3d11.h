#ifndef DBGDRAW_D3D11_H
#define DBGDRAW_D3D11_H

typedef struct dd_render_backend
{
  const d3d11_t* d3d11;

  int32_t vertex_buffer_size;
  ID3D11Buffer* vertex_buffer;

  ID3D11RasterizerState* rasterizer_state;
  ID3D11DepthStencilState* depth_stencil_state;
  ID3D11BlendState* blend_state;

  ID3D11InputLayout* base_input_layout;
  ID3D11VertexShader* base_vs;
  ID3D11PixelShader* base_ps;
  ID3D11Buffer* base_constant_buffer;

  ID3D11VertexShader* line_vs;
  ID3D11PixelShader* line_ps;
  ID3D11Buffer* line_cb;

} dd_render_backend_t;

typedef struct dd_base_cb_data
{
  dd_mat4_t mvp;
  int shading_type;
  int instancing_enabled;
} dd_base_cb_data_t;

ID3DBlob* dd__d3d11_complie_shader( const char* source, const char* target, const char* main );

void dd__init_base_shaders_source( const char **shdr_src );
void dd__init_line_shaders_source( const char **shdr_src );

int32_t dd_backend_init( dd_ctx_t* ctx );
int32_t dd_backend_term( dd_ctx_t* ctx );
int32_t dd_backend_render( dd_ctx_t* ctx );
int32_t dd_backend_init_font_texture( dd_ctx_t *ctx, const uint8_t *data, int32_t width, int32_t height, uint32_t *tex_id );

int32_t dd_backend_init( dd_ctx_t* ctx )
{
  HRESULT hr;
  dd_render_backend_t *backend = (dd_render_backend_t*)ctx->render_backend;
  const d3d11_t* d3d11 = backend->d3d11;

  const char* base_shd_src = NULL;
  dd__init_base_shaders_source( &base_shd_src );

  // Compile base vertex shader
  ID3D10Blob* base_vs_binary = dd__d3d11_complie_shader( base_shd_src, "vs_5_0", "vs_main" );
  hr = ID3D11Device_CreateVertexShader(d3d11->device, 
                                       ID3D10Blob_GetBufferPointer(base_vs_binary), 
                                       ID3D10Blob_GetBufferSize(base_vs_binary), 
                                       NULL, 
                                       &backend->base_vs);
  assert(SUCCEEDED(hr));

  // Compile base pixel shader
  ID3D10Blob* base_ps_binary = dd__d3d11_complie_shader( base_shd_src, "ps_5_0", "ps_main" );
  hr = ID3D11Device_CreatePixelShader(d3d11->device, 
                                      ID3D10Blob_GetBufferPointer( base_ps_binary ),
                                      ID3D10Blob_GetBufferSize( base_ps_binary ),
                                      NULL,
                                      &backend->base_ps);
  assert(SUCCEEDED(hr));

  // Create input layout for base shader
  D3D11_INPUT_ELEMENT_DESC element_descs[3] = 
  {
    [0] = { .SemanticName="POS", .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .AlignedByteOffset =  0 },
    [1] = { .SemanticName="NOR", .Format = DXGI_FORMAT_R32G32B32_FLOAT,    .AlignedByteOffset = 16 },
    [2] = { .SemanticName="COL", .Format = DXGI_FORMAT_R8G8B8A8_UNORM,     .AlignedByteOffset = 28 },
  };
  hr = ID3D11Device_CreateInputLayout( d3d11->device,
                                       element_descs,
                                       3, 
                                       ID3D10Blob_GetBufferPointer( base_vs_binary ),
                                       ID3D10Blob_GetBufferSize(base_vs_binary),
                                       &backend->base_input_layout );
  assert(SUCCEEDED(hr));

  ID3D10Blob_Release(base_vs_binary); base_vs_binary = 0;
  ID3D10Blob_Release(base_ps_binary); base_ps_binary = 0;

  // Create constant buffer (think OGL Uniforms / UBO)
  D3D11_BUFFER_DESC base_constant_buffer_desc = 
  {
    .ByteWidth = sizeof(dd_base_cb_data_t) + 0xf & 0xfffffff0,
    .Usage     = D3D11_USAGE_DYNAMIC,
    .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
  };
  hr = ID3D11Device_CreateBuffer(d3d11->device, &base_constant_buffer_desc, NULL, &backend->base_constant_buffer);
  assert(SUCCEEDED(hr));

  // Create vertex buffer
  backend->vertex_buffer_size = ctx->verts_cap * sizeof(dd_vertex_t);
  D3D11_BUFFER_DESC vb_desc = 
  {
    .ByteWidth      = backend->vertex_buffer_size,
    .Usage          = D3D11_USAGE_DYNAMIC,
    .BindFlags      = D3D11_BIND_VERTEX_BUFFER,
    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
  };
  hr = ID3D11Device_CreateBuffer(d3d11->device, &vb_desc, NULL, &backend->vertex_buffer);
  assert(SUCCEEDED(hr));

// Setup rasterizer state
  D3D11_RASTERIZER_DESC rasterizer_state_desc = 
  {
    .FillMode = D3D11_FILL_SOLID,
    .CullMode = D3D11_CULL_BACK
  };
  hr = ID3D11Device_CreateRasterizerState( d3d11->device, &rasterizer_state_desc, &backend->rasterizer_state );
  assert(SUCCEEDED(hr));

  // Setup depth-stencil state
  D3D11_DEPTH_STENCIL_DESC depth_stencil_state_desc = 
  {
    .DepthEnable    = ctx->enable_depth_test,
    .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
    .DepthFunc      = D3D11_COMPARISON_LESS
  };
  hr = ID3D11Device_CreateDepthStencilState( d3d11->device, &depth_stencil_state_desc, &backend->depth_stencil_state );
  assert(SUCCEEDED(hr));

  // Setup blend state
  D3D11_BLEND_DESC blend_state_desc =
  {
    .AlphaToCoverageEnable  = FALSE,
    .IndependentBlendEnable = FALSE,
    .RenderTarget[0] =
    {
      .BlendEnable = TRUE,
      .SrcBlend = D3D11_BLEND_SRC_ALPHA,
      .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
      .BlendOp = D3D11_BLEND_OP_ADD,
      .SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
      .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
      .BlendOpAlpha = D3D11_BLEND_OP_ADD,
      .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
    }
  };
  hr = ID3D11Device_CreateBlendState( d3d11->device, &blend_state_desc, &backend->blend_state );
  assert(SUCCEEDED(hr));

  return 1;
}

int32_t dd_backend_term(dd_ctx_t* ctx)
{
  dd_render_backend_t *backend = (dd_render_backend_t*)ctx->render_backend;

  ID3D11Buffer_Release( backend->vertex_buffer );

  ID3D11RasterizerState_Release( backend->rasterizer_state );
  ID3D11BlendState_Release( backend->blend_state );
  ID3D11DepthStencilState_Release( backend->depth_stencil_state );

  ID3D11InputLayout_Release( backend->base_input_layout );
  ID3D11VertexShader_Release( backend->base_vs );
  ID3D11PixelShader_Release( backend->base_ps );
  ID3D11Buffer_Release( backend->base_constant_buffer );

  return 0;
}

int32_t dd_backend_render(dd_ctx_t* ctx)
{
  dd_render_backend_t* backend = ctx->render_backend;
  const d3d11_t* d3d11 = backend->d3d11;

  if (!ctx->commands_len)
  {
    return DBGDRAW_ERR_OK;
  }

  // Update the vertex buffer
  D3D11_MAPPED_SUBRESOURCE vertex_buffer_data = {0};
  ID3D11DeviceContext_Map( d3d11->device_context, (ID3D11Resource*)backend->vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vertex_buffer_data );
  memcpy( vertex_buffer_data.pData, ctx->verts_data, ctx->verts_len*sizeof(dd_vertex_t) );
  ID3D11DeviceContext_Unmap(d3d11->device_context, (ID3D11Resource*)backend->vertex_buffer, 0) ;
  
  // Setup the required state
    FLOAT blend_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  ID3D11DeviceContext_OMSetDepthStencilState( d3d11->device_context, backend->depth_stencil_state, 0 );
  ID3D11DeviceContext_OMSetBlendState( d3d11->device_context, backend->blend_state, blend_color, 0xFFFFFFFF );
  ID3D11DeviceContext_RSSetState( d3d11->device_context, backend->rasterizer_state );
  
  static D3D_PRIMITIVE_TOPOLOGY topology_modes[3];
  topology_modes[DBGDRAW_MODE_FILL] = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  topology_modes[DBGDRAW_MODE_STROKE] = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
  topology_modes[DBGDRAW_MODE_POINT] = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

  D3D11_MAPPED_SUBRESOURCE constant_buffer_data = {0};

  for (int32_t i = 0; i < ctx->commands_len; ++i)
  {
    dd_cmd_t* cmd = ctx->commands + i;
    dd_mat4_t mvp = dd_mat4_mul( ctx->proj, dd_mat4_mul( ctx->view, cmd->xform ) );
    UINT strides = sizeof(dd_vertex_t);
    UINT offsets = 0;

    if (cmd->instance_count && cmd->instance_data )
    {
      // Bind instance data buffer
    }

    // Update the constant buffer
    // NOTE(maciej): Is UpdateSubresource faster than mapping?
    dd_base_cb_data_t cb_data = { .mvp = mvp, .shading_type = cmd->shading_type, .instancing_enabled = (cmd->instance_count > 0) };
    ID3D11DeviceContext_Map( d3d11->device_context, (ID3D11Resource*)backend->base_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constant_buffer_data );
    memcpy( constant_buffer_data.pData, &cb_data.mvp, sizeof(dd_base_cb_data_t) );
    ID3D11DeviceContext_Unmap(d3d11->device_context, (ID3D11Resource*)backend->base_constant_buffer, 0 );

    ID3D11DeviceContext_IASetPrimitiveTopology(d3d11->device_context, topology_modes[cmd->draw_mode]);
    ID3D11DeviceContext_IASetInputLayout(d3d11->device_context, backend->base_input_layout);  
    ID3D11DeviceContext_IASetVertexBuffers(d3d11->device_context, 0, 1, &backend->vertex_buffer, &strides, &offsets );

    ID3D11DeviceContext_VSSetConstantBuffers(d3d11->device_context, 0, 1, &backend->base_constant_buffer);
    ID3D11DeviceContext_VSSetShader(d3d11->device_context, backend->base_vs, NULL, 0);

    ID3D11DeviceContext_PSSetShader(d3d11->device_context, backend->base_ps, NULL, 0);
    ID3D11DeviceContext_Draw(d3d11->device_context, ctx->verts_len, 0);
  }
  
  return DBGDRAW_ERR_OK;
}

int32_t
dd_backend_init_font_texture(dd_ctx_t *ctx, const uint8_t *data, int32_t width, int32_t height, uint32_t *tex_id)
{
  (void)ctx;
  (void)data;
  (void)width;
  (void)height;
  (void)tex_id;
  return 0;
}

ID3DBlob*
dd__d3d11_complie_shader(const char* src, const char* target, const char* main)
{
  ID3DBlob* output = NULL;
  ID3DBlob* errors = NULL;
  UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3;
  D3DCompile(src, strlen(src), NULL, NULL, NULL, main, target, flags, 0, &output, &errors);
  if( errors )
  {
    printf("%s\n", (char*)ID3D10Blob_GetBufferPointer(errors));
    ID3D10Blob_Release(errors); errors = NULL;
    return NULL;
  }
  return output;
}

#define DBGDRAW_D3D11_STRINGIFY(x) #x

void
dd__init_base_shaders_source( const char** shdr_src )
{
  *shdr_src = DBGDRAW_D3D11_STRINGIFY(
    
    cbuffer vs_uniforms
    {
      float4x4 mvp;
      int shading_type;
    }; 
    
    struct vs_in {
      float4 pos: POS;
      float3 normal: NOR;
      float4 color: COL;
    };
    
    struct vs_out {
      float4 pos: SV_POSITION;
      float4 color: COL;
    };

    vs_out vs_main( vs_in input ) {
      vs_out output;
      output.pos = mul( mvp, input.pos );
      output.color = input.color;
      return output;
    } 

    float4 ps_main( vs_out input): SV_TARGET {
      return input.color;
    };
  );
}

#undef DBGDRAW_D3D11_STRINGIFY

#endif /* DBGDRAW_D3D11_H */