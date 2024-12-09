

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
set( lib_db_d                           ${dir_output_lib}/dbd.lib )
set( lib_db                             ${dir_output_lib}/db.lib )
elseif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
set( lib_db_d                           ${dir_output_lib}/libdbd.so )
set( lib_db                             ${dir_output_lib}/libdb.so )
endif ()

# network
set( dir_network_root                   ${CMAKE_SOURCE_DIR}/../network)
set( dir_network_include                ${dir_network_root}/include)
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
set( lib_network_d                      ${dir_output_lib}/networkd.lib )
set( lib_network                        ${dir_output_lib}/network.lib )
elseif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
set( lib_network_d                      ${dir_output_lib}/libnetworkd.so )
set( lib_network                        ${dir_output_lib}/libnetwork.so )
endif ()

# mq
set( dir_mq_root                        ${CMAKE_SOURCE_DIR}/../mq)
set( dir_mq_include                     ${dir_mq_root}/include)
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
set( lib_mq_d                           ${dir_output_lib}/mqd.lib )
set( lib_mq                             ${dir_output_lib}/mq.lib )
elseif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
set( lib_mq_d                           ${dir_output_lib}/libmqd.so )
set( lib_mq                             ${dir_output_lib}/libmq.so )
endif ()

# cache
set( dir_cache_root                     ${CMAKE_SOURCE_DIR}/../cache)
set( dir_cache_include                  ${dir_cache_root}/include)
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
set( lib_cache_d                        ${dir_output_lib}/cached.lib )
set( lib_cache                          ${dir_output_lib}/cache.lib )
elseif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
set( lib_cache_d                        ${dir_output_lib}/libcached.so )
set( lib_cache                          ${dir_output_lib}/libcache.so )
endif ()

# third party
set( dir_third_party_root               ${CMAKE_SOURCE_DIR}/../third_party )
set( dir_third_party_lib                ${CMAKE_SOURCE_DIR}/../third_party/lib )
set( dir_third_party_include            ${dir_third_party_root}/include )
# librdkafka
set( dir_third_party_librdkafka_include ${dir_third_party_include}/librdkafka )
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
set( lib_librdkafka_d                   ${dir_third_party_lib}/rdkafkad.lib )
set( lib_librdkafka                     ${dir_third_party_lib}/rdkafka.lib )
elseif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
set( lib_librdkafka_d                   ${dir_third_party_lib}/librdkafka.so.1 )
set( lib_librdkafka                     ${dir_third_party_lib}/librdkafka.so.1 )
endif ()
# hiredis
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
set( lib_hiredis_d                      ${dir_third_party_lib}/hiredis.lib )
set( lib_hiredis                        ${dir_third_party_lib}/hiredis.lib )
elseif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
set( lib_hiredis_d                      ${dir_third_party_lib}/libhiredis.so.1.1.0 )
set( lib_hiredis                        ${dir_third_party_lib}/libhiredis.so.1.1.0 )
endif ()
# libuv
set( dir_third_party_libuv_include      ${dir_third_party_include}/uv )
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
set( lib_uv_d                           ${dir_third_party_lib}/uv.lib )
set( lib_uv                             ${dir_third_party_lib}/uv.lib )
elseif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
set( lib_uv_d                           ${dir_third_party_lib}/libuv.so.1.0.0 )
set( lib_uv                             ${dir_third_party_lib}/libuv.so.1.0.0 )
endif ()
# redis++
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
set( lib_redispp_d                      ${dir_third_party_lib}/redis++d.lib )
set( lib_redispp                        ${dir_third_party_lib}/redis++.lib )
elseif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
set( lib_redispp_d                      ${dir_third_party_lib}/libredis++d.so )
set( lib_redispp                        ${dir_third_party_lib}/libredis++.so )
endif ()
