#ifndef DBGDRAW_OPENGL45_H
#define DBGDRAW_OPENGL45_H

//TODO(maciej): Don't do bindless, they don't work on Intel


typedef struct dbgdraw_render_backend {
  GLuint base_program;
  GLuint lines_program;
  GLuint lines2_program;
  GLuint vao;
  GLuint vbo;
  GLuint font_tex_attrib_loc;
  GLuint font_tex_ids[16];

  GLuint line_data_texture_id;
} dbgdraw_render_backend_t;


// TODO(maciej): Replace with proper debug.
void dbgdraw__gl_check(const char* filename, uint32_t lineno)
{
  uint32_t error = glGetError();

  if( error != GL_NO_ERROR )
  {
    printf("OGL Error %d at %s: %d\n", error, filename, lineno );
  }

}

#define GLCHECK(x) do{ x; dbgdraw__gl_check(__FILE__, __LINE__); } while(0)

int8_t
dbgdraw__check_gl_program_status( GLuint program_id, bool report_error )
{
  int32_t successful_linking = 0;
  glGetProgramiv( program_id, GL_LINK_STATUS, &successful_linking );
  if( report_error && successful_linking == GL_FALSE )
  {
    int32_t str_length = 0;
    glGetProgramiv( program_id, GL_INFO_LOG_LENGTH, &str_length );
    GLchar* info_log = malloc( str_length );
    glGetProgramInfoLog( program_id, str_length, &str_length, info_log );
    fprintf( stderr, "[GL] Link error :\n%s\n", info_log);
    free( info_log );
    return 1;
  }
  return 0;
}

int8_t
dbgdraw__check_gl_shader_status( GLuint shader_id, bool report_error )
{
  int32_t successful_linking = 0;
  glGetShaderiv( shader_id, GL_COMPILE_STATUS, &successful_linking );
  if( report_error && successful_linking == GL_FALSE )
  {
    int32_t str_length = 0;
    glGetShaderiv( shader_id, GL_INFO_LOG_LENGTH, &str_length );
    GLchar* info_log = malloc( str_length );
    glGetShaderInfoLog( shader_id, str_length, &str_length, info_log );
    fprintf( stderr, "[GL] Compile error :\n%s\n", info_log );
    free( info_log );
    return 1;
  }
  return 0;
}

GLuint
dbgdraw__gl_compile_shader_src( GLuint shader_type, const char* shader_src )
{
  const GLuint shader = glCreateShader( shader_type );
  glShaderSource( shader, 1, &shader_src, NULL );
  glCompileShader( shader );
  int error = dbgdraw__check_gl_shader_status( shader, true );
  if( error ) { exit(-1); }
  return shader;
}

GLuint
dbgdraw__gl_link_program( GLuint vertex_shader, GLuint geometry_shader, GLuint fragment_shader )
{
  GLuint program = glCreateProgram();

  if( vertex_shader )   { glAttachShader( program, vertex_shader ); }
  if( geometry_shader ) { glAttachShader( program, geometry_shader ); }
  if( fragment_shader ) { glAttachShader( program, fragment_shader ); }

  glLinkProgram( program );

  if( vertex_shader )   { glDetachShader( program, vertex_shader );   glDeleteShader(vertex_shader); }
  if( geometry_shader ) { glDetachShader( program, geometry_shader ); glDeleteShader(geometry_shader); }
  if( fragment_shader ) { glDetachShader( program, fragment_shader ); glDeleteShader(fragment_shader); }

  dbgdraw__check_gl_program_status( program, true );
  return program;
}

#define DBGDRAW_SHADER_HEADER "#version 450 core\n"
#define DBGDRAW_STRINGIFY(x) #x

void init_base_shaders_source( const char** vert_shdr, const char** frag_shdr_src );
void init_line_shaders_source( const char** vert_shdr_src, const char** geom_shdr_src, const char** frag_shdr_src );
void init_line_shaders_source2( const char** vert_shdr_src, const char** frag_shdr_src );

