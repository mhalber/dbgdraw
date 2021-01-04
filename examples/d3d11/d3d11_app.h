/* This file is modelled after the d3d11entry.h from sokol_gfx, by Andre Weissflog (@floooh)*/

#ifndef D3D11_APP
#define D3D11_APP

#include <stdint.h>
#include <stdbool.h>


//TODO(maciej): Ensure no warnings are produced: https://devblogs.microsoft.com/directx/dxgi-flip-model/

typedef struct d3d11_app_mouse 
{
  float cur_x;
  float cur_y;
  float prev_x;
  float prev_y;
  float scroll_x;
  float scroll_y;
  uint8_t buttons[8];
} d3d11_app_mouse_t;

typedef struct d3d11_ctx
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
} d3d11_ctx_t;

typedef struct d3d11_app_input
{
  uint8_t keys[512];
  d3d11_app_mouse_t mouse;
  int32_t mods;

} d3d11_app_input_t;

typedef struct d3d11_ctx_desc
{
  const char* win_title;
  int32_t win_x, win_y, win_w, win_h;
  int32_t sample_count;
} d3d11_ctx_desc_t;

int32_t d3d11_init(d3d11_ctx_t* d3d11, const d3d11_ctx_desc_t* desc);
void d3d11_terminate(d3d11_ctx_t* d3d11);

