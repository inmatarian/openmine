cmake_minimum_required(VERSION 2.4)

Project( OpenMine )

Find_Package( SDL REQUIRED )
Find_Package( OpenGL REQUIRED )

include_directories(
  ${SDL_INCLUDE_DIR}
)

set( SOURCES
  main.cpp
)

link_libraries(
  ${SDL_LIBRARY}
  ${OPENGL_LIBRARY}
  SDLmain
)

add_executable(
  openmine
  ${SOURCES}
)