int32_t
dbgdraw_backend_init( dbgdraw_ctx_t* ctx )
{
  static dbgdraw_render_backend_t backend = {0};
  ctx->backend = &backend;

  const char* base_vert_shdr_src = NULL;
  const char* base_frag_shdr_src = NULL;
  init_base_shaders_source( &base_vert_shdr_src, &base_frag_shdr_src );

  GLuint vertex_shader   = dbgdraw__gl_compile_shader_src( GL_VERTEX_SHADER, base_vert_shdr_src );
  GLuint fragment_shader = dbgdraw__gl_compile_shader_src( GL_FRAGMENT_SHADER, base_frag_shdr_src );
  ctx->backend->base_program  = dbgdraw__gl_link_program( vertex_shader, 0, fragment_shader );

  const char* line_vert_shdr_src = NULL;
  const char* line_geom_shdr_src = NULL;
  const char* line_frag_shdr_src = NULL;
  init_line_shaders_source( &line_vert_shdr_src, &line_geom_shdr_src, &line_frag_shdr_src );

  GLuint line_vertex_shader    = dbgdraw__gl_compile_shader_src( GL_VERTEX_SHADER, line_vert_shdr_src );
  GLuint line_geometry_shader  = dbgdraw__gl_compile_shader_src( GL_GEOMETRY_SHADER, line_geom_shdr_src );
  GLuint line_fragment_shader  = dbgdraw__gl_compile_shader_src( GL_FRAGMENT_SHADER, line_frag_shdr_src );
  ctx->backend->lines_program  = dbgdraw__gl_link_program( line_vertex_shader, line_geometry_shader, line_fragment_shader );

  const char* line2_vert_shdr_src = NULL;
  const char* line2_frag_shdr_src = NULL;
  init_line_shaders_source2( &line2_vert_shdr_src, &line2_frag_shdr_src );

  GLuint vertex_shader2   = dbgdraw__gl_compile_shader_src( GL_VERTEX_SHADER, line2_vert_shdr_src );
  GLuint fragment_shader2 = dbgdraw__gl_compile_shader_src( GL_FRAGMENT_SHADER, line2_frag_shdr_src );
  ctx->backend->lines2_program  = dbgdraw__gl_link_program( vertex_shader2, 0, fragment_shader2 );

  GLCHECK( glCreateVertexArrays(1, &backend.vao ) );

  GLCHECK( glCreateBuffers( 1, &backend.vbo ) );
  GLCHECK( glNamedBufferStorage( backend.vbo, ctx->verts_cap * sizeof(dbgdraw_vertex_t), NULL, GL_DYNAMIC_STORAGE_BIT ) );

  GLCHECK( glCreateTextures( GL_TEXTURE_BUFFER, 1, &backend.line_data_texture_id ) );
  GLCHECK( glTextureBuffer( backend.line_data_texture_id, GL_RGBA32F, backend.vbo ) );

  GLuint bind_idx = 0;
  GLuint pos_size_loc = glGetAttribLocation( backend.base_program, "in_position_and_size" );
  GLuint uv_loc       = glGetAttribLocation( backend.base_program, "in_uv" );

  GLuint color_loc    = glGetAttribLocation( backend.base_program, "in_color" );

  GLCHECK( glVertexArrayVertexBuffer( backend.vao, bind_idx, backend.vbo, 0, sizeof(dbgdraw_vertex_t) ) );

  GLCHECK( glEnableVertexArrayAttrib( backend.vao, pos_size_loc ) );
  GLCHECK( glEnableVertexArrayAttrib( backend.vao, uv_loc ) );
  GLCHECK( glEnableVertexArrayAttrib( backend.vao, color_loc ) );

  GLCHECK( glVertexArrayAttribFormat( backend.vao, pos_size_loc, 4, GL_FLOAT,         GL_FALSE, offsetof(dbgdraw_vertex_t, pos_size) ) );
  GLCHECK( glVertexArrayAttribFormat( backend.vao, uv_loc,       2, GL_FLOAT,         GL_FALSE, offsetof(dbgdraw_vertex_t, uv) ) );
  GLCHECK( glVertexArrayAttribFormat( backend.vao, color_loc,    4, GL_UNSIGNED_BYTE, GL_TRUE,  offsetof(dbgdraw_vertex_t, col) ) );

  GLCHECK( glVertexArrayAttribBinding( backend.vao, pos_size_loc, bind_idx ) );
  GLCHECK( glVertexArrayAttribBinding( backend.vao, uv_loc, bind_idx ) );
  GLCHECK( glVertexArrayAttribBinding( backend.vao, color_loc, bind_idx ) );

  return DBGDRAW_ERR_OK;
}

// Break this into update and render functions!
int32_t 
dbgdraw_backend_render( dbgdraw_ctx_t *ctx )
{
  if( !ctx->commands_len ) { return DBGDRAW_ERR_OK; }

  // Update the data --> May be unnecessary with persistently mapped buffers
  GLCHECK( glNamedBufferSubData( ctx->backend->vbo, 0, ctx->verts_len * sizeof(dbgdraw_vertex_t), ctx->verts_data ) );
  
  // Setup required ogl state
    // TODO(maciej): Rethink how commands are drawn --> sorting breaks 2d draws, since we do not do any depth testing
  if( ctx->enable_depth_test ) { GLCHECK( glEnable( GL_DEPTH_TEST ) ); }
  GLCHECK( glEnable( GL_BLEND ) );
  GLCHECK( glEnable( GL_LINE_SMOOTH ) );
  GLCHECK( glEnable( GL_PROGRAM_POINT_SIZE ) );
  GLCHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) );
  GLCHECK( glBindVertexArray( ctx->backend->vao ) );
  GLCHECK( glEnable( GL_POLYGON_OFFSET_FILL ) );

  dd_vec2_t viewport_size = dd_vec2( ctx->viewport.data[2] - ctx->viewport.data[0], 
                                     ctx->viewport.data[3] - ctx->viewport.data[1] );

