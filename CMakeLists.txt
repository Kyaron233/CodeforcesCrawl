cmake_minimum_required(VERSION 3.10) 

project(codeforce C) 

find_package(CURL REQUIRED)
# find_package()

add_executable(codeforce 
    src/main.c
    cJSON/cJSON.c
    )


target_include_directories(
    cJSON/
    ${CURL_INCLUDE_DIRS}
)