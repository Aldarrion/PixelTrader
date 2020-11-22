macro(SetupCompiler projectName)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")

        add_definitions(/MP)
    endif()

    # Shared setup for all MSVC-like compilers
    if(MSVC)
        # Disable C++ exceptions
        string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
        #add_definitions(/EHa- /EHs- /EHc- /EHr-)

        # Disable RTTI
        #string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
        add_definitions(/GR-)

        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif()
endmacro()
