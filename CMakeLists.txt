project("uring_demo_platform")


set(LIBS uring rt)


aux_source_directory(. SRC)
foreach (FILE_PATH ${SRC})
    string(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
    message(${FILE_NAME})
    add_executable(${FILE_NAME} ${FILE_PATH})
    target_link_libraries(${FILE_NAME} ${LIBS} )
endforeach()
