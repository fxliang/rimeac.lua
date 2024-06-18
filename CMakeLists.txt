cmake_minimum_required(VERSION 3.10)
project(rimeac.lua)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(lua_dir "lua-v5.4")
set(lua_ver "54")
set(liblua "lua${lua_ver}")

file(GLOB LUA_SOURCES "${lua_dir}/*.c")
list(REMOVE_ITEM LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${lua_dir}/lua.c")
list(REMOVE_ITEM LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${lua_dir}/onelua.c")
#list(REMOVE_ITEM LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${lua_dir}/luac.c")

add_library(${liblua} STATIC ${LUA_SOURCES})

#add_executable(lua ${lua_dir}/lua.c)
#target_link_libraries(lua ${liblua})
#add_executable(luac ${lua_dir}/luac.c)
#target_link_libraries(luac ${liblua})

if(WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib64)
    set(dist ${CMAKE_CURRENT_SOURCE_DIR}/dist64)
    set(rimedll ${CMAKE_CURRENT_SOURCE_DIR}/lib64/rime.dll)
  else()
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
    set(dist ${CMAKE_CURRENT_SOURCE_DIR}/dist)
    set(rimedll ${CMAKE_CURRENT_SOURCE_DIR}/lib/rime.dll)
  endif()
endif(WIN32)

file(GLOB RIMEAC_LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
include_directories(./include ${lua_dir})

add_executable(${PROJECT_NAME} ${RIMEAC_LUA_SOURCES})
target_link_libraries(${PROJECT_NAME} ${liblua} rime)

if(WIN32)
  install(TARGETS ${PROJECT_NAME} DESTINATION ${dist})
  install(FILES ${rimedll} README.md script.lua LICENSE.txt DESTINATION ${dist})
  install(DIRECTORY shared usr DESTINATION ${dist})
endif()
