### Work in progress

## dbgdraw

dbgdraw is intended to be a small immediate mode library for putting simple graphics on the screen. Below you can see some examples of what can be achieved in dbgdraw.

![Overview](images/overview.png)

### Features

- Written in C99 with minimal dependencies (c stdlib, stb_truetype.h [optional] )
- Validation - optionally enable asserts that will inform users on common mistakes

### Current issues

- No full UTF-8 support, just latin + greek character ranges
- Only backend is OpenGL 4.5

### Usage

dbgdraw is built around a familiar idea of using contexts. To start drawing, one needs to initialize context using `dd_init`. Then on each frame some information needs to be provided to *dbgdraw* regarding the application state, like the camera position and viewport size. This can be achieved using the `dd_new_frame` call. Similarly, at the end of the frame, user requests the content to be rendered using `dd_render`. Once application is finished, or context is no longer needed, it can be removed using `dd_term`.

Within each frame, user can begin issuing drawing commands. These need to be delimited by the `dd_begin_cmd` and `dd_end_cmd`. Within, user can request any number of primitives to be drawn, using calls like `dd_line`, `dd_sphere`, `dd_aabb`, and so on. There are number of state modifying functions of the form `dd_set_x` - for example `dd_set_color` will modify color of primitives drawn in subsequent calls.

A simple example of code to draw the examples first example in the above image is :

~~~
// Put information about the scene camera and viewport
dd_new_frame_info_t info = { 
    .view_matrix       = view.data,
    .projection_matrix = proj.data,
    .viewport_size     = viewport.data,
    .vertical_fov      = fovy,
    .projection_type   = DBGDRAW_PERSPECTIVE };
dd_new_frame( dd_ctx, &info );

// Prepare some data
vec3 x0 = vec3_negx(); vec3 x1 = vec3_posx();
vec3 y0 = vec3_negy(); vec3 y1 = vec3_posy();
vec3 z0 = vec3_negz(); vec3 z1 = vec3_posz();

// Set primitive size - this will be interpreted as point size if drawing points and line width if drawing lines
dd_set_primitive_size( dd_ctx, 1.0f );

// Set transformation to be used when drawing the subsequent commands
dd_set_transform( dd_ctx, model.data );

// Let's begin drawing lines
dd_begin_cmd( dd_ctx, DBGDRAW_MODE_STROKE );

// Draw axis
dd_set_color( dd_ctx, DBGDRAW_RED );
dd_line( dd_ctx, x0.data, x1.data );
dd_set_color( dd_ctx, DBGDRAW_GREEN );
dd_line( dd_ctx, y0.data, y1.data );
dd_set_color( dd_ctx, DBGDRAW_BLUE );
dd_line( dd_ctx, z0.data, z1.data );

// Draw axis aligned bounding box
dd_set_color( dd_ctx, DBGDRAW_GRAY );
dd_aabb( dd_ctx, vec3( -1.1f, -1.1f, -1.1f ).data, vec3( 1.1f, 1.1f, 1.1f ).data );
dd_end_cmd( dd_ctx );
    
dd_render( dd_ctx );
~~~

Note that all `dd_<xyz>` calls simply take pointer to float data, so they should be agnostic to any vector library that you might use! (With the exception of dbgdraw expecting column major matrices )

#### Memory

dbgdraw keeps two main memory buffers - the command buffer and the vertex buffer. Their initial size is specified in a call to `dd_init`. If during a frame user submits more commands / vertices than the initial size, the memory will be resized similarly to how it is performed in C++'s '`std::vector`, where the capacity of these buffers will be doubled. 

By default memory allocations are done with `malloc`, `free` and `realloc`. These defaults can be changed by redefining following symbols `DBGDRAW_MALLOC(size)`, `DBGDRAW_FREE(ptr)`, `DBGDRAW_REALLOC(ptr, size)`. This way one can use their own specific allocators.

Additionally there is a macro that controls the out-of-memory behaviour. By redefining `DBGDRAW_HANDLE_OUT_OF_MEMORY(ptr, len, cap, elemsize)` one can
specify different behavour to the growing described above. For example if you want the application to simply stop accepting commands if there is no more memory in the initial allocation, you could do the following:
~~~
#define DBGDRAW_HANDLE_OUT_OF_MEMORY(ptr, len, cap, elemsize)     \
  do                                                              \
  {                                                               \
    if (len >= cap)                                               \
    {                                                             \
      return DBGDRAW_ERR_OUT_OF_MEMORY;                           \
    }                                                             \
  } while (0)
~~~
The above will cause all function that fill the buffer to exit early with out of memory error that the client side might
choose to deal with in whichever way they choose.

#### Building

To use dbgdraw you can simply drop the `dbgdraw.h` into your application source tree and add `#define DBGDRAW_IMPLEMENTATION` and `#include "dbgdraw.h"`. Optionally, there is also `dbgdraw.c` that you can add to your build, if you wish to avoid the recompilation of the dbgdraw library each time.

As far examples go, everyone has their favorite build system, and everyone has a build system that they hate vehemently. As such, for now I opted to simply add build-system-free scripts that simply call the complier directly. For more details, see the examples directory. I currently do not have macos machine at hand to test the building of these at hand - I welcome pull requests to add these!
