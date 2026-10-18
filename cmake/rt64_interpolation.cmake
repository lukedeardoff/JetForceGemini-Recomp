# Experimental interpolation adapters; pinned dependency sources stay pristine.
include("${CMAKE_CURRENT_LIST_DIR}/rt64_video_settings.cmake")
function(jfg_interpolation_replace_source basename generated)
    get_target_property(sources rt64 SOURCES)
    set(patched)
    set(found FALSE)
    foreach(unit IN LISTS sources)
        get_filename_component(name "${unit}" NAME)
        if(name STREQUAL "${basename}")
            list(APPEND patched "${generated}")
            set(found TRUE)
        else()
            list(APPEND patched "${unit}")
        endif()
    endforeach()
    if(NOT found)
        message(FATAL_ERROR "Missing RT64 interpolation source: ${basename}")
    endif()
    set_property(TARGET rt64 PROPERTY SOURCES "${patched}")
endfunction()

set(source "${jfg_rt64_root}/src/hle/rt64_workload_queue.cpp")
file(SHA256 "${source}" digest)
if(NOT digest STREQUAL "6e53030e8899c42347b51e67879fcb9679ad487ac067c192612c432d85bca4a4" AND NOT digest STREQUAL "090cab491982923b41c75a55664586a61492343b89fe8963895c85bd6040d48f")
 message(FATAL_ERROR "Pinned RT64 interpolation input differs: src/hle/rt64_workload_queue.cpp")
