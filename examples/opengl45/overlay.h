int32_t
draw_legend( dbgdraw_ctx_t* ctx, char* legend_string, int x, int y )
{
  float px = x;
  float py = y+0.5;
  int32_t error = DBGDRAW_ERR_OK;
  error = dbgdraw_begin_cmd( ctx, DBGDRAW_MODE_TEXT );
  if( error ) { return error; }
  dbgdraw_text_info_t info = { .vert_align = DBGDRAW_TEXT_TOP };
  char* start, *end;
  start = end = legend_string;
  while( true )
  {
    while( *end != '\n' && *end != 0 ) { end++; }
    
    char tmp = *end;
    *end = 0;

    error = dbgdraw_set_color( ctx, DBGDRAW_WHITE );
    if( error ) { return error; }
    error = dbgdraw_text( ctx, msh_vec3( px, py, 0.0).data, start, &info );
    if( error ) { return error; }
    py -= (info.height + 4);
    
    *end = tmp;
    if( *end == 0 ) { break; }
    start = end++;
  }
  error = dbgdraw_end_cmd(ctx);
  return error;
}

int32_t
draw_frame_timer( dbgdraw_ctx_t* ctx, int32_t x, int32_t y,
                  float* frame_times, int32_t n_times, int32_t frame_time_idx )
{
  int32_t min_x = x;
  int32_t min_y = y;
  int32_t max_x = min_x + 250;
  int32_t max_y = min_y + 80;

  dbgdraw_begin_cmd( ctx, DBGDRAW_MODE_FILL );
  dd_color_t col = DBGDRAW_ORANGE;
  col.a = 125;
  dbgdraw_set_color( ctx, col );
  dbgdraw_quad( ctx, msh_vec3(min_x, min_y, 0.0f).data, msh_vec3(max_x, min_y, 0.0f).data,
                     msh_vec3(max_x, max_y, 0.0f).data, msh_vec3(min_x, max_y, 0.0f).data );

  col.a = 255;
  dbgdraw_set_color( ctx, col );
  dbgdraw_quad( ctx, msh_vec3(min_x, min_y, 0.0f).data,       msh_vec3(max_x, min_y, 0.0f).data,
                     msh_vec3(max_x, min_y+20.0f, 0.0f).data, msh_vec3(min_x, min_y+20.0, 0.0f).data );
  dbgdraw_end_cmd( ctx );

  dbgdraw_begin_cmd( ctx, DBGDRAW_MODE_STROKE );
  dbgdraw_set_color( ctx, col );
  dbgdraw_set_primitive_size( ctx, 2.0f );

  min_y += 20.0f;
  for( int32_t i = 0; i < n_times-1; ++i )
  {
    int32_t idx1 = (frame_time_idx + i) % n_times;
    int32_t idx2 = (frame_time_idx + i + 1) % n_times;

    static float min = 0;
    static float max = 33;

    float t1 = msh_clamp01( (frame_times[idx1] - min) / (max - min) );
    float t2 = msh_clamp01( (frame_times[idx2] - min) / (max - min) );
    float cur_y1 = min_y + t1*(max_y - min_y);
    float cur_y2 = min_y + t2*(max_y - min_y);
    float cur_x1 = min_x + ((i) * (max_x - min_x) / (float)(n_times-1));
    float cur_x2 = min_x + ((i+1) * (max_x - min_x) / (float)(n_times-1));
    
    dbgdraw_line( ctx, msh_vec3( cur_x1, cur_y1, 0.0 ).data, msh_vec3( cur_x2, cur_y2, 0 ).data );
  }
  dbgdraw_end_cmd( ctx );

  float dbgdraw_time = msh_compute_mean( frame_times, n_times );
  
  char dd_time_buf[128];
  snprintf(dd_time_buf, 128, "Frame Time %4.3fms (%5.1f FPS)", dbgdraw_time, 1000.0 / dbgdraw_time );
  dbgdraw_begin_cmd( ctx, DBGDRAW_MODE_TEXT );
  dbgdraw_text_info_t alignment = { .vert_align = DBGDRAW_TEXT_TOP, .horz_align = DBGDRAW_TEXT_RIGHT };
  dbgdraw_set_color( ctx, dbgdraw_rgbf( 0.1f, 0.1f, 0.3f ) );
  dbgdraw_text( ctx, msh_vec3( max_x-5.5f, min_y-5.5f, 0.0).data, dd_time_buf, &alignment );
  dbgdraw_end_cmd( ctx );

  return DBGDRAW_ERR_OK;
}