# GPU actor-shadow softening (issue #4, Luke Deardoff, AI-assisted).
#
# The game renders each actor shadow into a 64x64 I8 mask and post-processes
# it on the CPU (blur + opacity) before sampling it. With JFG_SHADOW_MODE=gpu
# (the default) the mask stays on the GPU at the internal resolution and these
# generated RT64 units soften it in the reinterpret pass instead, then apply
# the game's opacity. JFG_SHADOW_MODE=cpu keeps the CPU-mask writeback path.
#
# Same approach as rt64_depth_bounds.cmake: hash-check the pinned sources,
# apply exact single-site replacements, and compile generated copies.

function(jfg_shadow_soften_generate source_relative expected_hash generated_out)
    set(source "${jfg_rt64_root}/${source_relative}")
    file(READ "${source}" contents)
    string(REPLACE "\r\n" "\n" contents "${contents}")
    string(SHA256 hash "${contents}")
    if(NOT hash STREQUAL expected_hash)
        message(FATAL_ERROR
            "Pinned RT64 ${source_relative} differs from the audited shadow-softening input")
    endif()
    list(LENGTH ARGN pair_values)
    math(EXPR last "${pair_values} - 1")
    foreach(index RANGE 0 ${last} 2)
        math(EXPR next "${index} + 1")
        list(GET ARGN ${index} old_name)
        list(GET ARGN ${next} new_name)
        set(old "${${old_name}}")
        set(new "${${new_name}}")
        string(FIND "${contents}" "${old}" first)
        string(FIND "${contents}" "${old}" final REVERSE)
        if(first EQUAL -1 OR NOT first EQUAL final)
            message(FATAL_ERROR
                "Shadow-softening patch site ${old_name} in ${source_relative} is missing or ambiguous")
        endif()
        string(REPLACE "${old}" "${new}" contents "${contents}")
    endforeach()
    string(REGEX REPLACE "^src/" "" generated_relative "${source_relative}")
    set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated/rt64/${generated_relative}")
    file(WRITE "${generated}" "${contents}")
    set(${generated_out} "${generated}" PARENT_SCOPE)
endfunction()

function(jfg_shadow_soften_replace_source file_name generated)
    get_target_property(sources rt64 SOURCES)
    set(patched)
    set(replaced FALSE)
    foreach(unit IN LISTS sources)
        if(unit MATCHES "(^|[/\\\\])${file_name}$")
            list(APPEND patched "${generated}")
            set(replaced TRUE)
        else()
            list(APPEND patched "${unit}")
        endif()
    endforeach()
    if(NOT replaced)
        message(FATAL_ERROR "Pinned RT64 target is missing ${file_name}")
    endif()
    set_property(TARGET rt64 PROPERTY SOURCES "${patched}")
endfunction()

