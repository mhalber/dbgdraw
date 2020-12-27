#ifndef DBGDRAW_D3D11_H
#define DBGDRAW_D3D11_H

#define DBGDRAW_D3D11_MAX_FONTS 16

typedef struct dd_render_backend
{
  const d3d11_t* d3d11;

  uint32_t vertex_buffer_size;
  ID3D11Buffer* vertex_buffer;
  ID3D11ShaderResourceView* vertex_buffer_view;

  uint32_t instance_buffer_size;
  ID3D11Buffer* instance_buffer;
  ID3D11ShaderResourceView* instance_buffer_view;

  ID3D11RasterizerState* rasterizer_state;
  ID3D11DepthStencilState* depth_stencil_state;
  ID3D11BlendState* blend_state;

  ID3D11InputLayout* base_input_layout;
  ID3D11VertexShader* fill_vs;
  ID3D11PixelShader* fill_ps;
  ID3D11VertexShader* point_vs;
  ID3D11PixelShader* point_ps;

  ID3D11Buffer* base_constant_buffer;

#if DBGDRAW_HAS_TEXT_SUPPORT
  ID3D11SamplerState* font_texture_sampler;

  int32_t font_texture_count;
  ID3D11Texture2D* font_textures[DBGDRAW_D3D11_MAX_FONTS];
  ID3D11ShaderResourceView* font_textures_views[DBGDRAW_D3D11_MAX_FONTS];
#endif
} dd_render_backend_t;

typedef struct dd_base_cb_data
{
  dd_mat4_t mvp;
  dd_vec4_t viewport;
  int shading_type;
  int base_index;
  int instancing_enabled;
} dd_cb_data_t;

ID3DBlob* dd__d3d11_compile_shader( const char* source, const char* target, const char* main );

void dd__init_fill_shader_source( const char **shdr_src );
void dd__init_stroke_shader_source( const char **shdr_src );
void dd__init_point_shader_source( const char **shdr_src );

int32_t dd_backend_init( dd_ctx_t* ctx );
int32_t dd_backend_term( dd_ctx_t* ctx );
int32_t dd_backend_render( dd_ctx_t* ctx );
int32_t dd_backend_init_font_texture( dd_ctx_t *ctx, const uint8_t *data, int32_t width, int32_t height, uint32_t *tex_id );


