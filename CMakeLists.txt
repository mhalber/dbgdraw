cmake_minimum_required( VERSION 2.8 )

project(dbgdraw)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package( OpenGL REQUIRED )
find_package( GLFW3 REQUIRED )

set( EXAMPLES_DIR "${CMAKE_SOURCE_DIR}/examples")

include_directories( "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/external" "${EXAMPLES_DIR}/opengl45" ${GLFW3_INCLUDE_DIR} )

set( COMMON_SRCS "${CMAKE_SOURCE_DIR}/external/glad.c" "${CMAKE_SOURCE_DIR}/dbgdraw.c" )

set( TARGETS "basic" "bezier" "colors" "frustum_culling" "lines" "primitives" "text" )

foreach( TARGET ${TARGETS} )
  message( STATUS "dbgdraw_${TARGET}" )
  add_executable( "dbgdraw_${TARGET}" "${EXAMPLES_DIR}/opengl45/${TARGET}.c" ${COMMON_SRCS} )
  target_link_libraries( "dbgdraw_${TARGET}" ${GLFW3_LIBRARY} ${OPENGL_gl_LIBRARY} )
  if (MSVC)
    target_compile_options( "dbgdraw_${TARGET}" PRIVATE -wd4996 )
  endif()
endforeach( TARGET ) 