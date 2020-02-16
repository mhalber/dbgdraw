cmake_minimum_required( VERSION 2.8 )

project(dbgdraw)

find_package( OpenGL REQUIRED )
find_package( GLFW3 REQUIRED )

set( EXAMPLES_DIR "${CMAKE_SOURCE_DIR}/examples")

include_directories( "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/external" "${EXAMPLES_DIR}/opengl45" )

set( COMMON_SRCS "${CMAKE_SOURCE_DIR}/external/glad.c" "${CMAKE_SOURCE_DIR}/dbgdraw.c" )

set( TARGETS "basic" "bezier" "colors" "frustum_culling" "lines" "primitives" "text" )

foreach( TARGET ${TARGETS} )
  message( STATUS "dbgdraw_${TARGET}" )
  add_executable( "dbgdraw_${TARGET}" "${EXAMPLES_DIR}/opengl45/${TARGET}.c" ${COMMON_SRCS} )
  target_link_libraries( "dbgdraw_${TARGET}" glfw3 ${OPENGL_gl_LIBRARY} )
  target_compile_options( "dbgdraw_${TARGET}" PRIVATE -std=c11 -Wall -Wextra )
endforeach( TARGET )  