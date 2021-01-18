cmake_minimum_required( VERSION 2.8 )

project(dbgdraw)

set( EXAMPLES_DIR ${CMAKE_SOURCE_DIR}/examples)
set( TARGETS "basic" "bezier" "colors" "frustum_culling" "instancing" "lines" "primitives" "text" "vector_field" )
set( COMMON_SRCS ${CMAKE_SOURCE_DIR}/dbgdraw.c )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/examples/shared )

if (MSVC)
  set( CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug> )
  set( CMAKE_C_FLAGS "/FC /GR- /EHa- /nologo /W4 /wd4115 /wd4201 /wd4204 /wd4996 /wd4221" )
endif()

if (${DBGDRAW_BACKEND} STREQUAL "OGL33")

  message("-- Selected OGL33 backend!")

  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

  find_package( OpenGL REQUIRED )
  find_package( GLFW3 REQUIRED )

  set( SRC_DIR ${EXAMPLES_DIR}/opengl )
  set( PREFIX "dd_ogl33_")
  set( LIBS ${GLFW3_LIBRARY} ${OPENGL_gl_LIBRARY} )
  list( APPEND COMMON_SRCS ${CMAKE_SOURCE_DIR}/external/glad33.c )

  include_directories( ${SRC_DIR} ${GLFW3_INCLUDE_DIR} )
  add_definitions(-DDD_USE_OGL_33)

elseif (${DBGDRAW_BACKEND} STREQUAL "OGL45")

  message("-- Selected OGL45 backend!")

  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

  find_package( OpenGL REQUIRED )
  find_package( GLFW3 REQUIRED )

  set( SRC_DIR ${EXAMPLES_DIR}/opengl )
  set( PREFIX "dd_ogl45_")
  set( LIBS ${GLFW3_LIBRARY} ${OPENGL_gl_LIBRARY} )
  list( APPEND COMMON_SRCS ${CMAKE_SOURCE_DIR}/external/glad45.c )

  include_directories( ${SRC_DIR} ${GLFW3_INCLUDE_DIR} )
  add_definitions(-DDD_USE_OGL_45)

elseif (${DBGDRAW_BACKEND} STREQUAL "D3D11")

  message("-- Selected D3D11 backend!")

  set( SRC_DIR ${EXAMPLES_DIR}/d3d11 )
  set( PREFIX "dd_d3d11_")
  list( APPEND COMMON_SRCS ${SRC_DIR}/d3d11_app.c )
  include_directories( ${SRC_DIR} )

else()

message("-- Please provide backend! Possible values: OGL33, OGL45, D3D11")
return()

endif()

foreach( TARGET ${TARGETS} )
  message( STATUS ${PREFIX}${TARGET} )
  add_executable(  ${PREFIX}${TARGET} ${SRC_DIR}/${TARGET}.c ${COMMON_SRCS} )
  target_link_libraries( ${PREFIX}${TARGET} ${LIBS} )
endforeach( TARGET ) 