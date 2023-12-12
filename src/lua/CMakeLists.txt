find_package(Git REQUIRED)
execute_process(
  COMMAND ${GIT_EXECUTABLE} tag --points-at HEAD
  OUTPUT_VARIABLE GIT_TAG
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if ("${GIT_TAG}" STREQUAL "")
  set(GIT_TAG "vX.X.X")
endif()
message(STATUS "git tag: ${GIT_TAG}")

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_REVISION
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if ("${GIT_REVISION}" STREQUAL "")
  set(GIT_REVISION "unknown")
endif()
message(STATUS "git revision: ${GIT_REVISION}")

configure_file("@Cache2.anm" "${CMAKE_BINARY_DIR}/bin/script/@Cache2.anm" @ONLY NEWLINE_STYLE CRLF)
configure_file("Cache2.lua" "${CMAKE_BINARY_DIR}/bin/script/Cache2.lua" @ONLY NEWLINE_STYLE CRLF)
configure_file("../../README.md" "${CMAKE_BINARY_DIR}/bin/Extram2.txt" @ONLY NEWLINE_STYLE CRLF)
