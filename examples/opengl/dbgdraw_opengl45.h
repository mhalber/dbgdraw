#ifndef DBGDRAW_OPENGL45_H
#define DBGDRAW_OPENGL45_H

typedef struct dd_render_backend
{
  GLuint base_program;
  GLuint lines_program;
  GLuint vao;
  GLuint vbo;
  GLuint ibo;
  GLuint font_tex_attrib_loc;
  GLuint font_tex_ids[16];

  GLuint line_data_texture_id;
  size_t vbo_size;
  size_t ibo_size;
} dd_render_backend_t;

void dd__gl_check(const char *filename, uint32_t lineno)
{
  uint32_t error = glGetError();

  if (error != GL_NO_ERROR)
  {
    printf("OGL Error %d at %s: %d\n", error, filename, lineno);
    exit(-1);
  }
}

#define GLCHECK(x)                    \
  do                                  \
  {                                   \
    x;                                \
    dd__gl_check(__FILE__, __LINE__); \
  } while (0)

int8_t
dd__check_gl_program_status(GLuint program_id, bool report_error)
{
  int32_t successful_linking = 0;
  glGetProgramiv(program_id, GL_LINK_STATUS, &successful_linking);
  if (report_error && successful_linking == GL_FALSE)
  {
    int32_t str_length = 0;
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &str_length);
    GLchar *info_log = malloc(str_length);
    glGetProgramInfoLog(program_id, str_length, &str_length, info_log);
    fprintf(stderr, "[GL] Link error :\n%s\n", info_log);
    free(info_log);
    return 1;
  }
  return 0;
}

int8_t
dd__check_gl_shader_status(GLuint shader_id, bool report_error)
{
  int32_t successful_linking = 0;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &successful_linking);
  if (report_error && successful_linking == GL_FALSE)
  {
    int32_t str_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &str_length);
    GLchar *info_log = malloc(str_length);
    glGetShaderInfoLog(shader_id, str_length, &str_length, info_log);
    fprintf(stderr, "[GL] Compile error :\n%s\n", info_log);
    free(info_log);
    return 1;
  }
  return 0;
}

GLuint
dd__gl_compile_shader_src(GLuint shader_type, const char *shader_src)
{
  const GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &shader_src, NULL);
  glCompileShader(shader);
  int32_t error = dd__check_gl_shader_status(shader, true);
  if (error)
  {
    exit(-1);
  }
  return shader;
}

GLuint
dd__gl_link_program(GLuint vertex_shader, GLuint geometry_shader, GLuint fragment_shader)
{
  GLuint program = glCreateProgram();

  if (vertex_shader)
  {
    glAttachShader(program, vertex_shader);
  }
  if (geometry_shader)
  {
    glAttachShader(program, geometry_shader);
  }
  if (fragment_shader)
  {
    glAttachShader(program, fragment_shader);
  }

  glLinkProgram(program);

  if (vertex_shader)
  {
    glDetachShader(program, vertex_shader);
    glDeleteShader(vertex_shader);
  }
  if (geometry_shader)
  {
    glDetachShader(program, geometry_shader);
    glDeleteShader(geometry_shader);
  }
  if (fragment_shader)
  {
    glDetachShader(program, fragment_shader);
    glDeleteShader(fragment_shader);
  }

  dd__check_gl_program_status(program, true);
  return program;
}

#define DBGDRAW_SHADER_HEADER "#version 450 core\n"
#define DBGDRAW_STRINGIFY(x) #x

void dd__init_base_shaders_source(const char **vert_shdr, const char **frag_shdr_src);
void dd__init_line_shaders_source(const char **vert_shdr_src, const char **frag_shdr_src);

