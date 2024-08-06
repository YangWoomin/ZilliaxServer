

############################################
### common options
set(CMAKE_CXX_STANDARD 20)
add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)


############################################
### compiler options
if ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" )

    ############################################
    ### compile options
    add_compile_options(/Wall)
    add_compile_options(/source-charset:utf-8)
    # options for debug
    add_compile_options($<$<CONFIG:Debug>:/Od>)
    add_compile_options($<$<CONFIG:Debug>:/Zi>)
    add_compile_options($<$<CONFIG:Debug>:/MDd>)
    # options for release
    add_compile_options($<$<CONFIG:Release>:/Od>)
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_compile_options($<$<CONFIG:Release>:/MD>)
    # disable some warnings
    add_compile_options(/wd4820)
    add_compile_options(/wd5039)
    add_compile_options(/wd4996) # 'WSASocketA': Use WSASocketW() instead or define _WINSOCK_DEPRECATED_NO_WARNINGS to disable deprecated API warnings
    add_compile_options(/wd4710) # 함수를 인라인하지 못했습니다.


    ############################################
    ### definitions
    add_compile_definitions(_MSVC_)
    add_compile_definitions(_WIN64 _WIN64_)
    add_compile_definitions(_X64)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)

    ############################################
    ### link options
    add_link_options(/nologo)
    add_link_options(/MACHINE:X64)
    add_link_options(/MANIFEST:NO)
    add_link_options(/INCREMENTAL:NO)
    add_link_options(/DEBUG)

    ############################################
    ### etc
    set(TARGET_EXECUTABLE_SUFFIX ".exe")

elseif ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" ) # CMAKE_CXX_COMPILER_ID

    ############################################
    ### compile options
    add_compile_options(-Wall)
    # options for debug
    add_compile_options($<$<CONFIG:Debug>:-g>)
    add_compile_options($<$<CONFIG:Debug>:-O0>)
    # options for release
    add_compile_options($<$<CONFIG:Release>:-g>)
    add_compile_options($<$<CONFIG:Release>:-O0>)

    ############################################
    ### definitions
    add_compile_definitions(_X64 __x86_64__)

    ############################################
    ### link options
    # options for debug
    add_link_options($<$<CONFIG:Debug>:-g>)
    # options for release
    add_link_options($<$<CONFIG:Release>:-g>)

    ############################################
    ### etc
    set(TARGET_EXECUTABLE_SUFFIX ".out")

else ()

    message( FATAL_ERROR "We need only MSVC or GCC/G++ compilers" )

endif () # CMAKE_CXX_COMPILER_ID


############################################
### os options
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )

    ############################################
    ### definitions
    add_compile_definitions(_LINUX_ _POSIX_)

    ############################################
    ### link options
    add_link_options(-D_LINUX_ -D_POSIX_ -D_X64 -D__x86_64__)

    ############################################
    ### libraries
    #link_libraries(pthread)

endif () # CMAKE_SYSTEM_NAME

