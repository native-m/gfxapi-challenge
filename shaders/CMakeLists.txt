find_program(FXC fxc)
find_program(DXC dxc)

macro(add_shader_d3d11 _TARGET _FILE)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_FILE}.dxbc"
        COMMAND FXC
        ARGS /T ${_TARGET} /Fo "${CMAKE_CURRENT_BINARY_DIR}/${_FILE}.dxbc" "${CMAKE_CURRENT_SOURCE_DIR}/${_FILE}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_FILE}"
        VERBATIM)
    set(D3D11_SHADER_DEPENDENCIES "${D3D11_SHADER_DEPENDENCIES};${CMAKE_CURRENT_BINARY_DIR}/${_FILE}.dxbc")
endmacro()

macro(add_shader_d3d12 _TARGET _FILE)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_FILE}.dxil"
        COMMAND DXC
        ARGS /T ${_TARGET} /Fo "${CMAKE_CURRENT_BINARY_DIR}/${_FILE}.dxil" "${CMAKE_CURRENT_SOURCE_DIR}/${_FILE}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_FILE}"
        VERBATIM)
    set(D3D12_SHADER_DEPENDENCIES "${D3D12_SHADER_DEPENDENCIES};${CMAKE_CURRENT_BINARY_DIR}/${_FILE}.dxil")
endmacro()

add_shader_d3d11(vs_5_0 shader_vs.hlsl)
add_shader_d3d11(ps_5_0 shader_ps.hlsl)
add_custom_target(d3d11_shaders DEPENDS ${D3D11_SHADER_DEPENDENCIES})

add_shader_d3d12(vs_6_0 shader_vs.hlsl)
add_shader_d3d12(ps_6_0 shader_ps.hlsl)
add_custom_target(d3d12_shaders DEPENDS ${D3D12_SHADER_DEPENDENCIES})