static const char *PROGRAM_NAME = "dbgdraw_vector_field";

#define MSH_STD_INCLUDE_LIBC_HEADERS
#define MSH_STD_IMPLEMENTATION
#define MSH_VEC_MATH_IMPLEMENTATION
#define GLFW_INCLUDE_NONE

#include "msh_std.h"
#include "msh_vec_math.h"
#include "stb_truetype.h"
#include "dbgdraw.h"

#include "GLFW/glfw3.h"
#include "glad.h"
#include "dbgdraw_opengl45.h"

typedef struct {
    GLFWwindow* window;
    dd_ctx_t* dd_ctx;
} app_state_t;


int32_t init( app_state_t* state );
void frame( app_state_t* state );
void cleanup( app_state_t* state );


#define SIZE_X 17
#define SIZE_Y 9
static float field_data[SIZE_Y][SIZE_X] = {{0}};

int32_t
main( void )
{
    int32_t error      = 0;
    app_state_t* state = calloc( 1, sizeof(app_state_t) );
    
    error = init( state );
    if( error ) { goto main_return; }
    
    GLFWwindow* window    = state->window;
    
    while( !glfwWindowShouldClose( window ) )
    {
        frame( state );
        
        glfwSwapBuffers( window );
        glfwPollEvents();
    }
    
    main_return:
    cleanup( state );
    return error;
}

int32_t 
init( app_state_t* state ) {
    int32_t error = 0;
    
    error = !(glfwInit());
    if( error )
    {
        fprintf( stderr, "[ERROR] Failed to initialize GLFW library!\n" );
        return 1;
    }
    
    int32_t win_width = 640, win_height = 320;
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 5 );
    glfwWindowHint( GLFW_RESIZABLE, GL_FALSE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_SAMPLES, 4 );
    state->window = glfwCreateWindow( win_width, win_height, PROGRAM_NAME, NULL, NULL );
    if( !state->window )
    {
        fprintf( stderr, "[ERROR] Failed to create window\n" );
        return 1;
    }
    
    glfwMakeContextCurrent( state->window );
    if( !gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) )
    {
        fprintf( stderr, "[ERROR] Failed to initialize OpenGL context!\n" );
        return 1;
    }
    
    state->dd_ctx = calloc( 1, sizeof(dd_ctx_t) );
    dd_ctx_desc_t desc = 
    { .max_vertices = 1024,
        .max_commands = 16,
        .detail_level = 2};
    error = dd_init( state->dd_ctx, &desc );
    if( error )
    {
        fprintf( stderr, "[ERROR] Failed to initialize dbgdraw library!\n" );
        return 1;
    }
    
    return 0;
}


void 
frame(app_state_t* state) 
{
    GLFWwindow* window    = state->window;
    dd_ctx_t* dd_ctx = state->dd_ctx;
    
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glClearColor( 0.9f, 0.9f, 0.95f, 1.0f );
    
    int32_t w, h;
    glfwGetWindowSize( window, &w, &h );
    
    msh_vec3_t cam_pos = msh_vec3(0, 0, 5);
    msh_mat4_t view = msh_look_at( cam_pos, msh_vec3_zeros(), msh_vec3_posy() );
    msh_vec4_t viewport = msh_vec4( 0.0f, 0.0f, (float)w, (float)h );
    msh_mat4_t proj = msh_ortho( 0.0f, (float)w, 0.0f, (float)h, 0.01f, 10.0f );
    dd_new_frame_info_t info = 
    { .view_matrix       = view.data,
        .projection_matrix = proj.data,
        .viewport_size     = viewport.data,
        .vertical_fov      = (float)h,
        .projection_type   = DBGDRAW_ORTHOGRAPHIC };
    dd_new_frame( dd_ctx, &info );
    
    dd_ctx->aa_radius = dd_vec2( 2.0f, 2.0f );
    dd_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );
    
    // Fill in function data
    for (int y = 0; y < SIZE_Y; ++y)
    {
        for (int x = 0; x < SIZE_X; ++x)
        {
            float t = (float)glfwGetTime();
            field_data[y][x] = sinf(x/2.0f + t ) + cosf(y/2.0f + t);
        }
    }
    
    float padding = 32.0f;
    float base_x = padding;
    float base_y = padding;
    float delta_x = (w - 2.0f*padding) / (SIZE_X-2.0f);
    float delta_y = (h - 2.0f*padding) / (SIZE_Y-2.0f);
    float pos_a[2] = {0};
    float pos_b[2] = {0};
    float pos_c[2] = {0};
    dd_ctx->aa_radius = dd_vec2(2.0f, 0.0f);
    for (int y = 1; y < SIZE_Y; ++y)
    {
        for (int x = 1; x < SIZE_X; ++x)
        {
            float val_a = field_data[y][x];
            pos_a[0] = base_x + (x-1) * delta_x;
            pos_a[1] = base_y + (y-1) * delta_y;
            
            dd_set_color(dd_ctx, DBGDRAW_BLACK);
            float val_b = field_data[y][x-1];
            float val_c = field_data[y-1][x];
            float dx = val_b - val_a;
            float dy = val_c - val_a;
            pos_b[0] = pos_a[0] + 24.0f*dx;
            pos_b[1] = pos_a[1] + 24.0f*dy;
            pos_c[0] = pos_a[0] + 38.0f*dx;
            pos_c[1] = pos_a[1] + 38.0f*dy;
            
            dd_set_primitive_size(dd_ctx, 4.0f);
            dd_line2d(dd_ctx, pos_a, pos_b);
            
            dd_set_primitive_size(dd_ctx, 8.0f);
            dd_point2d(dd_ctx, pos_b);
            dd_set_primitive_size(dd_ctx, 1.0f);
            dd_point2d(dd_ctx, pos_c);
        }
    }
    
    
    dd_end_cmd( dd_ctx );
    
    dd_render( dd_ctx );
    
}


void cleanup( app_state_t* state )
{
    dd_term( state->dd_ctx );
    glfwTerminate();
    free( state );
}
