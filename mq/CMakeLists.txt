

cmake_minimum_required( VERSION 3.8 )


project( "mq" )


message( STATUS "SYSTEM_VERSION=$(CMAKE_SYSTEM_VERSION)" )
message( STATUS "SYSTEM_PROCESSOR=$(CMAKE_SYSTEM_PROCESSOR)" )
message( STATUS "SYSTEM_NAME=$(CMAKE_NAME)" )
message( STATUS "HOST_SYSTEM_NAME=$(CMAKE_HOST_SYSTEM_NAME)" )
message( STATUS "CXX_COMPILER_ID=$(CMAKE_CXX_COMPILER_ID)" )
message( STATUS "CXX_COMPILER_VER=$(CMAKE_CXX_COMPILER_VER)" )
message( STATUS "CXX_COMPILER=$(CMAKE_CXX_COMPILER)" )


include ( ../build/project_common_settings.cmake )
include ( ../build/compiler_options.cmake )


add_subdirectory( "src" )

