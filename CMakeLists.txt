cmake_minimum_required( VERSION 2.8 )

project(dbgdraw)

find_package( OpenGL REQUIRED )
find_package( GLFW3 REQUIRED )

set( EXAMPLES_DIR "${CMAKE_SOURCE_DIR}/examples")

include_directories( "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/external" "${EXAMPLES_DIR}/opengl45" )

set( COMMON_SRCS "${CMAKE_SOURCE_DIR}/external/glad.c" "${CMAKE_SOURCE_DIR}/debugdraw.c" )

set( TARGETS "basic" "bezier" "colors" "frustum_culling" "lines" "primitives" "text" )

foreach( TARGET ${TARGETS} )
  message( STATUS "dd_${TARGET}" )
  add_executable( "dd_${TARGET}" "${EXAMPLES_DIR}/opengl45/${TARGET}.c" ${COMMON_SRCS} )
  target_link_libraries( "dd_${TARGET}" glfw3 ${OPENGL_gl_LIBRARY} )
  target_compile_options( "dd_${TARGET}" PRIVATE -std=c11 -Wall -Wextra )
endforeach( TARGET )  