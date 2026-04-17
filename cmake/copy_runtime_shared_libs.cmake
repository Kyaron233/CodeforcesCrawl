if(NOT DEFINED SRC_DIR OR NOT DEFINED DST_DIR)
    message(FATAL_ERROR "SRC_DIR and DST_DIR must be provided")
endif()

if(EXISTS "${SRC_DIR}")
    file(GLOB _runtime_shared_libs
        "${SRC_DIR}/*.so"
        "${SRC_DIR}/*.so.*"
    )
    if(_runtime_shared_libs)
        file(COPY ${_runtime_shared_libs} DESTINATION "${DST_DIR}")
    endif()
endif()
