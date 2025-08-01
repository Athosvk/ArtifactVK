if(NOT DEFINED SRC OR NOT DEFINED DST)
    message(FATAL_ERROR "SRC and DST must be defined")
endif()

message(STATUS "Setting up assets directory from ${SRC} to ${DST}")

# Remove destination first to avoid leftover files
file(REMOVE_RECURSE "${DST}")

# Try to create a symlink to the whole directory
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E create_symlink "${SRC}" "${DST}"
    RESULT_VARIABLE SYMLINK_RESULT
    OUTPUT_QUIET ERROR_QUIET
)

if(SYMLINK_RESULT EQUAL 0)
    message(STATUS "Symlink created: ${DST} → ${SRC}")
else()
    message(WARNING "Symlink failed, copying directory instead")
    file(COPY "${SRC}" DESTINATION "${DST}")
    message(STATUS "Copied directory ${SRC} to ${DST}")
endif()