bool d3d11_process_events(d3d11_app_input_t** input);
void d3d11_clear(d3d11_ctx_t* d3d11, float r, float g, float b, float a);
void d3d11_viewport(d3d11_ctx_t* d3d11, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void d3d11_present(d3d11_ctx_t* d3d11);

/* (from sokol_app.h by Andre Weissflog) */

enum {
  D3D11_APP_MOUSE_BUTTON_INVALID = -1,
  D3D11_APP_MOUSE_BUTTON_LEFT = 0,
  D3D11_APP_MOUSE_BUTTON_RIGHT = 1,
  D3D11_APP_MOUSE_BUTTON_MIDDLE = 2,
};

/* key codes are the same names and values as GLFW */
enum
{
  D3D11_APP_KEYCODE_INVALID          = 0,
  D3D11_APP_KEYCODE_SPACE            = 32,
  D3D11_APP_KEYCODE_APOSTROPHE       = 39,  /* ' */
  D3D11_APP_KEYCODE_COMMA            = 44,  /* , */
  D3D11_APP_KEYCODE_MINUS            = 45,  /* - */
  D3D11_APP_KEYCODE_PERIOD           = 46,  /* . */
  D3D11_APP_KEYCODE_SLASH            = 47,  /* / */
  D3D11_APP_KEYCODE_0                = 48,
  D3D11_APP_KEYCODE_1                = 49,
  D3D11_APP_KEYCODE_2                = 50,
  D3D11_APP_KEYCODE_3                = 51,
  D3D11_APP_KEYCODE_4                = 52,
  D3D11_APP_KEYCODE_5                = 53,
  D3D11_APP_KEYCODE_6                = 54,
  D3D11_APP_KEYCODE_7                = 55,
  D3D11_APP_KEYCODE_8                = 56,
  D3D11_APP_KEYCODE_9                = 57,
  D3D11_APP_KEYCODE_SEMICOLON        = 59,  /* ; */
  D3D11_APP_KEYCODE_EQUAL            = 61,  /* = */
  D3D11_APP_KEYCODE_A                = 65,
  D3D11_APP_KEYCODE_B                = 66,
  D3D11_APP_KEYCODE_C                = 67,
  D3D11_APP_KEYCODE_D                = 68,
  D3D11_APP_KEYCODE_E                = 69,
  D3D11_APP_KEYCODE_F                = 70,
  D3D11_APP_KEYCODE_G                = 71,
  D3D11_APP_KEYCODE_H                = 72,
  D3D11_APP_KEYCODE_I                = 73,
  D3D11_APP_KEYCODE_J                = 74,
  D3D11_APP_KEYCODE_K                = 75,
  D3D11_APP_KEYCODE_L                = 76,
  D3D11_APP_KEYCODE_M                = 77,
  D3D11_APP_KEYCODE_N                = 78,
  D3D11_APP_KEYCODE_O                = 79,
  D3D11_APP_KEYCODE_P                = 80,
  D3D11_APP_KEYCODE_Q                = 81,
  D3D11_APP_KEYCODE_R                = 82,
  D3D11_APP_KEYCODE_S                = 83,
  D3D11_APP_KEYCODE_T                = 84,
  D3D11_APP_KEYCODE_U                = 85,
  D3D11_APP_KEYCODE_V                = 86,
  D3D11_APP_KEYCODE_W                = 87,
  D3D11_APP_KEYCODE_X                = 88,
  D3D11_APP_KEYCODE_Y                = 89,
  D3D11_APP_KEYCODE_Z                = 90,
  D3D11_APP_KEYCODE_LEFT_BRACKET     = 91,  /* [ */
  D3D11_APP_KEYCODE_BACKSLASH        = 92,  /* \ */
  D3D11_APP_KEYCODE_RIGHT_BRACKET    = 93,  /* ] */
  D3D11_APP_KEYCODE_GRAVE_ACCENT     = 96,  /* ` */
  D3D11_APP_KEYCODE_WORLD_1          = 161, /* non-US #1 */
  D3D11_APP_KEYCODE_WORLD_2          = 162, /* non-US #2 */
  D3D11_APP_KEYCODE_ESCAPE           = 256,
  D3D11_APP_KEYCODE_ENTER            = 257,
  D3D11_APP_KEYCODE_TAB              = 258,
  D3D11_APP_KEYCODE_BACKSPACE        = 259,
  D3D11_APP_KEYCODE_INSERT           = 260,
  D3D11_APP_KEYCODE_DELETE           = 261,
  D3D11_APP_KEYCODE_RIGHT            = 262,
  D3D11_APP_KEYCODE_LEFT             = 263,
  D3D11_APP_KEYCODE_DOWN             = 264,
  D3D11_APP_KEYCODE_UP               = 265,
  D3D11_APP_KEYCODE_PAGE_UP          = 266,
  D3D11_APP_KEYCODE_PAGE_DOWN        = 267,
  D3D11_APP_KEYCODE_HOME             = 268,
  D3D11_APP_KEYCODE_END              = 269,
  D3D11_APP_KEYCODE_CAPS_LOCK        = 280,
  D3D11_APP_KEYCODE_SCROLL_LOCK      = 281,
  D3D11_APP_KEYCODE_NUM_LOCK         = 282,
  D3D11_APP_KEYCODE_PRINT_SCREEN     = 283,
  D3D11_APP_KEYCODE_PAUSE            = 284,
  D3D11_APP_KEYCODE_F1               = 290,
  D3D11_APP_KEYCODE_F2               = 291,
  D3D11_APP_KEYCODE_F3               = 292,
  D3D11_APP_KEYCODE_F4               = 293,
  D3D11_APP_KEYCODE_F5               = 294,
  D3D11_APP_KEYCODE_F6               = 295,
  D3D11_APP_KEYCODE_F7               = 296,
  D3D11_APP_KEYCODE_F8               = 297,
  D3D11_APP_KEYCODE_F9               = 298,
  D3D11_APP_KEYCODE_F10              = 299,
  D3D11_APP_KEYCODE_F11              = 300,
  D3D11_APP_KEYCODE_F12              = 301,
  D3D11_APP_KEYCODE_F13              = 302,
  D3D11_APP_KEYCODE_F14              = 303,
  D3D11_APP_KEYCODE_F15              = 304,
  D3D11_APP_KEYCODE_F16              = 305,
  D3D11_APP_KEYCODE_F17              = 306,
  D3D11_APP_KEYCODE_F18              = 307,
  D3D11_APP_KEYCODE_F19              = 308,
  D3D11_APP_KEYCODE_F20              = 309,
  D3D11_APP_KEYCODE_F21              = 310,
  D3D11_APP_KEYCODE_F22              = 311,
  D3D11_APP_KEYCODE_F23              = 312,
  D3D11_APP_KEYCODE_F24              = 313,
  D3D11_APP_KEYCODE_F25              = 314,
  D3D11_APP_KEYCODE_KP_0             = 320,
  D3D11_APP_KEYCODE_KP_1             = 321,
  D3D11_APP_KEYCODE_KP_2             = 322,
  D3D11_APP_KEYCODE_KP_3             = 323,
  D3D11_APP_KEYCODE_KP_4             = 324,
  D3D11_APP_KEYCODE_KP_5             = 325,
  D3D11_APP_KEYCODE_KP_6             = 326,
  D3D11_APP_KEYCODE_KP_7             = 327,
  D3D11_APP_KEYCODE_KP_8             = 328,
  D3D11_APP_KEYCODE_KP_9             = 329,
  D3D11_APP_KEYCODE_KP_DECIMAL       = 330,
  D3D11_APP_KEYCODE_KP_DIVIDE        = 331,
  D3D11_APP_KEYCODE_KP_MULTIPLY      = 332,
  D3D11_APP_KEYCODE_KP_SUBTRACT      = 333,
  D3D11_APP_KEYCODE_KP_ADD           = 334,
  D3D11_APP_KEYCODE_KP_ENTER         = 335,
  D3D11_APP_KEYCODE_KP_EQUAL         = 336,
  D3D11_APP_KEYCODE_LEFT_SHIFT       = 340,
  D3D11_APP_KEYCODE_LEFT_CONTROL     = 341,
  D3D11_APP_KEYCODE_LEFT_ALT         = 342,
  D3D11_APP_KEYCODE_LEFT_SUPER       = 343,
  D3D11_APP_KEYCODE_RIGHT_SHIFT      = 344,
  D3D11_APP_KEYCODE_RIGHT_CONTROL    = 345,
  D3D11_APP_KEYCODE_RIGHT_ALT        = 346,
  D3D11_APP_KEYCODE_RIGHT_SUPER      = 347,
  D3D11_APP_KEYCODE_MENU             = 348,
};

enum {
  D3D11_APP_MODIFIER_SHIFT = (1<<0),
  D3D11_APP_MODIFIER_CTRL = (1<<1),
  D3D11_APP_MODIFIER_ALT = (1<<2),
  D3D11_APP_MODIFIER_SUPER = (1<<3)
};

#endif /*D3D11_APP*/
