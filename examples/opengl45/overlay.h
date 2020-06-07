
void
draw_legend(dd_ctx_t *ctx, char *legend_string, int32_t x, int32_t y)
{
  float px = (float)x;
  float py = y + 0.5f;

  dd_begin_cmd(ctx, DBGDRAW_MODE_FILL);
  dd_text_info_t info = {.vert_align = DBGDRAW_TEXT_TOP};
  char *start, *end;
  start = end = legend_string;
  while (true)
  {
    while (*end != '\n' && *end != 0)
    {
      end++;
    }
    char tmp = *end;

    *end = 0;

    dd_set_color(ctx, DBGDRAW_WHITE);
    dd_text_line(ctx, msh_vec3(px, py, 0.0).data, start, &info);
    py -= (info.height + 4);

    *end = tmp;
    if (*end == 0) { break; }
    start = ++end;
  }
  dd_end_cmd(ctx);
}

int32_t
draw_frame_timer(dd_ctx_t *ctx, int32_t x, int32_t y,
                 float *frame_times, int32_t n_times, int32_t frame_time_idx)
{
  float min_x = (float)x;
  float min_y = (float)y;
  float max_x = min_x + 250;
  float max_y = min_y + 80;

  dd_begin_cmd(ctx, DBGDRAW_MODE_FILL);
  dd_color_t col = DBGDRAW_ORANGE;
  col.a = 125;
  dd_set_color(ctx, col);
  dd_quad(ctx,  msh_vec3(min_x, min_y, 0.0f).data, msh_vec3(max_x, min_y, 0.0f).data,
                msh_vec3(max_x, max_y, 0.0f).data, msh_vec3(min_x, max_y, 0.0f).data);

  col.a = 255;
  dd_set_color(ctx, col);
  dd_quad(ctx, msh_vec3(min_x, min_y, 0.0f).data, msh_vec3(max_x, min_y, 0.0f).data,
               msh_vec3(max_x, min_y + 20.0f, 0.0f).data, msh_vec3(min_x, min_y + 20.0f, 0.0f).data);
  dd_end_cmd(ctx);

  dd_begin_cmd(ctx, DBGDRAW_MODE_STROKE);
  dd_set_color(ctx, col);
  dd_set_primitive_size(ctx, 2.0f);

  min_y += 20.0f;
  for (int32_t i = 0; i < n_times - 1; ++i)
  {
    int32_t idx1 = (frame_time_idx + i) % n_times;
    int32_t idx2 = (frame_time_idx + i + 1) % n_times;

    static float min = 0;
    static float max = 33;

    float t1 = msh_clamp01((frame_times[idx1] - min) / (max - min));
    float t2 = msh_clamp01((frame_times[idx2] - min) / (max - min));
    float cur_y1 = min_y + t1 * (max_y - min_y);
    float cur_y2 = min_y + t2 * (max_y - min_y);
    float cur_x1 = min_x + ((i) * (max_x - min_x) / (float)(n_times - 1));
    float cur_x2 = min_x + ((i + 1) * (max_x - min_x) / (float)(n_times - 1));

    dd_line(ctx, msh_vec3(cur_x1, cur_y1, 0.0).data, msh_vec3(cur_x2, cur_y2, 0).data);
  }
  dd_end_cmd(ctx);

  float dd_time = msh_compute_mean(frame_times, n_times);

  char dd_time_buf[128];
  snprintf(dd_time_buf, 128, "Frame Time %4.3fms (%5.1f FPS)", dd_time, 1000.0 / dd_time);
  dd_begin_cmd(ctx, DBGDRAW_MODE_FILL);
  dd_text_info_t alignment = {.vert_align = DBGDRAW_TEXT_TOP, .horz_align = DBGDRAW_TEXT_RIGHT};
  dd_set_color(ctx, dd_rgbf(0.1f, 0.1f, 0.3f));
  dd_text_line(ctx, msh_vec3(max_x - 5.5f, min_y - 5.5f, 0.0).data, dd_time_buf, &alignment);
  dd_end_cmd(ctx);

  return DBGDRAW_ERR_OK;
}