#if 0
  // Sort commands and find indices for switchting
  dbgdraw_sort_commands( ctx );
  int32_t cmd_base_idx[4] = {0};
  int32_t cmd_count[4] = {0};
  for( int i = 0; i < ctx->commands_len - 1; ++i )
  {
    dbgdraw_cmd_t *cmd_a = ctx->commands + i;
    dbgdraw_cmd_t *cmd_b = ctx->commands + (i+1);
    cmd_count[cmd_a->draw_mode]++;
    if( cmd_a->draw_mode != cmd_b->draw_mode )
    {
      cmd_base_idx[cmd_b->draw_mode] = i+1;
    }
  }
  cmd_count[ (ctx->commands + ctx->commands_len - 1)->draw_mode ]++;

  // Draw filled triangles first
  GLCHECK( glPolygonOffset( 2.0, 2.0 ) );
  GLCHECK( glUseProgram( ctx->backend->base_program ) );
  for( int32_t i = 0; i < cmd_count[DBGDRAW_MODE_FILL]; ++i )
  {
    int32_t cmd_idx = cmd_base_idx[DBGDRAW_MODE_FILL] + i;
    dbgdraw_cmd_t *cmd = ctx->commands + cmd_idx;
    dd_mat4_t mvp = dd_mat4_mul(ctx->proj, dd_mat4_mul( ctx->view, cmd->xform ) );
  
    GLCHECK( glUniformMatrix4fv( 0, 1, GL_FALSE, &mvp.data[0] ) );
    GLCHECK( glDrawArrays( GL_TRIANGLES, cmd->base_index, cmd->vertex_count ) );
  }

  // Then draw lines
  GLCHECK( glPolygonOffset( 1.0, 1.0 ) );
  GLCHECK( glUseProgram( ctx->backend->lines_program ) );
  for( int32_t i = 0; i < cmd_count[DBGDRAW_MODE_STROKE]; ++i )
  {
    int32_t cmd_idx = cmd_base_idx[DBGDRAW_MODE_STROKE] + i;
    dbgdraw_cmd_t *cmd = ctx->commands + cmd_idx;
    dd_mat4_t mvp = dd_mat4_mul(ctx->proj, dd_mat4_mul( ctx->view, cmd->xform ) );
    GLCHECK( glUniformMatrix4fv( 0, 1, GL_FALSE, mvp.data ) );
    GLCHECK( glUniform2fv( 1, 1, viewport_size.data ) );
    GLCHECK( glUniform2fv( 2, 1, ctx->aa_radius.data ) );
    GLCHECK( glDrawArrays( GL_LINES, cmd->base_index, cmd->vertex_count ) );
  }
  GLCHECK( glPolygonOffset( 0.0, 0.0 ) );
  GLCHECK( glDisable( GL_POLYGON_OFFSET_FILL ) );

  // Then draw points
  GLCHECK( glUseProgram( ctx->backend->base_program ) );
  for( int32_t i = 0; i < cmd_count[DBGDRAW_MODE_POINT]; ++i )
  {
    int32_t cmd_idx = cmd_base_idx[DBGDRAW_MODE_POINT] + i;
    dbgdraw_cmd_t *cmd = ctx->commands + cmd_idx;
    dd_mat4_t mvp = dd_mat4_mul(ctx->proj, dd_mat4_mul( ctx->view, cmd->xform ) );
    GLCHECK( glUniformMatrix4fv( 0, 1, GL_FALSE, &mvp.data[0] ) );
    GLCHECK( glDrawArrays( GL_POINTS, cmd->base_index, cmd->vertex_count ) );
  }

  // Finally, if enabled, draw text
#if DBGDRAW_HAS_STB_TRUETYPE
  glActiveTexture( GL_TEXTURE0 );
  for( int32_t i = 0; i < cmd_count[DBGDRAW_MODE_TEXT]; ++i )
  {
    int32_t cmd_idx = cmd_base_idx[DBGDRAW_MODE_TEXT] + i;
    dbgdraw_cmd_t *cmd = ctx->commands + cmd_idx;
    dd_mat4_t mvp = dd_mat4_mul(ctx->proj, dd_mat4_mul( ctx->view, cmd->xform ) );
    glBindTexture( GL_TEXTURE_2D, ctx->fonts[ cmd->font_idx ].tex_id ); 
    GLCHECK( glDrawArrays( GL_TRIANGLES, cmd->base_index, cmd->vertex_count ) );
  }
