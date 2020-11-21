cmake_minimum_required(VERSION 3.14)

# To enable MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)

set(CMAKE_CONFIGURATION_TYPES "Release;Debug;RelWithDebInfo")

set(CMAKE_CXX_STANDARD 17)

set(PROJ_NAME HiddenEngine)
project(${PROJ_NAME})

set(VK_SDK_DIR "" CACHE PATH "Root directory of the LunarG SDK")

###############################################################################
# Extern
## ImGui
file(GLOB_RECURSE IMGUI_HEADERS "Extern/imgui/include/*.h")
file(GLOB_RECURSE IMGUI_SOURCES "Extern/imgui/src/*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Extern/imgui/include/imgui" PREFIX "ImGui" FILES ${IMGUI_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Extern/imgui/src" PREFIX "ImGui" FILES ${IMGUI_SOURCES})

add_library(ImGui ${IMGUI_HEADERS} ${IMGUI_SOURCES})

target_include_directories(ImGui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Extern/imgui/include/imgui")
target_include_directories(ImGui PRIVATE "${VK_SDK_DIR}/Include")

target_include_directories(ImGui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Extern/imgui/include")

## CJson
file(GLOB_RECURSE CJSON_HEADERS "Extern/cjson/include/*.h")
file(GLOB_RECURSE CJSON_SOURCES "Extern/cjson/src/*.c")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Extern/cjson/include/cjson" PREFIX "CJson" FILES ${CJSON_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Extern/cjson/src" PREFIX "CJson" FILES ${CJSON_SOURCES})

add_library(CJson ${CJSON_HEADERS} ${CJSON_SOURCES})

target_include_directories(CJson PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Extern/cjson/include/cjson")

target_include_directories(CJson PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Extern/cjson/include")

###############################################################################
# Engine
file(GLOB_RECURSE ENGINE_HEADERS "Engine/include/*.h")
file(GLOB_RECURSE ENGINE_SOURCES "Engine/src/*.cpp")
file(GLOB_RECURSE ENGINE_NATVIS "Engine/*.natvis")
file(GLOB_RECURSE EDITORCONFIG ".editorconfig")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Engine/include" PREFIX "Engine" FILES ${ENGINE_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Engine/src" PREFIX "Engine" FILES ${ENGINE_SOURCES})

add_library(${PROJ_NAME} ${ENGINE_SOURCES} ${ENGINE_HEADERS} ${ENGINE_NATVIS} ${EDITORCONFIG})

target_include_directories(${PROJ_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Extern/VulkanMemoryAllocator/include")
target_include_directories(${PROJ_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/shaderStructs")

target_include_directories(${PROJ_NAME} PUBLIC "${VK_SDK_DIR}/Include")
target_include_directories(${PROJ_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Engine/include")
target_include_directories(${PROJ_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Extern/stb/include")
target_include_directories(${PROJ_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Extern/cjson/include")

target_link_libraries(${PROJ_NAME} ImGui)
target_link_libraries(${PROJ_NAME} CJson)
target_link_libraries(${PROJ_NAME} "${VK_SDK_DIR}/Lib/vulkan-1.lib")
target_link_libraries(${PROJ_NAME} "${VK_SDK_DIR}/Lib/shaderc_shared.lib")

##
include(CMakeCommon.cmake)
SetupCompiler(${PROJ_NAME})

