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

include(Engine/CMakeCommon.cmake)
SetupCompiler(${PROJ_NAME})

# Solution name
project(PixelTrader)
