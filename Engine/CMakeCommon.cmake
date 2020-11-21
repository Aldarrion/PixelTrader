macro(SetupCompiler projectName)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

        target_compile_options(${projectName} PUBLIC -Werror)

        target_compile_options(${projectName} PUBLIC -fsanitize=undefined)

        target_compile_options(${projectName} PUBLIC -Wno-unused-private-field)
        target_compile_options(${projectName} PUBLIC -Wno-nullability-completeness)
        target_compile_options(${projectName} PUBLIC -Wno-range-loop-construct)

    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")

        add_definitions(/MP)

        # Warnings as errors
        target_compile_options(${projectName} PUBLIC /WX)

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
        set_property(TARGET ${projectName} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
        set_property(TARGET ImGui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
        # Also for shaderc
        add_definitions(-D_HAS_ITERATOR_DEBUGGING=0)
        add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)
    endif()
endmacro()
