if(NOT DEFINED SRC OR NOT DEFINED DST)
    message(FATAL_ERROR "SRC and DST must be defined")
endif()

message(STATUS "Linking or copying entire directory from ${SRC} to ${DST}")

# Remove existing destination first
file(REMOVE_RECURSE "${DST}")

# Try to symlink whole directory
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E create_symlink "${SRC}" "${DST}"
    RESULT_VARIABLE result
    OUTPUT_QUIET ERROR_QUIET
)

if(result EQUAL 0)
    message(STATUS "Symlinked ${DST} → ${SRC}")
else()
    message(WARNING "Symlink failed, copying whole directory instead")
    file(COPY "${SRC}" DESTINATION "${DST}")
    message(STATUS "Copied directory ${SRC} to ${DST}")
endif()
