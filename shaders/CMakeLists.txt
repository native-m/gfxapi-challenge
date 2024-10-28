find_program(FXC fxc)

macro(add_shader_d3d11 _TARGET _FILE)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_FILE}.dxbc"
        COMMAND FXC
        ARGS /T ${_TARGET} /Fo "${CMAKE_CURRENT_BINARY_DIR}/${_FILE}.dxbc" "${CMAKE_CURRENT_SOURCE_DIR}/${_FILE}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_FILE}"
        VERBATIM)
    set(SHADER_DEPENDENCIES "${SHADER_DEPENDENCIES};${CMAKE_CURRENT_BINARY_DIR}/${_FILE}.dxbc")
endmacro()

add_shader_d3d11(vs_5_0 shader_vs.hlsl)
add_shader_d3d11(ps_5_0 shader_ps.hlsl)
add_custom_target(d3d11_shaders DEPENDS ${SHADER_DEPENDENCIES})
