option(FORMAT_SOURCES "execute clang-format" ON)
enable_testing()

if(FORMAT_SOURCES)
  file(GLOB_RECURSE sources LIST_DIRECTORIES false CONFIGURE_DEPENDS "*.h" "*.c")
  list(FILTER sources EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/3rd")
  find_program(CLANG_FORMAT_EXE clang-format)
  add_custom_target(${PROJECT_NAME}-format ALL
    COMMAND ${CLANG_FORMAT_EXE} -style=file -i ${sources}
  )
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(LUA51_PLATFORM x86_64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(LUA51_PLATFORM i686)
else()
  message(FATAL_ERROR "failed to detect environment")
endif()

set(LUA51_URL "https://github.com/oov/lua-5.1.5/releases/download/v5.1.5/lua_v5.1.5_${LUA51_PLATFORM}.zip")
string(REGEX MATCH "[^/]+$" LUA51_ARCHIVE_NAME "${LUA51_URL}")
set(LUA51_ARCHIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${LUA51_ARCHIVE_NAME}")
set(LUA51_DIR "${CMAKE_CURRENT_BINARY_DIR}/lua51")
set(LUA51_DLL "${LUA51_DIR}/bin/lua51.dll")
set(LUA51_INCLUDE "${LUA51_DIR}/include")
if(NOT EXISTS "${LUA51_DIR}")
  if(NOT EXISTS "${LUA51_ARCHIVE_PATH}")
    file(DOWNLOAD "${LUA51_URL}" "${LUA51_ARCHIVE_PATH}")
  endif()
  string(REGEX REPLACE "\\.[^.]+$" "" LUA51_ARCHIVE_NOEXT "${LUA51_ARCHIVE_NAME}")
  file(MAKE_DIRECTORY "${LUA51_DIR}")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf ${LUA51_ARCHIVE_PATH}
    WORKING_DIRECTORY "${LUA51_DIR}"
  )
endif()

add_library(extram_intf INTERFACE)
target_include_directories(extram_intf INTERFACE
  3rd/hashmap.c
  ${LUA51_INCLUDE}
)
target_compile_definitions(extram_intf INTERFACE
  _WIN32_WINNT=0x0601
  $<$<CONFIG:Release>:NDEBUG=1>
)
target_compile_options(extram_intf INTERFACE
  -Wall
  -Wextra
  -pedantic-errors
  $<$<CONFIG:Debug>:-O0>
  $<$<CONFIG:Release>:-O3>
)
target_link_options(extram_intf INTERFACE
  $<$<CONFIG:Release>:-s>
  -Wl,--kill-at
)

add_library(hashmap STATIC 3rd/hashmap.c/hashmap.c)
target_link_libraries(hashmap PRIVATE extram_intf)

add_library(tinycthread STATIC
  3rd/tinycthread/source/tinycthread.c
)
target_include_directories(tinycthread INTERFACE
  3rd/tinycthread/source
)
target_link_libraries(tinycthread PRIVATE extram_intf ${LUA51_DLL})
target_compile_options(tinycthread PRIVATE
  -Wno-reserved-macro-identifier
  -Wno-reserved-identifier
  -Wno-padded
  -Wno-missing-variable-declarations
  -Wno-atomic-implicit-seq-cst
)

add_library(kvs STATIC kvs.c)
target_link_libraries(kvs PRIVATE extram_intf PUBLIC hashmap)

add_executable(test_kvs test_kvs.c)
target_link_libraries(test_kvs PRIVATE extram_intf kvs)
add_test(NAME test_kvs COMMAND test_kvs)

add_library(intram2 SHARED)
set_target_properties(intram2 PROPERTIES PREFIX "" OUTPUT_NAME "Intram2" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/script")
target_sources(intram2 PRIVATE
  intram2.c
  hash.c
)
target_link_libraries(intram2 PRIVATE extram_intf kvs ${LUA51_DLL})

add_library(extram2 SHARED)
set_target_properties(extram2 PROPERTIES PREFIX "" OUTPUT_NAME "Extram2" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/script")
target_sources(extram2 PRIVATE
  extram2.c
  hash.c
  process.c
)
target_link_libraries(extram2 PRIVATE extram_intf tinycthread)

add_executable(extram2_remote)
set_target_properties(extram2_remote PROPERTIES OUTPUT_NAME "Extram2" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/script")
target_sources(extram2_remote PRIVATE
  extram2_remote.c
)
target_link_libraries(extram2_remote PRIVATE extram_intf kvs)