# --- rt64_framebuffer_manager.cpp: parameters and pipeline selection --------
set(jfg_fbm_globals_old [==[
namespace RT64 {
    static void fixSizeToMultiple(uint32_t &width, uint32_t &height) {]==])
set(jfg_fbm_globals_new [==[
namespace RT64 {
    // JFG: soften radius in native texels for 64x64 8b->IA8 shadow tiles
    // (0 disables), and the shadow opacity the game's CPU pass would apply:
    // raw IA 0x8E becomes at most 0x08 (black, alpha 8/15), alpha * 8/14.
    // Both are set by the JFG renderer shell.
    float ShadowBlurTexels = 0.0f;
    float ShadowOpacity = 8.0f / 14.0f;
    extern RenderPipeline *JfgShadowSoftenPipeline; // rt64_shader_library.cpp

    static void fixSizeToMultiple(uint32_t &width, uint32_t &height) {]==])
set(jfg_fbm_params_old [==[
        c.usesHDR = usesHDR;
]==])
set(jfg_fbm_params_new [==[
        c.usesHDR = usesHDR ? 1u : 0u;
        {
            // JFG: parameters packed into usesHDR bits 1+ so FbReinterpretCB
            // keeps its upstream layout (see FbShadowSoftenCS.hlsl).
            const float scaleX = float(resolutionScale.x);
            const float nativeWidth = (scaleX > 0.0f) ? (float(dstTile.usedWidth) / scaleX) : float(dstTile.usedWidth);
            const bool shadowLike = (c.srcSiz == G_IM_SIZ_8b) && (c.dstFmt == G_IM_FMT_IA) && (c.dstSiz == G_IM_SIZ_8b) && (nativeWidth <= 64.5f);
            if (shadowLike && (ShadowBlurTexels > 0.0f) && (scaleX > 0.0f) && (JfgShadowSoftenPipeline != nullptr)) {
                const uint32_t softnessQ = uint32_t(std::clamp<long>(std::lround(ShadowBlurTexels * 64.0f), 1L, 0xFFFL));
                const uint32_t scaleQ = uint32_t(std::clamp<long>(std::lround(scaleX * 64.0f), 1L, 0xFFFL));
                const uint32_t opacityQ = uint32_t(std::clamp<long>(std::lround(ShadowOpacity * 127.0f), 0L, 127L));
                c.usesHDR |= (softnessQ << 8) | (scaleQ << 20) | (opacityQ << 1);
            }
        }
]==])
set(jfg_fbm_dispatch_old [==[
            renderWorker->commandList->setPipeline(shaderRecord.pipeline.get());
            renderWorker->commandList->setComputePipelineLayout(shaderRecord.pipelineLayout.get());
            for (const CommandListReinterpretDispatch &dispatch : cmdListReinterpretations.cmdListDispatches) {
]==])
set(jfg_fbm_dispatch_new [==[
            RenderPipeline *activeReinterpretPipeline = shaderRecord.pipeline.get();
            renderWorker->commandList->setPipeline(activeReinterpretPipeline);
            renderWorker->commandList->setComputePipelineLayout(shaderRecord.pipelineLayout.get());
            for (const CommandListReinterpretDispatch &dispatch : cmdListReinterpretations.cmdListDispatches) {
                // JFG: softened shadow tiles carry their parameters in usesHDR bits 8+.
                RenderPipeline *wantedPipeline = shaderRecord.pipeline.get();
                if (((dispatch.reinterpretCB.usesHDR >> 8) != 0) && (JfgShadowSoftenPipeline != nullptr)) {
                    wantedPipeline = JfgShadowSoftenPipeline;
                }
                if (wantedPipeline != activeReinterpretPipeline) {
                    renderWorker->commandList->setPipeline(wantedPipeline);
                    activeReinterpretPipeline = wantedPipeline;
                }
]==])
jfg_shadow_soften_generate(src/hle/rt64_framebuffer_manager.cpp
    "1a97e98b34dc4707d4a9514ef6992bd751e5a0d6fe2c5bcefd50234b41686fd5"
    jfg_shadow_soften_fbm
    jfg_fbm_globals_old jfg_fbm_globals_new
    jfg_fbm_params_old jfg_fbm_params_new
    jfg_fbm_dispatch_old jfg_fbm_dispatch_new)

# --- rt64_shader_library.cpp: optional soften pipeline ----------------------
set(jfg_lib_plume_old [==[
#include "common/rt64_common.h"
]==])
set(jfg_lib_plume_new [==[
#include "common/rt64_common.h"
#include "plume_vulkan.h"
#ifdef _WIN32
#   include "plume_d3d12.h"
#endif
#include <cstdio>
]==])
set(jfg_lib_spirv_old [==[
#include "shaders/FbReinterpretCS.hlsl.spirv.h"
]==])
set(jfg_lib_spirv_new [==[
#include "shaders/FbReinterpretCS.hlsl.spirv.h"
#include "shaders/FbShadowSoftenCS.hlsl.spirv.h"
]==])
set(jfg_lib_dxil_old [==[
#   include "shaders/FbReinterpretCS.hlsl.dxil.h"
]==])
set(jfg_lib_dxil_new [==[
#   include "shaders/FbReinterpretCS.hlsl.dxil.h"
#   include "shaders/FbShadowSoftenCS.hlsl.dxil.h"
]==])
set(jfg_lib_global_old [==[
namespace RT64 {
    // ShaderLibrary
]==])
set(jfg_lib_global_new [==[
namespace RT64 {
    // JFG: optional actor-shadow softening pipeline (same layout as
    // fbReinterpret). Stays null if it fails to build; shadows then use the
    // standard reinterpret pass.
    RenderPipeline *JfgShadowSoftenPipeline = nullptr;

    static bool jfgComputePipelineValid(RenderPipeline *pipeline) {
        if (pipeline == nullptr) {
            return false;
        }
#   ifdef _WIN32
        if (auto *d3d12 = dynamic_cast<plume::D3D12ComputePipeline *>(pipeline)) {
            return d3d12->d3d != nullptr;
        }
#   endif
        if (auto *vulkan = dynamic_cast<plume::VulkanComputePipeline *>(pipeline)) {
            return vulkan->vk != VK_NULL_HANDLE;
        }
        return true;
    }

    // ShaderLibrary
]==])
set(jfg_lib_pipeline_old [==[
            fbReinterpret.pipeline = device->createComputePipeline(pipelineDesc);
]==])
set(jfg_lib_pipeline_new [==[
            fbReinterpret.pipeline = device->createComputePipeline(pipelineDesc);

            // JFG: shadow softening variant. Never replaces the upstream
            // pipeline; if the driver rejects it, softening stays disabled.
            if (JfgShadowSoftenPipeline == nullptr) {
                std::unique_ptr<RenderShader> softenShader = device->createShader(CREATE_SHADER_INPUTS(FbShadowSoftenCSBlobDXIL, FbShadowSoftenCSBlobSPIRV, FbReinterpretCSBlobMSL, "CSMain", shaderFormat));
                RenderComputePipelineDesc softenDesc(fbReinterpret.pipelineLayout.get(), softenShader.get(), 8, 8, 1);
                std::unique_ptr<RenderPipeline> softenPipeline = device->createComputePipeline(softenDesc);
                if (jfgComputePipelineValid(softenPipeline.get())) {
                    JfgShadowSoftenPipeline = softenPipeline.release();
                }
                else {
                    fprintf(stderr, "[JFG] Shadow softening pipeline failed to build; using standard shadows.\n");
                }
            }
]==])
jfg_shadow_soften_generate(src/render/rt64_shader_library.cpp
    "72421375b232d5449f2f90f153c8452420084f43809c3234bc9727eddfde6ffd"
    jfg_shadow_soften_lib
    jfg_lib_plume_old jfg_lib_plume_new
    jfg_lib_spirv_old jfg_lib_spirv_new
    jfg_lib_dxil_old jfg_lib_dxil_new
    jfg_lib_global_old jfg_lib_global_new
    jfg_lib_pipeline_old jfg_lib_pipeline_new)

jfg_shadow_soften_replace_source("rt64_framebuffer_manager\\.cpp" "${jfg_shadow_soften_fbm}")
jfg_shadow_soften_replace_source("rt64_shader_library\\.cpp" "${jfg_shadow_soften_lib}")
# Preserve quoted and relative header lookup for the generated translation units.
target_include_directories(rt64 PRIVATE
    "${jfg_rt64_root}/src/hle" "${jfg_rt64_root}/src/render")
