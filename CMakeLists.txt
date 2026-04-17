cmake_minimum_required(VERSION 3.11)

# Default to in-repo vcpkg toolchain; recover if cache points to a stale path.
set(_repo_vcpkg_toolchain "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${_repo_vcpkg_toolchain}" CACHE STRING "Vcpkg toolchain file")
elseif(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}" AND EXISTS "${_repo_vcpkg_toolchain}")
    set(CMAKE_TOOLCHAIN_FILE "${_repo_vcpkg_toolchain}" CACHE STRING "Vcpkg toolchain file" FORCE)
endif()

project(codeforce C) 

find_package(CURL REQUIRED)

add_executable(codeforce 
    src/main.c
    cJSON/cJSON.c
    src/utils/utils.c
    )
target_include_directories(codeforce PRIVATE
    "cJSON"
    "src/include"
)

target_link_libraries(codeforce PRIVATE CURL::libcurl)

if(WIN32)
    add_custom_command(TARGET codeforce POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:codeforce>
            $<TARGET_FILE_DIR:codeforce>
        COMMAND ${CMAKE_COMMAND}
            -DSRC_DIR=${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/bin
            -DDST_DIR=$<TARGET_FILE_DIR:codeforce>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_runtime_dlls.cmake
        COMMAND ${CMAKE_COMMAND}
            -DSRC_DIR=${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin
            -DDST_DIR=$<TARGET_FILE_DIR:codeforce>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_runtime_dlls.cmake
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endif()