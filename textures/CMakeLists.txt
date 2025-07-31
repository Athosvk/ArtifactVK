set(TEXTURES_SRC_DIR "${CMAKE_SOURCE_DIR}/textures")
set(TEXTURES_DST_DIR "${CMAKE_BINARY_DIR}")

add_custom_command(
    OUTPUT "${TEXTURES_DST_DIR}/.assets_linked"  # dummy file to mark "done"

    COMMAND ${CMAKE_COMMAND} -D SRC="${TEXTURES_SRC_DIR}" -D DST="${TEXTURES_DST_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/link_assets.cmake"
	COMMAND ${CMAKE_COMMAND} -E touch "${TEXTURES_DST_DIR}/.assets_linked"  # create marker after link/copy
    COMMENT "Linking or copying textures directory"
)

add_custom_target(SetupTextures ALL
    DEPENDS "${TEXTURES_DST_DIR}/.assets_linked"
)