#endif

#else
  static GLenum gl_modes[4];
  gl_modes[DBGDRAW_MODE_FILL]   = GL_TRIANGLES;
  gl_modes[DBGDRAW_MODE_STROKE] = GL_LINES;
  gl_modes[DBGDRAW_MODE_POINT]  = GL_POINTS;
  gl_modes[DBGDRAW_MODE_TEXT]   = GL_TRIANGLES;

  for( int32_t i = 0; i < ctx->commands_len; ++i )
  {
    dbgdraw_cmd_t *cmd = ctx->commands + i;
    dd_mat4_t mvp = dd_mat4_mul(ctx->proj, dd_mat4_mul( ctx->view, cmd->xform ) );

    if( cmd->draw_mode == DBGDRAW_MODE_FILL || cmd->draw_mode == DBGDRAW_MODE_TEXT )
    {
      GLCHECK( glEnable( GL_POLYGON_OFFSET_FILL ) );
      GLCHECK( glPolygonOffset( 2.0, 2.0 ) );
      GLCHECK( glUseProgram( ctx->backend->base_program ) );
      GLCHECK( glUniformMatrix4fv( 0, 1, GL_FALSE, &mvp.data[0] ) );
#if DBGDRAW_HAS_STB_TRUETYPE
      if( cmd->font_idx >= 0 )
      {
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, ctx->fonts[ cmd->font_idx ].tex_id );
        glUniform1i( ctx->backend->font_tex_attrib_loc, 0 );
      }
#endif
      GLCHECK( glDrawArrays( gl_modes[cmd->draw_mode], cmd->base_index, cmd->vertex_count ) );
      GLCHECK( glPolygonOffset( 0.0, 0.0 ) );
      GLCHECK( glDisable( GL_POLYGON_OFFSET_FILL ) );

    }

    else if( cmd->draw_mode == DBGDRAW_MODE_POINT )
    {
      GLCHECK( glUseProgram( ctx->backend->base_program ) );
      GLCHECK( glUniformMatrix4fv( 0, 1, GL_FALSE, &mvp.data[0] ) );

      GLCHECK( glDrawArrays( gl_modes[cmd->draw_mode], cmd->base_index, cmd->vertex_count ) );
    }

    else
    {

      GLCHECK( glActiveTexture( GL_TEXTURE0 ) );
      GLCHECK( glBindTexture( GL_TEXTURE_BUFFER, ctx->backend->line_data_texture_id ) );

      GLCHECK( glEnable( GL_POLYGON_OFFSET_FILL ) );
      GLCHECK( glPolygonOffset( 1.0, 1.0 ) );
      GLCHECK( glUseProgram( ctx->backend->lines2_program ) );

      GLCHECK( glUniformMatrix4fv( 0, 1, GL_FALSE, mvp.data ) );
      GLCHECK( glUniform2fv( 1, 1, viewport_size.data ) );
      GLCHECK( glUniform2fv( 2, 1, ctx->aa_radius.data ) );
      GLCHECK( glUniform1i( 3, 0 ) );
      GLCHECK( glUniform2i( 4, cmd->base_index, cmd->vertex_count ) );
      // For tex buffer lines vbo does not matter.
      GLCHECK( glDrawArrays( GL_TRIANGLES, 0, 3 * cmd->vertex_count ) );
      // GLCHECK( glDrawArrays( gl_modes[cmd->draw_mode], cmd->base_index, cmd->vertex_count ) );
      GLCHECK( glPolygonOffset( 0.0, 0.0 ) );
      GLCHECK( glDisable( GL_POLYGON_OFFSET_FILL ) );
    }
  }
#endif

  // Reset ogl state
  GLCHECK( glPolygonOffset( 0.0, 0.0 ) );
  GLCHECK( glDisable( GL_POLYGON_OFFSET_FILL ) );
  GLCHECK( glUseProgram( 0 ) );
  GLCHECK( glBindVertexArray( 0 ) );
  GLCHECK( glDisable( GL_DEPTH_TEST ) );
  GLCHECK( glDisable( GL_BLEND ) );
  GLCHECK( glDisable( GL_LINE_SMOOTH ) );
  GLCHECK( glDisable( GL_PROGRAM_POINT_SIZE ) );

  return DBGDRAW_ERR_OK;
}