int32_t
dd_backend_init(dd_ctx_t *ctx)
{
  static dd_render_backend_t backend = {0};
  ctx->render_backend = &backend;

  const char *base_vert_shdr_src = NULL;
  const char *base_frag_shdr_src = NULL;
  dd__init_base_shaders_source(&base_vert_shdr_src, &base_frag_shdr_src);

  GLuint vertex_shader = dd__gl_compile_shader_src(GL_VERTEX_SHADER, base_vert_shdr_src);
  GLuint fragment_shader = dd__gl_compile_shader_src(GL_FRAGMENT_SHADER, base_frag_shdr_src);
  backend.base_program = dd__gl_link_program(vertex_shader, 0, fragment_shader);

  const char *line_vert_shdr_src = NULL;
  const char *line_frag_shdr_src = NULL;
  dd__init_line_shaders_source(&line_vert_shdr_src, &line_frag_shdr_src);

  GLuint vertex_shader2 = dd__gl_compile_shader_src(GL_VERTEX_SHADER, line_vert_shdr_src);
  GLuint fragment_shader2 = dd__gl_compile_shader_src(GL_FRAGMENT_SHADER, line_frag_shdr_src);
  backend.lines_program = dd__gl_link_program(vertex_shader2, 0, fragment_shader2);

  GLCHECK(glCreateVertexArrays(1, &backend.vao));

  GLCHECK(glCreateBuffers(1, &backend.vbo));
  GLCHECK(glCreateBuffers(1, &backend.ibo));

  backend.vbo_size = ctx->verts_cap * sizeof(dd_vertex_t);
  GLCHECK(glNamedBufferData(backend.vbo, backend.vbo_size, NULL, GL_DYNAMIC_DRAW));
  backend.ibo_size = 512 * sizeof(dd_instance_data_t);
  GLCHECK(glNamedBufferData(backend.ibo, backend.ibo_size, NULL, GL_DYNAMIC_DRAW));

  GLCHECK(glCreateTextures(GL_TEXTURE_BUFFER, 1, &backend.line_data_texture_id));
  GLCHECK(glTextureBuffer(backend.line_data_texture_id, GL_RGBA32F, backend.vbo));

  GLuint bind_idx = 0;
  GLuint pos_size_loc = glGetAttribLocation(backend.base_program, "in_position_and_size");
  GLuint uv_or_normal_loc = glGetAttribLocation(backend.base_program, "in_uv_or_normal");
  GLuint color_loc = glGetAttribLocation(backend.base_program, "in_color");

  GLuint instance_pos_loc = glGetAttribLocation(backend.base_program, "in_instance_pos");
  GLuint instance_col_loc = glGetAttribLocation(backend.base_program, "in_instance_col");

  GLCHECK(glVertexArrayVertexBuffer(backend.vao, bind_idx, backend.vbo, 0, sizeof(dd_vertex_t)));

  GLCHECK(glEnableVertexArrayAttrib(backend.vao, pos_size_loc));
  GLCHECK(glEnableVertexArrayAttrib(backend.vao, uv_or_normal_loc));
  GLCHECK(glEnableVertexArrayAttrib(backend.vao, color_loc));

  GLCHECK(glVertexArrayAttribFormat(backend.vao, pos_size_loc, 4, GL_FLOAT, GL_FALSE, offsetof(dd_vertex_t, pos_size)));
  GLCHECK(glVertexArrayAttribFormat(backend.vao, uv_or_normal_loc, 3, GL_FLOAT, GL_FALSE, offsetof(dd_vertex_t, uv)));
  GLCHECK(glVertexArrayAttribFormat(backend.vao, color_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(dd_vertex_t, col)));

  GLCHECK(glVertexArrayAttribBinding(backend.vao, pos_size_loc, bind_idx));
  GLCHECK(glVertexArrayAttribBinding(backend.vao, uv_or_normal_loc, bind_idx));
  GLCHECK(glVertexArrayAttribBinding(backend.vao, color_loc, bind_idx));

  bind_idx += 1;
  GLCHECK(glVertexArrayVertexBuffer(backend.vao, bind_idx, backend.ibo, 0, sizeof(dd_instance_data_t)));

  GLCHECK(glEnableVertexArrayAttrib(backend.vao, instance_pos_loc));
  GLCHECK(glEnableVertexArrayAttrib(backend.vao, instance_col_loc));

  GLCHECK(glVertexArrayAttribFormat(backend.vao, instance_pos_loc, 3, GL_FLOAT, GL_FALSE, offsetof(dd_instance_data_t, position)));
  GLCHECK(glVertexArrayAttribFormat(backend.vao, instance_col_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(dd_instance_data_t, color)));
  
  GLCHECK(glVertexArrayAttribBinding(backend.vao, instance_pos_loc, bind_idx));
  GLCHECK(glVertexArrayAttribBinding(backend.vao, instance_col_loc, bind_idx));
  
  GLCHECK(glVertexArrayBindingDivisor(backend.vao, bind_idx, 1));

  return DBGDRAW_ERR_OK;
}

