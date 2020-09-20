cmake_minimum_required(VERSION 3.14)

# To enable MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)

set(CMAKE_CONFIGURATION_TYPES "Release;Debug;RelWithDebInfo")

set(CMAKE_CXX_STANDARD 17)

set(PROJ_NAME PixelTrader)
project(${PROJ_NAME})

file(GLOB_RECURSE GAME_HEADERS "game/include/*.h")
file(GLOB_RECURSE GAME_SOURCES "game/src/*.cpp")
file(GLOB_RECURSE GAME_NATVIS "game/*.natvis")
file(GLOB_RECURSE EDITORCONFIG ".editorconfig")
file(GLOB_RECURSE EXTERN_CJSON_SRC "extern/cjson/src/*.c")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/game/include" PREFIX "Game" FILES ${GAME_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/game/src" PREFIX "Game" FILES ${GAME_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/extern/cjson/src" PREFIX "Extern/cJSON" FILES ${EXTERN_CJSON_SRC})

add_executable(${PROJ_NAME} WIN32 ${GAME_SOURCES} ${GAME_HEADERS} ${GAME_NATVIS} ${EXTERN_CJSON_SRC} ${EDITORCONFIG})

set(VK_SDK_DIR "" CACHE PATH "Root directory of the LunarG SDK")

target_include_directories(${PROJ_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/game/include")
target_include_directories(${PROJ_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/extern/VulkanMemoryAllocator/include")
target_include_directories(${PROJ_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/extern/stb/include")
target_include_directories(${PROJ_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/extern/cjson/include")
target_include_directories(${PROJ_NAME} PRIVATE "${VK_SDK_DIR}/Include")

target_link_libraries(${PROJ_NAME} "${VK_SDK_DIR}/Lib/vulkan-1.lib")
target_link_libraries(${PROJ_NAME} "${VK_SDK_DIR}/Lib/shaderc_combined.lib")

set_property(TARGET ${PROJ_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data")

if(MSVC)
    # Disable C++ exceptions
    string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

    # Disable RTTI
    string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")

    # shaderc is compiled with /MD so we cannot compile with /MDd even on debug or we would need to rebuild the shaderc_combined.lib
    set_property(TARGET ${PROJ_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    # Also for shaderc
    add_definitions(-D_HAS_ITERATOR_DEBUGGING=0)
    add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)

    add_definitions(/MP)
    add_definitions(/D _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${PROJ_NAME} PRIVATE /WX)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267") # conversion from 'size_t' to 'uint', possible loss of data
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244") # conversion from 'LONG' to 'float', possible loss of data
endif()

# Solution name
project(PixelTrader)
