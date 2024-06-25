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

if(MSVC)
  foreach(flag_var
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
      CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
  endforeach(flag_var)

  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

file(GLOB RIMEAC_LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
include_directories(./include ${lua_dir} ./LuaBridge3/Distribution)

add_executable(${PROJECT_NAME} ${RIMEAC_LUA_SOURCES})
target_link_libraries(${PROJECT_NAME} ${liblua} rime)

if(MSVC)
  install(TARGETS ${PROJECT_NAME} DESTINATION ${dist})
  install(FILES "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.pdb" DESTINATION ${dist} OPTIONAL)
  install(FILES ${rimedll} README.md script.lua LICENSE.txt DESTINATION ${dist})
  install(DIRECTORY shared usr DESTINATION ${dist})
endif()