endif()
file(READ "${jfg_workload_generated}" contents)
set(before [=[                // Detect the color image to interpolate for this workload.]=])
set(after [=[                const auto jfgSnapshot = jfg::presentation_snapshot(&workload);
                const bool jfgOffscreen = jfgSnapshot && !jfgSnapshot->scene;
                std::vector<uint32_t> jfgOffscreenColors;
                std::unordered_set<uint32_t> jfgOffscreenColorSet;
                InterpolatedFrameCounters jfgOffscreenCounters;
                // Detect the color image to interpolate for this workload.]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 interpolation patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
set(before [=[std::vector<uint32_t> &colorVector = ext.sharedResources->colorImageAddressVector;]=])
set(after [=[std::vector<uint32_t> &colorVector = jfgOffscreen ? jfgOffscreenColors : ext.sharedResources->colorImageAddressVector;]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 interpolation patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
set(before [=[std::unordered_set<uint32_t> &colorSet = ext.sharedResources->colorImageAddressSet;]=])
set(after [=[std::unordered_set<uint32_t> &colorSet = jfgOffscreen ? jfgOffscreenColorSet : ext.sharedResources->colorImageAddressSet;]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 interpolation patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
set(before [=[bool interpolationCandidate = fbPair.earlyPresentCandidate();]=])
set(after [=[bool interpolationCandidate = fbPair.earlyPresentCandidate() || (jfgSnapshot && jfgSnapshot->scene && fbPair.colorImage.width >= 320 && fbPair.colorImage.siz == G_IM_SIZ_16b);]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 interpolation patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
set(before [=[if (resetTicks) {]=])
set(after [=[if (resetTicks && !jfgOffscreen) {]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 interpolation patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
set(before [=[ext.sharedResources->viOriginalRate = workload.viOriginalRate;]=])
set(after [=[if (!jfgOffscreen) ext.sharedResources->viOriginalRate = workload.viOriginalRate;]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 interpolation patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
set(before [=[const bool useDifferentCounters = (lastPresentId != workload.presentId);]=])
set(after [=[const bool useDifferentCounters = !jfgOffscreen && (lastPresentId != workload.presentId);]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 interpolation patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
set(before [=[                InterpolatedFrameCounters &curFrameCounters = ext.sharedResources->interpolatedFrames[ext.sharedResources->interpolatedFramesIndex];
                curFrameCounters.skipped = false;]=])
set(after [=[                InterpolatedFrameCounters &curFrameCounters = jfgOffscreen ? jfgOffscreenCounters : ext.sharedResources->interpolatedFrames[ext.sharedResources->interpolatedFramesIndex];
                curFrameCounters.skipped = false;]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 interpolation patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
set(before [=[                    framesRendered++;]=])
set(after [=[                    jfg::trace_presentation_workload(workload, workloadConfig.targetRate,
                        frame, displayFrames, curFrameWeight);
                    framesRendered++;]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 interpolation patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
jfg_video_patch_workload()
set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated/rt64/rt64_workload_queue.cpp")
file(WRITE "${generated}" [=[#include "rt64_interpolation_adapter.hpp"
]=] "${contents}")
jfg_interpolation_replace_source("rt64_workload_queue.cpp" "${generated}")

set(source "${jfg_rt64_root}/src/hle/rt64_present_queue.cpp")
file(SHA256 "${source}" digest)
if(NOT digest STREQUAL "023dc3f4bc74bf6863e37ad1781fb32eaa543a7713bd8b9a3757b2262a1a1e52" AND NOT digest STREQUAL "e024b012b30304b0232581790af6416412264fc40a2b2e00494ba7ed0c842566")
 message(FATAL_ERROR "Pinned RT64 interpolation input differs: src/hle/rt64_present_queue.cpp")
endif()
file(READ "${source}" contents)
set(before [=[swapChainValid = ext.swapChain->present(swapChainIndex, &waitSemaphore, 1);]=])
set(after [=[swapChainValid = ext.swapChain->present(swapChainIndex, &waitSemaphore, 1);
                if (swapChainValid) jfg::frame_diagnostic_state().presented();
                jfg::trace_interpolation("present", present.presentId, viOriginalRate,
                    targetRate, static_cast<unsigned>(i), static_cast<unsigned>(framesToPresent),
                    0.0f, 0U, 0U);]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 interpolation patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
set(before [=[                    commandList->barriers(RenderBarrierStage::NONE, RenderTextureBarrier(swapChainTexture, RenderTextureLayout::PRESENT));]=])
set(after [=[                    auto jfgCapture = jfg::begin_interpolation_capture(ext.device, commandList,
                        swapChainTexture, ext.swapChain->getWidth(), ext.swapChain->getHeight(),
                        present.presentId, static_cast<unsigned>(i), jfgOutputSample.tick);
                    commandList->barriers(RenderBarrierStage::NONE, RenderTextureBarrier(swapChainTexture, RenderTextureLayout::PRESENT));]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 frame capture patch site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
set(before [=[                    ext.presentGraphicsWorker->commandQueue->executeCommandLists(&commandList, 1, &waitSemaphore, 1, &signalSemaphore, 1, ext.presentGraphicsWorker->commandFence.get());
                    ext.presentGraphicsWorker->wait();]=])
set(after [=[                    ext.presentGraphicsWorker->commandQueue->executeCommandLists(&commandList, 1, &waitSemaphore, 1, &signalSemaphore, 1, ext.presentGraphicsWorker->commandFence.get());
                    ext.presentGraphicsWorker->wait();
                    jfg::finish_interpolation_capture(jfgCapture);]=])
string(FIND "${contents}" "${before}" offset)
if(offset EQUAL -1)
 message(FATAL_ERROR "Missing RT64 frame capture completion site")
endif()
string(REPLACE "${before}" "${after}" contents "${contents}")
jfg_video_patch_present()
set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated/rt64/rt64_present_queue.cpp")
file(WRITE "${generated}" [=[#include "jfg/renderer/frame_diagnostics_state.hpp"
#include "rt64_interpolation_capture.hpp"
]=] "${contents}")
jfg_interpolation_replace_source("rt64_present_queue.cpp" "${generated}")

file(READ "${jfg_rt64_transform_processor_generated}" contents)
string(REPLACE "            DrawData &drawData = workload.drawData;"
    "            DrawData &drawData = workload.drawData;\n            if (jfg::apply_presentation_interpolation(workload, p.prevFrameWeight, p.curFrameWeight)) continue;"
    contents "${contents}")
string(REPLACE "prevFrameValid ? drawData.lerpWorldTransforms.data()"
    "(prevFrameValid || jfg::presentation_snapshot(&workload)) ? drawData.lerpWorldTransforms.data()"
    contents "${contents}")
string(REPLACE "prevFrameValid ? drawData.prevWorldTransforms.data()"
    "(prevFrameValid || jfg::presentation_snapshot(&workload)) ? drawData.prevWorldTransforms.data()"
    contents "${contents}")
file(WRITE "${jfg_rt64_transform_processor_generated}"
    "#include \"rt64_interpolation_adapter.hpp\"\n${contents}")
target_include_directories(rt64 PRIVATE "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src/renderer" "${jfg_rt64_root}/src/hle")
