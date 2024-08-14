

set( postfix_debug  "d" )

set( dir_proj_root                      ${CMAKE_SOURCE_DIR} )
set( dir_proj_include                   ${dir_proj_root}/include )
set( dir_output_bin                     ${dir_proj_root}/../output/bin )
set( dir_output_lib                     ${dir_proj_root}/../output/lib )

# common
set( dir_common_root                    ${CMAKE_SOURCE_DIR}/../common )
set( dir_common_include                 ${dir_common_root}/include)

# db
set( dir_db_root                        ${CMAKE_SOURCE_DIR}/../db)
set( dir_db_include                     ${dir_db_root}/include)
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
set( bin_db_d                           ${dir_output_lib}/dbd.lib )
set( bin_db                             ${dir_output_lib}/db.lib )
elseif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
set( bin_db_d                           ${dir_output_lib}/libdbd.so )
set( bin_db                             ${dir_output_lib}/libdb.so )
endif ()

# network
set( dir_network_root                   ${CMAKE_SOURCE_DIR}/../network)
set( dir_network_include                ${dir_network_root}/include)
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
set( bin_network_d                      ${dir_output_lib}/networkd.lib )
set( bin_network                        ${dir_output_lib}/network.lib )
elseif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
set( bin_network_d                      ${dir_output_lib}/libnetworkd.so )
set( bin_network                        ${dir_output_lib}/libnetwork.so )
endif ()

# third party
set( dir_third_party_root               ${CMAKE_SOURCE_DIR}/../third_party )
set( dir_third_party_include            ${dir_third_party_root}/include )
