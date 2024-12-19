cmake_minimum_required(VERSION 3.10)
project(rimeac.lua)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(lua_dir "lua-v5.4")
set(lua_ver "54")
set(liblua "lua_static${lua_ver}")

file(GLOB LUA_SOURCES "${lua_dir}/*.c")
list(REMOVE_ITEM LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${lua_dir}/lua.c")
list(REMOVE_ITEM LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${lua_dir}/onelua.c")

add_library(${liblua} STATIC ${LUA_SOURCES})
if(NOT WIN32)
  target_compile_options(${liblua} PRIVATE "-fPIC")
endif()

if(MINGW)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

if(WIN32)
  # build lua54.dll
  add_library(lua54 SHARED ${LUA_SOURCES})
  set_target_properties(lua54 PROPERTIES
      OUTPUT_NAME "lua${lua_ver}"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
  target_compile_definitions(lua54 PRIVATE LUA_BUILD_AS_DLL)
  # build lua.exe
  add_executable(lua "${CMAKE_CURRENT_SOURCE_DIR}/${lua_dir}/lua.c")
  set_target_properties(lua PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
  target_link_libraries(lua lua54)
  #  lua.exe runs without Visual C++ Redistributable installed
  if(MSVC)
      set_target_properties(lua PROPERTIES
          MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
      target_link_options(lua PRIVATE "/LARGEADDRESSAWARE")
      target_link_options(lua54 PRIVATE "/LARGEADDRESSAWARE")
  endif()
endif()

# link rime path and set dist path
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

# set MT and pdb
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
include_directories(./include ${lua_dir})

# target rimeac.lua.exe or rimeac.lua
add_executable(${PROJECT_NAME} ${RIMEAC_LUA_SOURCES})
target_link_libraries(${PROJECT_NAME} ${liblua} rime)
if(MSVC)
  target_link_options(${PROJECT_NAME} PRIVATE "/LARGEADDRESSAWARE")
endif()

# target librimeac.dll or librimeac.so
if(MSVC)
  set(LIBRIMEAC "librimeac")
else()
  set(LIBRIMEAC "rimeac")
endif()

add_library(${LIBRIMEAC} SHARED ${RIMEAC_LUA_SOURCES})
target_link_libraries(${LIBRIMEAC} ${liblua} rime)
target_compile_definitions(${LIBRIMEAC} PRIVATE MODULE)
if(MSVC)
  target_link_options(${LIBRIMEAC} PRIVATE "/LARGEADDRESSAWARE")
endif()

# installations
if(MSVC)
  install(TARGETS ${PROJECT_NAME} 
    DESTINATION ${dist})
  install(TARGETS lua RUNTIME 
    DESTINATION ${dist})
  install(TARGETS lua54 RUNTIME 
    DESTINATION ${dist})
  install(FILES "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.pdb" 
    DESTINATION ${dist} OPTIONAL)
  install(FILES ${rimedll} README.md script.lua script_ext.lua rime_api_console.lua LICENSE.txt
    DESTINATION ${dist})
  install(DIRECTORY shared usr 
    DESTINATION ${dist}
    PATTERN ".placeholder" EXCLUDE)
  install(TARGETS ${LIBRIMEAC} RUNTIME DESTINATION ${dist})
elseif(MINGW)
  install(TARGETS ${PROJECT_NAME} 
    DESTINATION ${dist})
  install(TARGETS lua RUNTIME 
    DESTINATION ${dist})
  install(TARGETS lua54 RUNTIME 
    DESTINATION ${dist})
  install(FILES ${rimedll} README.md script.lua script_ext.lua rime_api_console.lua LICENSE.txt
    DESTINATION ${dist})
  install(DIRECTORY shared usr 
    DESTINATION ${dist}
    PATTERN ".placeholder" EXCLUDE)
  install(TARGETS ${LIBRIMEAC} RUNTIME DESTINATION ${dist})
endif()