int32_t
dd_backend_render(dd_ctx_t *ctx)
{
  assert(ctx);
  assert(ctx->render_backend);
  dd_render_backend_t* backend = ctx->render_backend;

  if (!ctx->commands_len)
  {
    return DBGDRAW_ERR_OK;
  }

  // TODO(maciej): Swap to persitent mapped buffer (glBufferStorage + glMapBufferRange) and measure the performance?
  if (backend->vbo_size < ctx->verts_cap * sizeof(dd_vertex_t))
  {
    backend->vbo_size = ctx->verts_cap * sizeof(dd_vertex_t);
    GLCHECK(glNamedBufferData(backend->vbo, backend->vbo_size, NULL, GL_DYNAMIC_DRAW));
  }
  GLCHECK(glNamedBufferSubData(backend->vbo, 0, ctx->verts_len * sizeof(dd_vertex_t), ctx->verts_data));

  // Setup required ogl state
  if (ctx->enable_depth_test)
  {
    GLCHECK(glEnable(GL_DEPTH_TEST));
  }
  GLCHECK(glEnable(GL_BLEND));
  GLCHECK(glCullFace(GL_BACK));
  GLCHECK(glEnable(GL_CULL_FACE));
  GLCHECK(glEnable(GL_LINE_SMOOTH));
  GLCHECK(glEnable(GL_PROGRAM_POINT_SIZE));
  GLCHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GLCHECK(glBindVertexArray(backend->vao));
  GLCHECK(glEnable(GL_POLYGON_OFFSET_FILL));
  GLCHECK(glPolygonOffset(1.0, 1.0));

  dd_vec2_t viewport_size = dd_vec2(ctx->viewport.data[2], ctx->viewport.data[3]);

  static GLenum gl_modes[3];
  gl_modes[DBGDRAW_MODE_FILL] = GL_TRIANGLES;
  gl_modes[DBGDRAW_MODE_STROKE] = GL_LINES;
  gl_modes[DBGDRAW_MODE_POINT] = GL_POINTS;

  for (int32_t i = 0; i < ctx->commands_len; ++i)
  {
    dd_cmd_t *cmd = ctx->commands + i;
    dd_mat4_t mvp = dd_mat4_mul(ctx->proj, dd_mat4_mul(ctx->view, cmd->xform));

    if (cmd->instance_count && cmd->instance_data)
    {
      if (backend->ibo_size < cmd->instance_count * sizeof(dd_instance_data_t))
      {
        ctx->instance_cap = cmd->instance_count;
        backend->ibo_size = ctx->instance_cap * sizeof(dd_instance_data_t);
        GLCHECK(glNamedBufferData(backend->ibo, backend->ibo_size, NULL, GL_DYNAMIC_DRAW));
      }
      GLCHECK(glNamedBufferSubData(backend->ibo, 0, backend->ibo_size, cmd->instance_data));
    }

    if (cmd->draw_mode == DBGDRAW_MODE_FILL)
    {
      GLCHECK(glUseProgram(backend->base_program));
      GLCHECK(glUniformMatrix4fv(0, 1, GL_FALSE, &mvp.data[0]));
      GLCHECK(glUniform1i(1, cmd->shading_type));
      GLCHECK(glUniform1i(2, (cmd->instance_count>0) ));

#if DBGDRAW_HAS_TEXT_SUPPORT
      if (cmd->font_idx >= 0)
      {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ctx->fonts[cmd->font_idx].tex_id);
        glUniform1i(backend->font_tex_attrib_loc, 0);
      }
#endif
      if (cmd->instance_count <= 0)
      {
        GLCHECK(glDrawArrays(gl_modes[cmd->draw_mode], cmd->base_index, cmd->vertex_count));
      }
      else
      {
        GLCHECK(glDrawArraysInstanced(gl_modes[cmd->draw_mode], cmd->base_index, cmd->vertex_count, cmd->instance_count ));
      }
    }

    else if (cmd->draw_mode == DBGDRAW_MODE_POINT)
    {
      GLCHECK(glUseProgram(backend->base_program));
      GLCHECK(glUniformMatrix4fv(0, 1, GL_FALSE, &mvp.data[0]));
      GLCHECK(glUniform1i(1, 0));
      GLCHECK(glUniform1i(2, (int)(cmd->instance_count>0) ));

      if (cmd->instance_count <= 0)
      {
        GLCHECK(glDrawArrays(gl_modes[cmd->draw_mode], cmd->base_index, cmd->vertex_count));
      }
      else
      {
        GLCHECK(glDrawArraysInstanced(gl_modes[cmd->draw_mode], cmd->base_index, cmd->vertex_count, cmd->instance_count ));
      }
    }

    else
    {
      GLCHECK(glActiveTexture(GL_TEXTURE0));
      GLCHECK(glBindTexture(GL_TEXTURE_BUFFER, backend->line_data_texture_id));

      GLCHECK(glUseProgram(backend->lines_program));

      GLCHECK(glUniformMatrix4fv(0, 1, GL_FALSE, mvp.data));
      GLCHECK(glUniform2fv(1, 1, viewport_size.data));
      GLCHECK(glUniform2fv(2, 1, ctx->aa_radius.data));
      GLCHECK(glUniform1i(3, 0));
      GLCHECK(glUniform2i(4, cmd->base_index, cmd->vertex_count));
      GLCHECK(glUniform1i(5, (cmd->instance_count>0) ));

      // For tex buffer lines vbo does not matter.
      if (cmd->instance_count <= 0)
      {
        GLCHECK(glDrawArrays(GL_TRIANGLES, 0, 3 * cmd->vertex_count));
      }
      else
      {
        GLCHECK(glDrawArraysInstanced(GL_TRIANGLES, 0, 3 * cmd->vertex_count, cmd->instance_count ));
      }
    }
  }

  // Reset ogl state
  GLCHECK(glPolygonOffset(0.0, 0.0));
  GLCHECK(glDisable(GL_POLYGON_OFFSET_FILL));
  GLCHECK(glUseProgram(0));
  GLCHECK(glBindVertexArray(0));
  GLCHECK(glDisable(GL_DEPTH_TEST));
  GLCHECK(glDisable(GL_BLEND));
  GLCHECK(glDisable(GL_LINE_SMOOTH));
  GLCHECK(glDisable(GL_PROGRAM_POINT_SIZE));

  return DBGDRAW_ERR_OK;
}

