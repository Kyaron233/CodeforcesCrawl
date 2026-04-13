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
    )
target_include_directories(codeforce PRIVATE
    "cJSON"
)

target_link_libraries(codeforce PRIVATE CURL::libcurl)