# Included by project(rt64), before its shader functions exist.
# Compiles the GPU actor-shadow softening shader (src/renderer/shaders) into
# the pinned RT64 target with RT64's own DXC rules, so the checkout stays
# pristine. cmake/rt64_shadow_soften.cmake wires it into the reinterpret pass.
# Shadow work: Luke Deardoff (AI-assisted), issue #4.
set(jfg_shadow_soften_shader_source
    "${CMAKE_CURRENT_LIST_DIR}/../src/renderer/shaders/FbShadowSoftenCS.hlsl")
function(jfg_rt64_shadow_soften_shader)
    file(REAL_PATH "${jfg_shadow_soften_shader_source}" source)
    if(NOT EXISTS "${source}" OR NOT TARGET rt64 OR
       NOT COMMAND build_shader_spirv_impl OR NOT DEFINED DXC_CS_OPTS)
        message(FATAL_ERROR "Pinned RT64 shader rules for shadow softening are missing")
    endif()
    # Same output location and blob names as RT64's own shaders, so the
    # generated rt64_shader_library.cpp includes it like FbReinterpretCS.
    set(output "${CMAKE_BINARY_DIR}/src/shaders/FbShadowSoftenCS.hlsl")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/src/shaders")
    # Its includes resolve against RT64's shader directory.
    set(options ${DXC_CS_OPTS} "-I${PROJECT_SOURCE_DIR}/src/shaders")
    if(WIN32)
        build_shader_dxil_impl(rt64 "${source}" FbShadowSoftenCS "${output}" ${options})
    endif()
    build_shader_spirv_impl(rt64 "${source}" FbShadowSoftenCS "${output}" ${options})
endfunction()
cmake_language(DEFER CALL jfg_rt64_shadow_soften_shader)