#if DBGDRAW_HAS_STB_TRUETYPE
int32_t
dbgdraw_backend_init_font_texture( dbgdraw_ctx_t* ctx, const uint8_t* data, int32_t width, int32_t height, 
                                                       uint32_t* tex_id )
{
  GLCHECK( glCreateTextures( GL_TEXTURE_2D, 1, &ctx->backend->font_tex_ids[ctx->fonts_len] ) );
  GLCHECK( glTextureParameteri( ctx->backend->font_tex_ids[ctx->fonts_len], GL_TEXTURE_WRAP_S, GL_REPEAT ) );
  GLCHECK( glTextureParameteri( ctx->backend->font_tex_ids[ctx->fonts_len], GL_TEXTURE_WRAP_T, GL_REPEAT ) );
  GLCHECK( glTextureParameteri( ctx->backend->font_tex_ids[ctx->fonts_len], GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
  GLCHECK( glTextureParameteri( ctx->backend->font_tex_ids[ctx->fonts_len], GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );

  GLCHECK( glTextureStorage2D( ctx->backend->font_tex_ids[ctx->fonts_len], 1, GL_R8, width, height ) );
  GLCHECK( glTextureSubImage2D( ctx->backend->font_tex_ids[ctx->fonts_len], 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data ) );
  *tex_id = ctx->backend->font_tex_ids[ctx->fonts_len];

  // ctx->backend->font_tex_handles[ctx->fonts_len] = glGetTextureHandleARB( ctx->backend->font_tex_ids[ctx->fonts_len] );
  // GLCHECK( glMakeTextureHandleResidentARB(ctx->backend->font_tex_handles[ctx->fonts_len] ) ); // Remember to clean-up
  ctx->backend->font_tex_attrib_loc = glGetUniformLocation( ctx->backend->base_program, "tex" );

  return DBGDRAW_ERR_OK;
}
#endif

int32_t
dbgdraw_backend_term( dbgdraw_ctx_t* ctx )
{
  glDeleteVertexArrays( 1, &ctx->backend->vao );
  glDeleteBuffers( 1, &ctx->backend->vbo );
  glDeleteProgram( ctx->backend->base_program );
  glDeleteProgram( ctx->backend->lines_program );
  #if DBGDRAW_HAS_STB_TRUETYPE
    for( int32_t i = 0; i < ctx->fonts_len; ++i )
    {
      // glMakeTextureHandleNonResidentARB( ctx->backend->font_tex_handles[i] );
      glDeleteTextures( 1, &ctx->backend->font_tex_ids[i] );
    }
  #endif
  return DBGDRAW_ERR_OK;
}


void init_base_shaders_source( const char** vert_shdr_src, const char** frag_shdr_src )
{
  *vert_shdr_src = 
    DBGDRAW_SHADER_HEADER
    DBGDRAW_STRINGIFY(
      out gl_PerVertex { vec4 gl_Position;  float gl_PointSize; float gl_ClipDistance[]; };
      layout(location = 0) uniform mat4 u_mvp;

      layout(location = 0) in vec4 in_position_and_size;
      layout(location = 1) in vec2 in_uv;
      layout(location = 2) in vec4 in_color;

      layout(location = 0) out vec4 v_color;
      layout(location = 1) out vec2 v_uv;
      
      void main() {
        v_color = in_color;
        v_uv = in_uv;
        gl_Position = u_mvp * vec4( in_position_and_size.xyz, 1.0 );
        gl_PointSize = in_position_and_size.w;
      }
    );
  
  *frag_shdr_src = 
    DBGDRAW_SHADER_HEADER
    DBGDRAW_STRINGIFY(
      uniform sampler2D tex;

      layout(location = 0) in vec4 v_color;
      layout(location = 1) in vec2 v_uv; 

      layout(location = 0) out vec4 frag_color;

      void main() {
        frag_color = vec4(v_color.rgb, min( 1.0, v_color.a + texture(tex, v_uv).r) );
      }
    );
  
}

void init_line_shaders_source( const char** vert_shdr_src, const char** geom_shdr_src, const char** frag_shdr_src )
{
  *vert_shdr_src = 
    DBGDRAW_SHADER_HEADER
    DBGDRAW_STRINGIFY(
      layout(location = 0) uniform mat4 u_mvp;

      layout(location = 0) in vec4 in_position_and_size;
      layout(location = 1) in vec2 in_uv;
      layout(location = 2) in vec4 in_color;

      layout(location = 0) out vec4 v_color;
      layout(location = 1) out noperspective float v_line_width;

      void main()
      {
        v_color = in_color;
        v_line_width = in_position_and_size.w;
        gl_Position = u_mvp * vec4(in_position_and_size.xyz, 1.0);
      }
    );

  *geom_shdr_src = 
    DBGDRAW_SHADER_HEADER
    DBGDRAW_STRINGIFY(
      layout(lines) in;
      layout(triangle_strip, max_vertices = 4) out;

      layout(location = 1) uniform vec2 u_viewport_size;
      layout(location = 2) uniform vec2 u_aa_radius;

      layout(location = 0) in vec4 v_color[];
      layout(location = 1) in noperspective float v_line_width[];

      out vec4 g_color;
      out noperspective float g_line_width;
      out noperspective float g_line_length;
      out noperspective float g_u;
      out noperspective float g_v;

      void main()
      {
        float u_width        = u_viewport_size[0];
        float u_height       = u_viewport_size[1];
        float u_aspect_ratio = u_height / u_width;

        vec2 ndc_a = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
        vec2 ndc_b = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;

        vec2 line_vector = ndc_b - ndc_a;
        vec2 viewport_line_vector = line_vector * u_viewport_size;
        vec2 dir = normalize(vec2( line_vector.x, line_vector.y * u_aspect_ratio ));

        float line_width_a     = max( 1.0, v_line_width[0] ) + u_aa_radius[0];
        float line_width_b     = max( 1.0, v_line_width[1] ) + u_aa_radius[0];
        float extension_length = u_aa_radius[1];
        float line_length      = length( viewport_line_vector ) + 2.0 * extension_length;
        
        vec2 normal    = vec2( -dir.y, dir.x );
        vec2 normal_a  = vec2( line_width_a/u_width, line_width_a/u_height ) * normal;
        vec2 normal_b  = vec2( line_width_b/u_width, line_width_b/u_height ) * normal;
        vec2 extension = vec2( extension_length / u_width, extension_length / u_height ) * dir;

        g_color = vec4( v_color[0].rgb, v_color[0].a * min( v_line_width[0], 1.0f ) );
        g_u = line_width_a;
        g_v = line_length * 0.5;
        g_line_width = line_width_a;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (ndc_a + normal_a - extension) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw );
        EmitVertex();
        
        g_u = -line_width_a;
        g_v = line_length * 0.5;
        g_line_width = line_width_a;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (ndc_a - normal_a - extension) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw );
        EmitVertex();
        
        g_color = vec4( v_color[0].rgb, v_color[0].a * min( v_line_width[0], 1.0f ) );
        g_u = line_width_b;
        g_v = -line_length * 0.5;
        g_line_width = line_width_b;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (ndc_b + normal_b + extension) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw );
        EmitVertex();
        
        g_u = -line_width_b;
        g_v = -line_length * 0.5;
        g_line_width = line_width_b;
        g_line_length = line_length * 0.5;
        gl_Position = vec4( (ndc_b - normal_b + extension) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw );
        EmitVertex();
        
        EndPrimitive();
      }
    );
  
  *frag_shdr_src = 
    DBGDRAW_SHADER_HEADER
    DBGDRAW_STRINGIFY(
      layout(location = 2) uniform vec2 u_aa_radius;
      
      in vec4 g_color;
      in noperspective float g_u;
      in noperspective float g_v;
      in noperspective float g_line_width;
      in noperspective float g_line_length;

      out vec4 frag_color;
      void main()
      {
        /* We render a quad that is fattened by r, giving total width of the line to be w+r. We want smoothing to happen
           around w, so that the edge is properly smoothed out. As such, in the smoothstep function we have:
           Far edge   : 1.0                                          = (w+r) / (w+r)
           Close edge : 1.0 - (2r / (w+r)) = (w+r)/(w+r) - 2r/(w+r)) = (w-r) / (w+r)
           This way the smoothing is centered around 'w'.
         */
        float au = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[0]) / g_line_width),  1.0, abs(g_u / g_line_width) );
        float av = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[1]) / g_line_length), 1.0, abs(g_v / g_line_length) );
        frag_color = g_color;
        frag_color.a *= min(av, au);
      }
    );
}