int32_t 
dd_backend_init( dd_ctx_t* ctx )
{
  HRESULT hr;
  dd_render_backend_t *backend = (dd_render_backend_t*)ctx->render_backend;
  const d3d11_t* d3d11 = backend->d3d11;

  const char* fill_shd_src = NULL;
  const char* point_shd_src = NULL;
  dd__init_fill_shader_source( &fill_shd_src );
  dd__init_point_shader_source( &point_shd_src );

  // Compile fill vertex shader
  ID3D10Blob* fill_vs_binary = dd__d3d11_compile_shader( fill_shd_src, "vs_5_0", "vs_main" );
  hr = ID3D11Device_CreateVertexShader(d3d11->device, 
                                       ID3D10Blob_GetBufferPointer(fill_vs_binary), 
                                       ID3D10Blob_GetBufferSize(fill_vs_binary), 
                                       NULL, 
                                       &backend->fill_vs);
  assert(SUCCEEDED(hr));

  // Compile fill pixel shader
  ID3D10Blob* fill_ps_binary = dd__d3d11_compile_shader( fill_shd_src, "ps_5_0", "ps_main" );
  hr = ID3D11Device_CreatePixelShader(d3d11->device, 
                                      ID3D10Blob_GetBufferPointer( fill_ps_binary ),
                                      ID3D10Blob_GetBufferSize( fill_ps_binary ),
                                      NULL,
                                      &backend->fill_ps);
  assert(SUCCEEDED(hr));


  // Create input layout for fill shader
  D3D11_INPUT_ELEMENT_DESC element_descs[] = 
  {
    [0] = { .SemanticName="POS", .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .AlignedByteOffset =  0 },
    [1] = { .SemanticName="NOR", .Format = DXGI_FORMAT_R32G32B32_FLOAT,    .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT },
    [2] = { .SemanticName="COL", .Format = DXGI_FORMAT_R8G8B8A8_UNORM,     .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT },
    [3] = { .SemanticName="POS", .Format = DXGI_FORMAT_R32G32B32_FLOAT,    .AlignedByteOffset = 0,
            .SemanticIndex = 1, .InputSlot=1, .InputSlotClass=D3D11_INPUT_PER_INSTANCE_DATA, .InstanceDataStepRate = 1},
    [4] = { .SemanticName="COL", .Format = DXGI_FORMAT_R8G8B8A8_UNORM,     .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
            .SemanticIndex = 1, .InputSlot=1, .InputSlotClass=D3D11_INPUT_PER_INSTANCE_DATA, .InstanceDataStepRate = 1}
  };
  hr = ID3D11Device_CreateInputLayout( d3d11->device,
                                       element_descs,
                                       5, 
                                       ID3D10Blob_GetBufferPointer(fill_vs_binary),
                                       ID3D10Blob_GetBufferSize(fill_vs_binary),
                                       &backend->base_input_layout );
  assert(SUCCEEDED(hr));

  ID3D10Blob_Release(fill_vs_binary); fill_vs_binary = 0;
  ID3D10Blob_Release(fill_ps_binary); fill_ps_binary = 0;



  // Compile point vertex shader
  ID3D10Blob* point_vs_binary = dd__d3d11_compile_shader( point_shd_src, "vs_5_0", "vs_main" );
  hr = ID3D11Device_CreateVertexShader(d3d11->device, 
                                       ID3D10Blob_GetBufferPointer(point_vs_binary), 
                                       ID3D10Blob_GetBufferSize(point_vs_binary), 
                                       NULL, 
                                       &backend->point_vs);
  assert(SUCCEEDED(hr));

  // Compile point pixel shader
  ID3D10Blob* point_ps_binary = dd__d3d11_compile_shader( point_shd_src, "ps_5_0", "ps_main" );
  hr = ID3D11Device_CreatePixelShader(d3d11->device, 
                                      ID3D10Blob_GetBufferPointer( point_ps_binary ),
                                      ID3D10Blob_GetBufferSize( point_ps_binary ),
                                      NULL,
                                      &backend->point_ps);
  assert(SUCCEEDED(hr));


  // Create constant buffer (think OGL Uniforms / UBO)
  D3D11_BUFFER_DESC base_constant_buffer_desc = 
  {
    .ByteWidth = sizeof(dd_cb_data_t) + 0xf & 0xfffffff0,
    .Usage     = D3D11_USAGE_DYNAMIC,
    .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
  };
  hr = ID3D11Device_CreateBuffer(d3d11->device, &base_constant_buffer_desc, NULL, &backend->base_constant_buffer);
  assert(SUCCEEDED(hr));

  // Create the vertex buffer
  backend->vertex_buffer_size = ctx->verts_cap * sizeof(dd_vertex_t);
  D3D11_BUFFER_DESC vb_desc = 
  {
    .ByteWidth      = backend->vertex_buffer_size,
    .Usage          = D3D11_USAGE_DYNAMIC,
    .BindFlags      = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_VERTEX_BUFFER,
    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    .MiscFlags      = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS
  };
  hr = ID3D11Device_CreateBuffer(d3d11->device, &vb_desc, NULL, &backend->vertex_buffer);
  assert(SUCCEEDED(hr));

  D3D11_SHADER_RESOURCE_VIEW_DESC vb_view_desc = 
  {
    .Format = DXGI_FORMAT_R32_TYPELESS,
    .ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX,
    .BufferEx = 
    {
      .FirstElement = 0,
      .NumElements = (ctx->verts_cap * sizeof(dd_vertex_t)) / 4, // why divide by 4??
      .Flags = D3D11_BUFFEREX_SRV_FLAG_RAW,
    }
  };
  hr = ID3D11Device_CreateShaderResourceView(d3d11->device, (ID3D11Resource*)backend->vertex_buffer, &vb_view_desc, &backend->vertex_buffer_view);
  assert(SUCCEEDED(hr));

  // Create the instance buffer
  backend->instance_buffer_size = ctx->instance_cap * sizeof(dd_instance_data_t);
  D3D11_BUFFER_DESC ib_desc = 
  {
    .ByteWidth      = backend->instance_buffer_size,
    .Usage          = D3D11_USAGE_DYNAMIC,
    .BindFlags      = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_VERTEX_BUFFER,
    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    .MiscFlags      = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS
  };
  hr = ID3D11Device_CreateBuffer(d3d11->device, &ib_desc, NULL, &backend->instance_buffer);
  assert(SUCCEEDED(hr));

  D3D11_SHADER_RESOURCE_VIEW_DESC ib_view_desc = 
  {
    .Format = DXGI_FORMAT_R32_TYPELESS,
    .ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX,
    .BufferEx = 
    {
      .FirstElement = 0,
      .NumElements = (ctx->instance_cap * sizeof(dd_instance_data_t)) / 4, // why divide by 4??
      .Flags = D3D11_BUFFEREX_SRV_FLAG_RAW,
    }
  };
  hr = ID3D11Device_CreateShaderResourceView(d3d11->device, (ID3D11Resource*)backend->instance_buffer, &ib_view_desc, &backend->instance_buffer_view);
  assert(SUCCEEDED(hr));

  // Setup sampler    
  D3D11_SAMPLER_DESC sampler_desc = 
  {
    .Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
    .AddressU       = D3D11_TEXTURE_ADDRESS_WRAP,
    .AddressV       = D3D11_TEXTURE_ADDRESS_WRAP,
    .AddressW       = D3D11_TEXTURE_ADDRESS_WRAP,
    .ComparisonFunc = D3D11_COMPARISON_NEVER,
  };

  ID3D11Device_CreateSamplerState(d3d11->device, &sampler_desc, &backend->font_texture_sampler);


  // Setup rasterizer state
  D3D11_RASTERIZER_DESC rasterizer_state_desc = 
  {
    .FillMode = D3D11_FILL_SOLID,
    .CullMode = D3D11_CULL_NONE
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

  return DBGDRAW_ERR_OK;
}

int32_t dd_backend_term(dd_ctx_t* ctx)
{
  dd_render_backend_t *backend = (dd_render_backend_t*)ctx->render_backend;

  ID3D11Buffer_Release( backend->vertex_buffer );
  ID3D11Buffer_Release( backend->instance_buffer );
  ID3D11ShaderResourceView_Release( backend->vertex_buffer_view );
  ID3D11ShaderResourceView_Release( backend->instance_buffer_view );

  ID3D11RasterizerState_Release( backend->rasterizer_state );
  ID3D11BlendState_Release( backend->blend_state );
  ID3D11DepthStencilState_Release( backend->depth_stencil_state );

  ID3D11InputLayout_Release( backend->base_input_layout );
  ID3D11VertexShader_Release( backend->fill_vs );
  ID3D11PixelShader_Release( backend->fill_ps );
  ID3D11Buffer_Release( backend->base_constant_buffer );

#if DBGDRAW_HAS_TEXT_SUPPORT
  for (int32_t i = 0; i < backend->font_texture_count; ++i)
  {
    ID3D11Buffer_Release( backend->font_textures[i] );
    ID3D11ShaderResourceView_Release( backend->font_textures_views[i] );
  }
#endif
  return DBGDRAW_ERR_OK;
}

int32_t dd_backend_render(dd_ctx_t* ctx)
{
  dd_render_backend_t* backend = ctx->render_backend;
  const d3d11_t* d3d11 = backend->d3d11;

  if (!ctx->commands_len)
  {
    return DBGDRAW_ERR_OK;
  }

  // Update the data buffer
  if ( backend->vertex_buffer_size < ctx->verts_cap * sizeof(dd_vertex_t))
  {
    backend->vertex_buffer_size = ctx->verts_cap * sizeof(dd_vertex_t);
    ID3D11Buffer_Release( backend->vertex_buffer );
    ID3D11ShaderResourceView_Release( backend->vertex_buffer_view );
    D3D11_BUFFER_DESC db_desc = 
    {
      .ByteWidth      = backend->vertex_buffer_size,
      .Usage          = D3D11_USAGE_DYNAMIC,
      .BindFlags      = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_VERTEX_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags      = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS
    };
    HRESULT hr = ID3D11Device_CreateBuffer(d3d11->device, &db_desc, NULL, &backend->vertex_buffer);
    assert(SUCCEEDED(hr));
    D3D11_SHADER_RESOURCE_VIEW_DESC db_view_desc = 
    {
      .Format = DXGI_FORMAT_R32_TYPELESS,
      .ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX,
      .BufferEx = 
      {
        .FirstElement = 0,
        .NumElements = (ctx->verts_cap * sizeof(dd_vertex_t)) / 4,
        .Flags = D3D11_BUFFEREX_SRV_FLAG_RAW,
      }
    };
    hr = ID3D11Device_CreateShaderResourceView(d3d11->device, (ID3D11Resource*)backend->vertex_buffer, &db_view_desc, &backend->vertex_buffer_view);
    assert(SUCCEEDED(hr));
  }

  // TODO(maciej): Set vertex data dirty somehow?
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
  topology_modes[DBGDRAW_MODE_FILL]   = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  topology_modes[DBGDRAW_MODE_STROKE] = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
  topology_modes[DBGDRAW_MODE_POINT]  = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

  D3D11_MAPPED_SUBRESOURCE constant_buffer_data = {0};

  ID3D11Buffer* null_buffer = NULL;
  ID3D11InputLayout* null_layout = NULL;
  UINT strides[2] = {sizeof(dd_vertex_t), sizeof(dd_instance_data_t)};
  UINT offsets[2] = {0, 0};
  for (int32_t i = 0; i < ctx->commands_len; ++i)
  {
    dd_cmd_t* cmd = ctx->commands + i;
    dd_mat4_t mvp = dd_mat4_mul( ctx->proj, dd_mat4_mul( ctx->view, cmd->xform ) );
    int num_buffers = 1;
    if (cmd->instance_count && cmd->instance_data )
    {
      num_buffers = 2;
      // TODO(maciej): Set instances dirty somehow?
      D3D11_MAPPED_SUBRESOURCE instance_buffer_data = {0};
      ID3D11DeviceContext_Map( d3d11->device_context, (ID3D11Resource*)backend->instance_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instance_buffer_data );
      memcpy( instance_buffer_data.pData, cmd->instance_data, cmd->instance_count*sizeof(dd_instance_data_t) );
      ID3D11DeviceContext_Unmap(d3d11->device_context, (ID3D11Resource*)backend->instance_buffer, 0) ;
    }

    // Update the constant buffer
    // NOTE(maciej): Is UpdateSubresource faster than mapping?
    dd_cb_data_t cb_data = 
    { 
      .mvp = mvp, 
      .viewport = ctx->viewport,
      .shading_type = cmd->shading_type, 
      .base_index = cmd->base_index,
      .instancing_enabled = (int)(cmd->instance_count && cmd->instance_data)
    };

    ID3D11DeviceContext_Map( d3d11->device_context, (ID3D11Resource*)backend->base_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constant_buffer_data );
    memcpy( constant_buffer_data.pData, &cb_data.mvp, sizeof(dd_cb_data_t) );
    ID3D11DeviceContext_Unmap(d3d11->device_context, (ID3D11Resource*)backend->base_constant_buffer, 0 );

    ID3D11DeviceContext_IASetPrimitiveTopology(d3d11->device_context, topology_modes[cmd->draw_mode]);

    if (cmd->draw_mode == DBGDRAW_MODE_FILL )
    {
      ID3D11Buffer* buffers[] = { backend->vertex_buffer, backend->instance_buffer };

      ID3D11DeviceContext_IASetInputLayout(d3d11->device_context, backend->base_input_layout);  
      ID3D11DeviceContext_IASetVertexBuffers(d3d11->device_context, 0, num_buffers, buffers, strides, offsets );

      ID3D11DeviceContext_VSSetShader(d3d11->device_context, backend->fill_vs, NULL, 0);
      ID3D11DeviceContext_VSSetConstantBuffers(d3d11->device_context, 0, 1, &backend->base_constant_buffer);

      ID3D11DeviceContext_PSSetShader(d3d11->device_context, backend->fill_ps, NULL, 0);
      ID3D11DeviceContext_PSSetConstantBuffers(d3d11->device_context, 0, 1, &backend->base_constant_buffer);

#if DBGDRAW_HAS_TEXT_SUPPORT
      if (cmd->font_idx >= 0)
      {
        ID3D11DeviceContext_PSSetShaderResources(d3d11->device_context, 0, 1, &backend->font_textures_views[cmd->font_idx]);
        ID3D11DeviceContext_PSSetSamplers(d3d11->device_context, 0, 1, &backend->font_texture_sampler);
      }
#endif

      if (cmd->instance_count ) 
      {
        ID3D11DeviceContext_DrawInstanced(d3d11->device_context, cmd->vertex_count, max(cmd->instance_count, 1), cmd->base_index, 0 );
      }
      else
      {
        ID3D11DeviceContext_Draw( d3d11->device_context, cmd->vertex_count, cmd->base_index );
      }

    }
    else if (cmd->draw_mode == DBGDRAW_MODE_POINT)
    {
      ID3D11ShaderResourceView* buffer_views[] = {backend->vertex_buffer_view, backend->instance_buffer_view};
      ID3D11DeviceContext_IASetPrimitiveTopology(d3d11->device_context, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      ID3D11DeviceContext_IASetInputLayout(d3d11->device_context, null_layout);  
      ID3D11DeviceContext_IASetVertexBuffers(d3d11->device_context, 0, 1, &null_buffer, strides, offsets );

      ID3D11DeviceContext_VSSetShaderResources(d3d11->device_context, 0, num_buffers, buffer_views);
      ID3D11DeviceContext_VSSetShader(d3d11->device_context, backend->point_vs, NULL, 0);

      ID3D11DeviceContext_PSSetConstantBuffers(d3d11->device_context, 0, 1, &backend->base_constant_buffer);
      ID3D11DeviceContext_PSSetShader(d3d11->device_context, backend->point_ps, NULL, 0);
      if (cmd->instance_count ) 
      {
        ID3D11DeviceContext_DrawInstanced(d3d11->device_context, 6*cmd->vertex_count, max(cmd->instance_count, 1), 0, 0 );
      }
      else
      {
        ID3D11DeviceContext_Draw(d3d11->device_context, 6*cmd->vertex_count, 0);
      }
    }
    else
    {
      // SKIP
    }
  }  
  return DBGDRAW_ERR_OK;
}


ID3DBlob*
dd__d3d11_compile_shader(const char* src, const char* target, const char* main)
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

int32_t
dd_backend_init_font_texture(dd_ctx_t *ctx, const uint8_t *data, int32_t width, int32_t height, uint32_t *tex_id)
{
#if DBGDRAW_HAS_TEXT_SUPPORT

  // TODO(maciej): Change into error?
  assert(ctx->fonts_cap <= DBGDRAW_D3D11_MAX_FONTS);

  HRESULT hr;
  dd_render_backend_t* backend = ctx->render_backend;
  const d3d11_t* d3d11 = backend->d3d11;

  // Create texture
  D3D11_TEXTURE2D_DESC texture_desc = 
  {
    .Width = width,
    .Height = height,
    .MipLevels = 1,
    .ArraySize = 1, // Would have to redesign api to make use of this
    .Format = DXGI_FORMAT_R8_UNORM,
    .SampleDesc = { .Count = 1 },
    .Usage = D3D11_USAGE_IMMUTABLE,
    .BindFlags = D3D11_BIND_SHADER_RESOURCE
  };

  D3D11_SUBRESOURCE_DATA texture_data =
  {
    .pSysMem = data,
    .SysMemPitch = width,
  };

  int32_t cur_idx = backend->font_texture_count++;
  *tex_id = cur_idx;
  hr = ID3D11Device_CreateTexture2D(d3d11->device, &texture_desc, &texture_data, &(backend->font_textures[cur_idx]) );
  assert(SUCCEEDED(hr));
  hr = ID3D11Device_CreateShaderResourceView(d3d11->device, (ID3D11Resource*)backend->font_textures[cur_idx], NULL, &(backend->font_textures_views[cur_idx]));
  assert(SUCCEEDED(hr));
#endif

  return DBGDRAW_ERR_OK;
}

#define DBGDRAW_D3D11_STRINGIFY(x) #x

void
dd__init_fill_shader_source( const char** shdr_src )
{
  *shdr_src = DBGDRAW_D3D11_STRINGIFY(
    Texture2D font_texture: register(t0);
    SamplerState font_texture_sampler : register(s0);
    
    cbuffer vs_uniforms
    {
      float4x4 mvp;
      float4 viewport;
      int shading_type;
      int base_index;
      int instancing_enabled;
    };
    
    struct vs_in {
      float4 pos_size: POS0;
      float3 uv_or_normal: NOR;
      float4 color: COL0;

      float3 instance_pos: POS1;
      float4 instance_color: COL1;
    };
    
    struct vs_out {
      float4 pos: SV_POSITION;
      float3 uv_or_normal: NOR;
      float4 color: COL;
    };

    vs_out vs_main(vs_in input) {
      float3 pos = float3(0.0, 0.0, 0.0);
      float4 color = float4(0.0, 0.0, 0.0, 0.0);
      float3 uv_or_normal = float3(0.0, 0.0, 0.0);

      if (instancing_enabled == 1) { pos = input.instance_pos + input.pos_size.xyz; color = input.instance_color + input.color;}
      else                         { pos = input.pos_size.xyz; color = input.color; }
      if (shading_type == 0)       { uv_or_normal = input.uv_or_normal; }
      else                         { uv_or_normal = mul(mvp, float4(input.uv_or_normal, 0.0)).xyz; } // TODO(maciej): Normal matrix
    

      vs_out output;
      output.pos = mul( mvp, float4(pos, 1.0f) );
      output.uv_or_normal = uv_or_normal;
      output.color = color;
      return output;
    } 

    float4 ps_main(vs_out input): SV_TARGET {
        if (shading_type == 0)
        {
          float texture_val = font_texture.Sample(font_texture_sampler, input.uv_or_normal.xy).r;
          float alpha = min(input.color.a, texture_val);
          return float4(input.color.rgb, alpha);
        }
        else
        {
          float3 light_dir = float3(0, 0, 1);
          float ndotl = dot( input.uv_or_normal, light_dir);
          return float4(input.color.rgb * ndotl, input.color.a);
        }
    };
  );
}

void
dd__init_point_shader_source( const char** shdr_src )
{
  *shdr_src = DBGDRAW_D3D11_STRINGIFY(
    
    ByteAddressBuffer vertex_data : register(t0);
    ByteAddressBuffer instance_data : register(t1);

    cbuffer vs_uniforms
    {
      float4x4 mvp;
      float4 viewport;
      int shading_type;
      int base_index;
      int instancing_enabled;
    };
  
    struct vs_out {
      float4 pos: SV_POSITION;
      float4 color: COL;
    };

    struct vertex {
      float3 pos;
      float size;
      float3 normal_uv;
      float4 color;
    };

    struct instance {
      float3 pos;
      float4 color;
    };

    float4 unpack_color( uint packed_color )
    {
      uint ru8 = packed_color       & 0xff;
      uint gu8 = packed_color >>  8 & 0xff;
      uint bu8 = packed_color >> 16 & 0xff;
      uint au8 = packed_color >> 24 & 0xff;

      float r = (float)ru8 / 255.0f;
      float g = (float)gu8 / 255.0f;
      float b = (float)bu8 / 255.0f;
      float a = (float)au8 / 255.0f;

      return float4(r,g,b,a);
    }

    vertex load_vertex( uint idx )
    {
      float4 data_a = asfloat(vertex_data.Load4( idx ));
      float3 data_b = asfloat(vertex_data.Load3( idx + 16 ));
      uint data_c = vertex_data.Load(idx + 28);
      
      vertex v;
      v.pos = data_a.xyz;
      v.size = data_a.w;
      v.normal_uv = data_b.xyz;
      v.color = unpack_color( data_c );
      return v;
    }

    instance load_instance( uint idx )
    {
      float3 pos_data = asfloat(instance_data.Load3(idx));
      uint color_data = instance_data.Load(idx+12);

      instance i;
      i.pos = pos_data;
      i.color = unpack_color(color_data);
      return i;
    }

    vs_out vs_main( uint idx : SV_VertexID, uint instance_idx : SV_InstanceID ) {

      float width  = viewport.z - viewport.x;
      float height = viewport.w - viewport.y;
      vertex v = load_vertex( (base_index + idx/6)*32 );
    
      if (instancing_enabled == 1)
      {
        instance i = load_instance( instance_idx * 16 );
        v.pos = v.pos + i.pos;
        v.color = v.color + i.color;
      }

      float2 offsets[6];
      offsets[0] = float2(-v.size, -v.size);// Note(maciej): Divide by 2?
      offsets[1] = float2(-v.size,  v.size);
      offsets[2] = float2( v.size, -v.size);

      offsets[3] = float2(-v.size,  v.size);
      offsets[4] = float2( v.size,  v.size);
      offsets[5] = float2( v.size, -v.size);
      float2 offset = offsets[idx%6];

      vs_out output;
      float4 clip_pos = mul(mvp, float4(v.pos, 1.0f));
      float2 ndc_pos = clip_pos.xy / clip_pos.w;
      float2 viewport_pos = float2(ndc_pos.x*width, ndc_pos.y*height);
      viewport_pos += offset;
      ndc_pos = float2(viewport_pos.x / width, viewport_pos.y / height);
      output.pos = float4( ndc_pos.xy*clip_pos.w, clip_pos.zw );
      output.color = v.color;

      return output;
    } 

    float4 ps_main( vs_out input): SV_TARGET {
      return input.color;
    };
  );
}

#undef DBGDRAW_D3D11_STRINGIFY

#endif /* DBGDRAW_D3D11_H */