#if DBGDRAW_HAS_TEXT_SUPPORT
int32_t
dd_backend_init_font_texture(dd_ctx_t *ctx, const uint8_t *data, int32_t width, int32_t height,
                             uint32_t *tex_id)
{
  assert(ctx);
  assert(ctx->render_backend);
  dd_render_backend_t* backend = ctx->render_backend;

  GLCHECK(glCreateTextures(GL_TEXTURE_2D, 1, &backend->font_tex_ids[ctx->fonts_len]));
  GLCHECK(glTextureParameteri(backend->font_tex_ids[ctx->fonts_len], GL_TEXTURE_WRAP_S, GL_REPEAT));
  GLCHECK(glTextureParameteri(backend->font_tex_ids[ctx->fonts_len], GL_TEXTURE_WRAP_T, GL_REPEAT));
  GLCHECK(glTextureParameteri(backend->font_tex_ids[ctx->fonts_len], GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GLCHECK(glTextureParameteri(backend->font_tex_ids[ctx->fonts_len], GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  GLCHECK(glTextureStorage2D(backend->font_tex_ids[ctx->fonts_len], 1, GL_R8, width, height));
  GLCHECK(glTextureSubImage2D(backend->font_tex_ids[ctx->fonts_len], 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data));
  *tex_id = backend->font_tex_ids[ctx->fonts_len];
  backend->font_tex_attrib_loc = glGetUniformLocation(backend->base_program, "tex");

  return DBGDRAW_ERR_OK;
}
#endif

int32_t
dd_backend_term(dd_ctx_t *ctx)
{
  assert(ctx);
  assert(ctx->render_backend);
  dd_render_backend_t* backend = ctx->render_backend;

  glDeleteVertexArrays(1, &backend->vao);
  glDeleteBuffers(1, &backend->vbo);
  glDeleteProgram(backend->base_program);
  glDeleteProgram(backend->lines_program);
#if DBGDRAW_HAS_TEXT_SUPPORT
  for (int32_t i = 0; i < ctx->fonts_len; ++i)
  {
    glDeleteTextures(1, &backend->font_tex_ids[i]);
  }
#endif
  return DBGDRAW_ERR_OK;
}

void dd__init_base_shaders_source(const char **vert_shdr_src, const char **frag_shdr_src)
{
  *vert_shdr_src =
    DBGDRAW_SHADER_HEADER
    DBGDRAW_STRINGIFY(
      layout(location = 0) uniform mat4 u_mvp;
      layout(location = 1) uniform int shading_type;
      layout(location = 2) uniform bool instancing_enabled;

      layout(location = 0) in vec4 in_position_and_size;
      layout(location = 1) in vec3 in_uv_or_normal;
      layout(location = 2) in vec4 in_color;
      layout(location = 3) in vec3 in_instance_pos;
      layout(location = 4) in vec4 in_instance_col;
      
      layout(location = 0) out vec4 v_color;
      layout(location = 1) out vec3 v_uv_or_normal;
      layout(location = 2) out flat int v_shading_type;

      void main() {
        if (instancing_enabled) { v_color = in_color + in_instance_col; }
        else                    { v_color = in_color; }
        if (shading_type == 0)
        {
          v_uv_or_normal = in_uv_or_normal;
        }
        else
        {
          // TODO(maciej): Normal matrix
          v_uv_or_normal = vec3(u_mvp * vec4(in_uv_or_normal, 0.0));
        }
        v_shading_type = shading_type;
        if (instancing_enabled) { gl_Position = u_mvp * vec4(in_position_and_size.xyz + in_instance_pos, 1.0); }
        else                    { gl_Position = u_mvp * vec4(in_position_and_size.xyz, 1.0); }
        gl_PointSize = in_position_and_size.w;
      });

  *frag_shdr_src =
    DBGDRAW_SHADER_HEADER
    DBGDRAW_STRINGIFY(
      uniform sampler2D tex;

      layout(location = 0) in vec4 v_color;
      layout(location = 1) in vec3 v_uv_or_normal;
      layout(location = 2) in flat int v_shading_type;

      layout(location = 0) out vec4 frag_color;

      void main() {
        if (v_shading_type == 0)
        {
          frag_color = vec4(v_color.rgb, min(1.0, v_color.a + texture(tex, vec2(v_uv_or_normal)).r));
        }
        else
        {
          vec3 light_dir = vec3(0, 0, 1);
          float ndotl = dot(v_uv_or_normal, light_dir);
          frag_color = vec4(v_color.rgb * ndotl, v_color.a);
        }
      });
}

void dd__init_line_shaders_source(const char **vert_shdr_src, const char **frag_shdr_src)
{
  *vert_shdr_src =
    DBGDRAW_SHADER_HEADER
    DBGDRAW_STRINGIFY(

      layout(location = 3) in vec3 in_instance_pos;
      layout(location = 4) in vec4 in_instance_col;

      layout(location = 0) uniform mat4 u_mvp;
      layout(location = 1) uniform vec2 u_viewport_size;
      layout(location = 2) uniform vec2 u_aa_radius;
      layout(location = 3) uniform samplerBuffer u_line_data_sampler;
      layout(location = 4) uniform ivec2 u_command_info;
      layout(location = 5) uniform bool instancing_enabled;

      out vec4 v_col;
      out noperspective float v_u;
      out noperspective float v_v;
      out noperspective float v_line_width;
      out noperspective float v_line_length;

      vec4 get_vertex_position(int idx, samplerBuffer sampler) {
        return texelFetch(sampler, idx);
      }

      vec4 get_vertex_color(int idx, samplerBuffer sampler) {
        vec4 tex_sample = texelFetch(sampler, idx);
        uint packed_color = floatBitsToUint(tex_sample.w);
        vec4 color = vec4(float((packed_color)&uint(0x000000FF)),
                          float((packed_color >> 8) & uint(0x000000FF)),
                          float((packed_color >> 16) & uint(0x000000FF)),
                          float((packed_color >> 24) & uint(0x000000FF)));
        return color / 255.0f;
      }

      ivec3 calculate_segment_ids() {
        int base_idx = (gl_VertexID / 6) * 2;
        return ivec3(base_idx - 2, base_idx, base_idx + 2);
      }

      ivec2 calculate_vertex_ids(int segment_idx, int base_idx) {
        return ivec2(base_idx + segment_idx * 2, base_idx + (segment_idx + 1) * 2);
      }

      void main() {
        float u_width = u_viewport_size[0];
        float u_height = u_viewport_size[1];
        float u_aspect_ratio = u_height / u_width;
        int u_base_idx = u_command_info[0];
        int u_count = u_command_info[1];

        // Get indices of line segments
        int base_idx = 2 * u_base_idx;
        ivec3 segment_ids = calculate_segment_ids();
        ivec2 line_ids_0 = calculate_vertex_ids(segment_ids[0], base_idx);
        ivec2 line_ids_1 = calculate_vertex_ids(segment_ids[1], base_idx);
        ivec2 line_ids_2 = calculate_vertex_ids(segment_ids[2], base_idx);

        // Sample data for this line segment
        vec4 pos_width[6] = vec4[6](vec4(0), vec4(0), vec4(0), vec4(0), vec4(0), vec4(0));
        if (segment_ids[0] >= 0)
        {
          pos_width[0] = get_vertex_position(line_ids_0[0], u_line_data_sampler);
          pos_width[1] = get_vertex_position(line_ids_0[1], u_line_data_sampler);
        }
        pos_width[2] = get_vertex_position(line_ids_1[0], u_line_data_sampler);
        pos_width[3] = get_vertex_position(line_ids_1[1], u_line_data_sampler);

        if (segment_ids[2] < u_count)
        {
          pos_width[4] = get_vertex_position(line_ids_2[0], u_line_data_sampler);
          pos_width[5] = get_vertex_position(line_ids_2[1], u_line_data_sampler);
        }

        if (instancing_enabled)
        { 
          pos_width[0] = pos_width[0] + vec4(in_instance_pos, 0.0);
          pos_width[1] = pos_width[1] + vec4(in_instance_pos, 0.0);
          pos_width[2] = pos_width[2] + vec4(in_instance_pos, 0.0);
          pos_width[3] = pos_width[3] + vec4(in_instance_pos, 0.0);
          pos_width[4] = pos_width[4] + vec4(in_instance_pos, 0.0);
          pos_width[5] = pos_width[5] + vec4(in_instance_pos, 0.0);
        }

        vec4 clip_pos[6];
        clip_pos[0] = u_mvp * vec4(pos_width[0].xyz, 1.0);
        clip_pos[1] = u_mvp * vec4(pos_width[1].xyz, 1.0);
        clip_pos[2] = u_mvp * vec4(pos_width[2].xyz, 1.0);
        clip_pos[3] = u_mvp * vec4(pos_width[3].xyz, 1.0);
        clip_pos[4] = u_mvp * vec4(pos_width[4].xyz, 1.0);
        clip_pos[5] = u_mvp * vec4(pos_width[5].xyz, 1.0);

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
        viewport_pos[0] = vec2(ndc_pos[0].x * half_w + half_w, ndc_pos[0].y * half_h + half_h);
        viewport_pos[1] = vec2(ndc_pos[1].x * half_w + half_w, ndc_pos[1].y * half_h + half_h);
        viewport_pos[2] = vec2(ndc_pos[2].x * half_w + half_w, ndc_pos[2].y * half_h + half_h);
        viewport_pos[3] = vec2(ndc_pos[3].x * half_w + half_w, ndc_pos[3].y * half_h + half_h);
        viewport_pos[4] = vec2(ndc_pos[4].x * half_w + half_w, ndc_pos[4].y * half_h + half_h);
        viewport_pos[5] = vec2(ndc_pos[5].x * half_w + half_w, ndc_pos[5].y * half_h + half_h);

        vec2 line_vector_0 = viewport_pos[1] - viewport_pos[0];
        vec2 line_vector_1 = viewport_pos[3] - viewport_pos[2];
        vec2 line_vector_2 = viewport_pos[5] - viewport_pos[4];

        float line_vector_0_length = length(line_vector_0);
        float line_vector_1_length = length(line_vector_1);
        float line_vector_2_length = length(line_vector_2);

        vec2 line_vector_0_unit = line_vector_0 / line_vector_0_length;
        vec2 line_vector_1_unit = line_vector_1 / line_vector_1_length;
        vec2 line_vector_2_unit = line_vector_2 / line_vector_2_length;

        if (line_vector_0_length <= 0.000001)
        {
          line_vector_0_unit = vec2(0.0, 0.0);
          line_vector_0_length = 0.0;
        }
        if (line_vector_1_length <= 0.000001)
        {
          line_vector_1_unit = vec2(0.0, 0.0);
          line_vector_1_length = 0.0;
        }
        if (line_vector_2_length <= 0.000001)
        {
          line_vector_2_unit = vec2(0.0, 0.0);
          line_vector_2_length = 0.0;
        }

        vec2 mitter_0 = line_vector_1_unit + line_vector_0_unit;
        vec2 mitter_1 = line_vector_1_unit + line_vector_2_unit;
        mitter_0 = vec2(-mitter_0.y, mitter_0.x) * 0.5;
        mitter_1 = vec2(-mitter_1.y, mitter_1.x) * 0.5;

        vec2 dir = line_vector_1_unit;
        vec2 normal = vec2(-dir.y, dir.x);

        float mitter_0_length = dot(mitter_0, normal);
        float mitter_1_length = dot(mitter_1, normal);
        float cos_angle_threshold = -0.7;

        if (segment_ids[0] < 0 ||
            length(viewport_pos[2] - viewport_pos[1]) > 0 ||
            dot(line_vector_0_unit, line_vector_1_unit) < cos_angle_threshold)
        {
          mitter_0 = normal;
          mitter_0_length = 1;
        }

        if (segment_ids[2] >= u_count ||
            length(viewport_pos[4] - viewport_pos[3]) > 0 ||
            dot(line_vector_2_unit, line_vector_1_unit) < cos_angle_threshold)
        {
          mitter_1 = normal;
          mitter_1_length = 1;
        }

        float extension_length = u_aa_radius.y;
        float line_length = line_vector_1_length + 2.0 * extension_length;
        float line_width_a = max(pos_width[2].w, 1.0) + u_aa_radius.x;
        float line_width_b = max(pos_width[3].w, 1.0) + u_aa_radius.x;

        vec2 normal_a = 0.5 * line_width_a * mitter_0 / mitter_0_length;
        vec2 normal_b = 0.5 * line_width_b * mitter_1 / mitter_1_length;
        vec2 extension = extension_length * dir;

        int quad_id = gl_VertexID % 6;
        ivec2 quad[6] = ivec2[6](ivec2(0, -1), ivec2(1, 1), ivec2(0, 1),
                                 ivec2(0, -1), ivec2(1, -1), ivec2(1, 1));
        ivec2 quad_pos = quad[quad_id];

        v_line_width = (1.0 - quad_pos.x) * line_width_a + quad_pos.x * line_width_b;
        v_line_length = 0.5 * line_length;
        v_v = (2.0 * quad_pos.x - 1.0) * v_line_length;
        v_u = (quad_pos.y) * v_line_width;

        vec2 zw_part = (1.0 - quad_pos.x) * clip_pos[2].zw + quad_pos.x * clip_pos[3].zw;
        vec2 dir_y = quad_pos.y * ((1.0 - quad_pos.x) * normal_a + quad_pos.x * normal_b);
        vec2 dir_x = quad_pos.x * line_vector_1 + (2.0 * quad_pos.x - 1.0) * extension;

        vec4 color[2];
        color[0] = get_vertex_color(line_ids_1[0] + 1, u_line_data_sampler);
        color[1] = get_vertex_color(line_ids_1[1] + 1, u_line_data_sampler);

        if (instancing_enabled)
        {
          color[0] += in_instance_col;
          color[1] += in_instance_col;
        }
        
        v_col = color[quad_pos.x];
        v_col.a = min(pos_width[2 + quad_pos.x].w * v_col.a, 1.0f);

        vec2 viewport_pt = viewport_pos[2] + dir_x + dir_y;
        vec2 ndc_pt = vec2((viewport_pt.x - half_w) / half_w, (viewport_pt.y - half_h) / half_h);

        gl_Position = vec4(ndc_pt * zw_part.y, zw_part);
      });

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
      void main() {
        float au = 1.0 - smoothstep(1.0 - ((2.0 * u_aa_radius.x) / v_line_width), 1.0, abs(v_u / v_line_width));
        float av = 1.0 - smoothstep(1.0 - (u_aa_radius.y / v_line_length), 1.0, abs(v_v / v_line_length));
        frag_color = v_col;
        frag_color.a *= min(au, av);
      });
}

#endif
