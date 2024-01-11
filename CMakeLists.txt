cmake_minimum_required(VERSION 3.20)
project(aviutl_extram2 C)
add_subdirectory(src/c)
add_subdirectory(src/lua)

add_custom_target(copy_exa ALL
  COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/src/exa/CacheText2.exa" "${CMAKE_BINARY_DIR}/bin/キャッシュテキスト2.exa"
)
