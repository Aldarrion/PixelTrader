cmake_minimum_required(VERSION 3.14)

set(PROJ_NAME PixelTrader)
project(${PROJ_NAME})

add_subdirectory(Engine)

set(CMAKE_CXX_STANDARD 17)

# Game
file(GLOB_RECURSE GAME_HEADERS "Game/include/*.h")
file(GLOB_RECURSE GAME_SOURCES "Game/src/*.cpp")
file(GLOB_RECURSE GAME_NATVIS "Game/*.natvis")
file(GLOB_RECURSE EDITORCONFIG "Engine/.editorconfig")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Game/include" PREFIX "Game" FILES ${GAME_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Game/src" PREFIX "Game" FILES ${GAME_SOURCES})

add_executable(${PROJ_NAME} WIN32 ${GAME_SOURCES} ${GAME_HEADERS} ${GAME_NATVIS} ${EDITORCONFIG})

target_include_directories(${PROJ_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Game/include")

target_link_libraries(${PROJ_NAME} HiddenEngine)

set_property(TARGET ${PROJ_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Data")

#include(Engine/CMakeCommon.txt)
#SetupCompiler(${PROJ_NAME})

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

    target_compile_options(${PROJ_NAME} PUBLIC -Werror)

    target_compile_options(${PROJ_NAME} PUBLIC -fsanitize=undefined)

    target_compile_options(${PROJ_NAME} PUBLIC -Wno-unused-private-field)
    target_compile_options(${PROJ_NAME} PUBLIC -Wno-nullability-completeness)
    target_compile_options(${PROJ_NAME} PUBLIC -Wno-range-loop-construct)

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")

    add_definitions(/MP)

    # Warnings as errors
    target_compile_options(${PROJ_NAME} PUBLIC /WX)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267") # conversion from 'size_t' to 'uint', possible loss of data
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244") # conversion from 'LONG' to 'float', possible loss of data
endif()

# Shared setup for all MSVC-like compilers
if(MSVC)
    # Disable C++ exceptions
    string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

    # Disable RTTI
    string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")

    add_definitions(/D _CRT_SECURE_NO_WARNINGS)

    # shaderc is compiled with /MD so we cannot compile with /MDd even on debug or we would need to rebuild the shaderc_combined.lib
    set_property(TARGET ${PROJ_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set_property(TARGET ImGui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    # Also for shaderc
    add_definitions(-D_HAS_ITERATOR_DEBUGGING=0)
    add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
endif()

# Solution name
project(PixelTrader)