void init_line_shaders_source2( const char** vert_shdr_src, const char** frag_shdr_src )
{
  *vert_shdr_src = 
    DBGDRAW_SHADER_HEADER
    DBGDRAW_STRINGIFY(
      
      layout(location = 0) uniform mat4 u_mvp;
      layout(location = 1) uniform vec2 u_viewport_size;
      layout(location = 2) uniform vec2 u_aa_radius;
      layout(location = 3) uniform samplerBuffer u_line_data_sampler;
      layout(location = 4) uniform ivec2 u_command_info;

      out vec4 v_col;
      out noperspective float v_u;
      out noperspective float v_v;
      out noperspective float v_line_width;
      out noperspective float v_line_length;

      vec4 get_vertex_position( int idx, samplerBuffer sampler )
      {
        return texelFetch( sampler, idx );
      }

      vec4 get_vertex_color( int idx, samplerBuffer sampler )
      {
        vec4 tex_sample = texelFetch( sampler, idx );
        uint packed_color = floatBitsToUint( tex_sample.z );
        vec4 color = vec4( float( (packed_color)       & uint(0x000000FF)),
                           float( (packed_color >> 8)  & uint(0x000000FF)),
                           float( (packed_color >> 16) & uint(0x000000FF)),
                           float( (packed_color >> 24) & uint(0x000000FF)) );
        return color / 255.0f;
      }

      ivec3 calculate_segment_ids()
      {
        int base_idx = (gl_VertexID / 6) * 2;
        return ivec3( base_idx - 2, base_idx, base_idx + 2 );
      }

      ivec2 calculate_vertex_ids( int segment_idx, int base_idx )
      {
        return ivec2( base_idx + segment_idx * 2, base_idx + (segment_idx + 1) * 2 );
      }

      void main()
      {
        float u_width        = u_viewport_size[0];
        float u_height       = u_viewport_size[1];
        float u_aspect_ratio = u_height / u_width;
        int   u_base_idx     = u_command_info[0];
        int   u_count        = u_command_info[1];

        // Get indices of line segments
        int base_idx = 2 * u_base_idx;
        ivec3 segment_ids = calculate_segment_ids();
        ivec2 line_ids_0 = calculate_vertex_ids( segment_ids[0], base_idx );
        ivec2 line_ids_1 = calculate_vertex_ids( segment_ids[1], base_idx );
        ivec2 line_ids_2 = calculate_vertex_ids( segment_ids[2], base_idx );

        // Sample data for this line segment
        vec4 pos_width[6] = vec4[6]( vec4(0), vec4(0), vec4(0), vec4(0), vec4(0), vec4(0) );
        if( segment_ids[0] >= 0 )
        {
          pos_width[0] = get_vertex_position( line_ids_0[0], u_line_data_sampler );
          pos_width[1] = get_vertex_position( line_ids_0[1], u_line_data_sampler );
        }
        pos_width[2] = get_vertex_position( line_ids_1[0], u_line_data_sampler );
        pos_width[3] = get_vertex_position( line_ids_1[1], u_line_data_sampler );
        
        if( segment_ids[2] < u_count )
        {
          pos_width[4] = get_vertex_position( line_ids_2[0], u_line_data_sampler );
          pos_width[5] = get_vertex_position( line_ids_2[1], u_line_data_sampler );
        }

        vec4 clip_pos[6];
        clip_pos[0] = u_mvp * vec4( pos_width[0].xyz, 1.0 );
        clip_pos[1] = u_mvp * vec4( pos_width[1].xyz, 1.0 );
        clip_pos[2] = u_mvp * vec4( pos_width[2].xyz, 1.0 );
        clip_pos[3] = u_mvp * vec4( pos_width[3].xyz, 1.0 );
        clip_pos[4] = u_mvp * vec4( pos_width[4].xyz, 1.0 );
        clip_pos[5] = u_mvp * vec4( pos_width[5].xyz, 1.0 );

        vec2 ndc_pos[6];
        ndc_pos[0] = clip_pos[0].xy / clip_pos[0].w;
        ndc_pos[1] = clip_pos[1].xy / clip_pos[1].w;
        ndc_pos[2] = clip_pos[2].xy / clip_pos[2].w;
        ndc_pos[3] = clip_pos[3].xy / clip_pos[3].w;
        ndc_pos[4] = clip_pos[4].xy / clip_pos[4].w;
        ndc_pos[5] = clip_pos[5].xy / clip_pos[5].w;

        float half_w = u_width / 2;
        float half_h = u_height / 2;
        vec2 viewport_pos[6];
        viewport_pos[0] = vec2( ndc_pos[0].x * half_w + half_w, ndc_pos[0].y * half_h + half_h );
        viewport_pos[1] = vec2( ndc_pos[1].x * half_w + half_w, ndc_pos[1].y * half_h + half_h );
        viewport_pos[2] = vec2( ndc_pos[2].x * half_w + half_w, ndc_pos[2].y * half_h + half_h );
        viewport_pos[3] = vec2( ndc_pos[3].x * half_w + half_w, ndc_pos[3].y * half_h + half_h );
        viewport_pos[4] = vec2( ndc_pos[4].x * half_w + half_w, ndc_pos[4].y * half_h + half_h );
        viewport_pos[5] = vec2( ndc_pos[5].x * half_w + half_w, ndc_pos[5].y * half_h + half_h );

        vec2 line_vector_0 = viewport_pos[1] - viewport_pos[0];
        vec2 line_vector_1 = viewport_pos[3] - viewport_pos[2];
        vec2 line_vector_2 = viewport_pos[5] - viewport_pos[4];

        float line_vector_0_length = length( line_vector_0 );
        float line_vector_1_length = length( line_vector_1 );
        float line_vector_2_length = length( line_vector_2 );

        vec2 line_vector_0_unit = line_vector_0 / line_vector_0_length;
        vec2 line_vector_1_unit = line_vector_1 / line_vector_1_length;
        vec2 line_vector_2_unit = line_vector_2 / line_vector_2_length;

        if( line_vector_0_length <= 0.000001 ) { line_vector_0_unit = vec2(0.0, 0.0); line_vector_0_length = 0.0; }
        if( line_vector_1_length <= 0.000001 ) { line_vector_1_unit = vec2(0.0, 0.0); line_vector_1_length = 0.0; }
        if( line_vector_2_length <= 0.000001 )  { line_vector_2_unit = vec2(0.0, 0.0); line_vector_2_length = 0.0; }

        vec2 mitter_0 = line_vector_1_unit + line_vector_0_unit;
        vec2 mitter_1 = line_vector_1_unit + line_vector_2_unit;
        mitter_0 = vec2( -mitter_0.y, mitter_0.x ) * 0.5;
        mitter_1 = vec2( -mitter_1.y, mitter_1.x ) * 0.5;

        vec2 dir    = line_vector_1_unit;
        vec2 normal = vec2( -dir.y, dir.x );

        float mitter_0_length = dot( mitter_0, normal );
        float mitter_1_length = dot( mitter_1, normal );
        float cos_angle_threshold = -0.7;

        if( segment_ids[0] < 0 ||
            length( viewport_pos[2] - viewport_pos[1] ) > 0  ||
            dot( line_vector_0_unit, line_vector_1_unit ) < cos_angle_threshold )
        {
          mitter_0 = normal;
          mitter_0_length = 1;
        }

        if( segment_ids[2] >= u_count || 
            length( viewport_pos[4] - viewport_pos[3]) > 0 ||
            dot( line_vector_2_unit, line_vector_1_unit ) < cos_angle_threshold )
        {
          mitter_1 = normal;
          mitter_1_length = 1;
        }

        float extension_length = u_aa_radius.y;
        float line_length      = line_vector_1_length + 2.0 * extension_length;
        float line_width_a     = max( pos_width[2].w, 1.0 ) + u_aa_radius.x;
        float line_width_b     = max( pos_width[3].w, 1.0 ) + u_aa_radius.x;

        vec2 normal_a  = 0.5 * line_width_a * mitter_0 / mitter_0_length;
        vec2 normal_b  = 0.5 * line_width_b * mitter_1 / mitter_1_length;
        vec2 extension = extension_length * dir;

        int quad_id = gl_VertexID % 6;
        ivec2 quad[6] = ivec2[6]( ivec2(0, -1), ivec2(0, 1), ivec2(1,  1),
                                  ivec2(0, -1), ivec2(1, 1), ivec2(1, -1) );
        ivec2 quad_pos = quad[ quad_id ];

        v_line_width = (1.0 - quad_pos.x) * line_width_a + quad_pos.x * line_width_b;
        v_line_length = 0.5 * line_length;
        v_v = (2.0 * quad_pos.x - 1.0) * v_line_length;
        v_u = (quad_pos.y) * v_line_width;

        vec2 zw_part = (1.0 - quad_pos.x) * clip_pos[2].zw + quad_pos.x * clip_pos[3].zw;
        vec2 dir_y = quad_pos.y * ((1.0 - quad_pos.x) * normal_a + quad_pos.x * normal_b);
        vec2 dir_x = quad_pos.x * line_vector_1 + (2.0 * quad_pos.x - 1.0) * extension;


        vec4 color[2];
        color[0] = get_vertex_color( line_ids_1[0] + 1, u_line_data_sampler );
        color[1] = get_vertex_color( line_ids_1[1] + 1, u_line_data_sampler );

        v_col = color[ quad_pos.x ];
        v_col.a = min( pos_width[2 + quad_pos.x].w * v_col.a, 1.0f );

        vec2 viewport_pt = viewport_pos[2] + dir_x + dir_y;
        vec2 ndc_pt = vec2( (viewport_pt.x - half_w) / half_w, (viewport_pt.y - half_h) / half_h );

        gl_Position = vec4( ndc_pt * zw_part.y, zw_part );
      }
    );
  
  *frag_shdr_src = 
    DBGDRAW_SHADER_HEADER
    DBGDRAW_STRINGIFY(
      layout(location = 2) uniform vec2 u_aa_radius;

      in vec4 v_col;
      in noperspective float v_u;
      in noperspective float v_v;
      in noperspective float v_line_width;
      in noperspective float v_line_length;

      out vec4 frag_color;
      void main()
      {
        float au = 1.0 - smoothstep( 1.0 - ((2.0*u_aa_radius[0]) / v_line_width),  1.0, abs( v_u / v_line_width ) );
        float av = 1.0 - smoothstep( 1.0 - ((u_aa_radius[1]) / v_line_length), 1.0, abs( v_v / v_line_length ) );
        frag_color = v_col;
        frag_color.a *= min(au, av);
      }
    );

}


#endif
