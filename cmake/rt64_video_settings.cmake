# Applied only to already hash-checked RT64 build-tree copies.
function(jfg_video_replace needle replacement)
    string(FIND "${contents}" "${needle}" offset)
    if(offset EQUAL -1)
        message(FATAL_ERROR "Missing pinned RT64 video-settings patch site: ${needle}")
    endif()
    string(REPLACE "${needle}" "${replacement}" contents "${contents}")
    set(contents "${contents}" PARENT_SCOPE)
endfunction()

function(jfg_video_patch_workload)
    jfg_video_replace([=[                ext.presentQueue->waitForPresentId(workload.presentId);]=] [=[                const auto jfgWaitStart = std::chrono::steady_clock::now();
                ext.presentQueue->waitForPresentId(workload.presentId);
                if (jfg::interpolation_trace_enabled()) {
                    const auto snapshot = jfg::presentation_snapshot(&workload);
                    jfg::trace_interpolation("workload-wait", snapshot ? snapshot->tick : 0U,
                        workload.presentId, workload.workloadId, 0U, 0U, 0.0f,
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::steady_clock::now()-jfgWaitStart).count(),
                        snapshot && !snapshot->scene ? 1U : 0U);
                }]=])
    jfg_video_replace([=[                    // Add total time the frame took to render.]=] [=[                    if (jfgSnapshot && !jfgOffscreen) {
                        const unsigned outputIndex = !overrideTarget ? UINT32_MAX : overrideModifier - 1U;
                        jfg::record_presentation_output(&curFrameCounters, outputIndex,
                            {jfgSnapshot->tick, workload.viOriginalRate, frame, displayFrames, curFrameWeight});
                    }
                    // Add total time the frame took to render.]=])

    jfg_video_replace([=[                            skipWorkloadNow = ((frame + 1) < displayFrames) && (writeCursor != threadCursor);]=] [=[                            bool nextScene = (writeCursor != threadCursor);
                            if (nextScene) {
                                // Small shadow-only passes between VIs must not end
                                // a scene's interpolation halfway through at any rate.
                                // A queued scene or unknown workload still takes priority.
                                nextScene = false;
                                uint32_t next = threadCursor;
                                while (next != writeCursor) {
                                    const auto snapshot = jfg::presentation_snapshot(&workloads[next]);
                                    if (!snapshot || snapshot->scene) { nextScene = true; break; }
                                    next = (next + 1U) % uint32_t(workloads.size());
                                }
                            }
                            skipWorkloadNow = ((frame + 1) < displayFrames) && nextScene;]=])
    jfg_video_replace([=[            // Limit the target rate to the rate detected by the swap chain.
            if ((ext.sharedResources->swapChainRate > 0) && (workloadConfig.targetRate > ext.sharedResources->swapChainRate)) {
                workloadConfig.targetRate = ext.sharedResources->swapChainRate;
            }]=] [=[            workloadConfig.targetRate = jfg::presentation_target(workloadConfig.targetRate,
                ext.sharedResources->swapChainRate);]=])
    jfg_video_replace([=[                // Estimate amount of frames to render based on how many display frames it'd take to reach the next logical frame.]=] [=[                const bool jfgUnlimited = jfg::presentation_unlimited() && generateInterpolatedFrames && !jfgOffscreen;
                float jfgPreviousAlpha = 0.0f;
                // Estimate amount of frames to render based on how many display frames it'd take to reach the next logical frame.]=])
    jfg_video_replace([=[                if (generateInterpolatedFrames) {
                    logicalTicks += workloadConfig.targetRate;]=] [=[                if (jfgUnlimited) {
                    displayFrames = jfg::kUnlimitedPresentation;
                    frameReduction = false;
                }
                else if (generateInterpolatedFrames) {
                    logicalTicks += workloadConfig.targetRate;]=])
    jfg_video_replace([=[                uint32_t requiredFrames = (usingMSAA && generateInterpolatedFrames) ? displayFrames : (displayFrames - 1);]=] [=[                uint32_t requiredFrames = jfgUnlimited ? jfg::kPresentationRingSize :
                    ((usingMSAA && generateInterpolatedFrames) ? displayFrames : (displayFrames - 1));]=])
    jfg_video_replace([=[                bool skippedFrames = false;]=] [=[                // Unlimited has no predetermined frame count. Always mark the stream
                // complete on leaving its source-frame time window, waking the consumer.
                bool skippedFrames = jfgUnlimited;]=])
    jfg_video_replace([=[                for (uint32_t frame = 0; (frame < displayFrames) && !skipWorkloadNow; frame++) {
                    // Evaluate]=] [=[                for (uint32_t frame = 0; (frame < displayFrames) && !skipWorkloadNow; frame++) {
                    if (jfgUnlimited && frame > 0 &&
                        (!threadsRunning || workloadTimer.elapsedMicroseconds() >= originalTimeMicro)) break;
                    // Evaluate]=])
    jfg_video_replace([=[                    if ((frame > 0) && (originalTimeMicro > 0)) {]=] [=[                    if (!jfgUnlimited && (frame > 0) && (originalTimeMicro > 0)) {]=])
    jfg_video_replace([=[                        prevFrameWeight = std::clamp((workloadConfig.targetRate + displayTicks - logicalTicks) / float(workloadConfig.targetRate), 0.0f, 1.0f);
                        displayTicks += workload.viOriginalRate;
                        curFrameWeight = std::clamp((workloadConfig.targetRate + displayTicks - logicalTicks) / float(workloadConfig.targetRate), 0.0f, 1.0f);]=] [=[                        if (!jfgUnlimited) {
                            prevFrameWeight = std::clamp((workloadConfig.targetRate + displayTicks - logicalTicks) / float(workloadConfig.targetRate), 0.0f, 1.0f);
                            displayTicks += workload.viOriginalRate;
                            curFrameWeight = std::clamp((workloadConfig.targetRate + displayTicks - logicalTicks) / float(workloadConfig.targetRate), 0.0f, 1.0f);
                        }]=])
    jfg_video_replace([=[                            overrideTarget = interpolatedTargets[targetIndex].get();
                            overrideModifier = (targetIndex + 1);

                            if (useDifferentCounters && (prevFrameCounters.available > 0) && (targetIndex < prevFrameCounters.available)) {]=] [=[                            const uint32_t storageIndex = jfgUnlimited ? (targetIndex % jfg::kPresentationRingSize) : (targetIndex * 2U + jfgTargetBank);
                            overrideTarget = interpolatedTargets[storageIndex].get();
                            overrideModifier = (storageIndex + 1);

                            if (jfgUnlimited) {
                                std::unique_lock<std::mutex> lock(ext.sharedResources->interpolatedMutex);
                                // The unlimited producer owns only this scene's
                                // time window. A consumer can be waiting for the
                                // next workload, so a slot wait must expire even
                                // when no presentation notification can arrive.
                                const auto remainingMicro = std::max<int64_t>(0,
                                    originalTimeMicro - workloadTimer.elapsedMicroseconds());
                                ext.sharedResources->interpolatedCondition.wait_for(lock,
                                    std::chrono::microseconds(remainingMicro), [&]() {
                                    const bool previousDone = !useDifferentCounters || prevFrameCounters.presented >= prevFrameCounters.available;
                                    const bool slotFree = targetIndex < jfg::kPresentationRingSize ||
                                        curFrameCounters.presented > (targetIndex - jfg::kPresentationRingSize);
                                    return !threadsRunning || (previousDone && slotFree);
                                });
                                if (!threadsRunning || curFrameCounters.presented >= curFrameCounters.count ||
                                    workloadTimer.elapsedMicroseconds() >= originalTimeMicro) break;
                            }
                            else if (useDifferentCounters && (prevFrameCounters.available > 0) && (targetIndex < prevFrameCounters.available)) {]=])
    jfg_video_replace([=[                    const bool uploadExtras = (frame == 0) && workloadConfig.raytracingEnabled;]=] [=[                    if (jfgUnlimited) {
                        prevFrameWeight = jfgPreviousAlpha;
                        curFrameWeight = std::clamp(float(workloadTimer.elapsedMicroseconds()) / float(originalTimeMicro), 0.0f, 1.0f);
                        deltaTimeMs = (curFrameWeight - prevFrameWeight) / float(workload.viOriginalRate);
                        jfgPreviousAlpha = curFrameWeight;
                    }
                    const bool uploadExtras = (frame == 0) && workloadConfig.raytracingEnabled;]=])
    jfg_video_replace([=[                        if (skipWorkloadNow) {
                            displayTicks += workload.viOriginalRate * (displayFrames - (frame + 1));]=] [=[                        if (skipWorkloadNow && !jfgUnlimited) {
                            displayTicks += workload.viOriginalRate * (displayFrames - (frame + 1));]=])
    jfg_video_replace([=[                if ((requiredFrames > 0) && (interpolatedTargets.size() < requiredFrames)) {
                    uint32_t previousSize = uint32_t(interpolatedTargets.size());
                    interpolatedTargets.resize(requiredFrames);
                    for (uint32_t i = previousSize; i < requiredFrames; i++) {
                        interpolatedTargets[i] = std::make_unique<RenderTarget>(interpolationTargetKey.address, Framebuffer::Type::Color, RenderMultisampling(), usesHDR);
                    }
                }]=] [=[                // Finite sequences have separate alternating banks. A new
                // sequence must not wait for, or overwrite, the previous one.
                const uint32_t jfgTargetBank = ext.sharedResources->interpolatedFramesIndex;
                const uint32_t storageFrames = jfgUnlimited ? requiredFrames : requiredFrames * 2U;
                {
                    // The consumer reads this vector under the same mutex.
                    // Growing it during a cadence change must not race a lookup.
                    std::scoped_lock<std::mutex> targetsLock(ext.sharedResources->interpolatedMutex);
                    if (interpolatedTargets.size() < storageFrames)
                        interpolatedTargets.resize(storageFrames);
                    for (uint32_t i = 0; i < requiredFrames; ++i) {
                        const uint32_t slot = jfgUnlimited ? i : i * 2U + jfgTargetBank;
                        if (!interpolatedTargets[slot])
                            interpolatedTargets[slot] = std::make_unique<RenderTarget>(interpolationTargetKey.address, Framebuffer::Type::Color, RenderMultisampling(), usesHDR);
                    }
                }]=])
    jfg_video_replace([=[                            else if (useDifferentCounters && (prevFrameCounters.available > 0) && (targetIndex < prevFrameCounters.available)) {
                                // Wait until the target has finished presenting if the alternate frame counter (used by the present queue) is making use of this target.
                                std::unique_lock<std::mutex> interpolatedLock(ext.sharedResources->interpolatedMutex);
                                ext.sharedResources->interpolatedCondition.wait(interpolatedLock, [&]() {
                                    frameReduction = frameReduction || (prevFrameCounters.presented <= targetIndex);
                                    return prevFrameCounters.presented > targetIndex;
                                });
                            }]=] [=[                            // Capped frames use the other bank, so they do not
                            // need the old cross-sequence target-reuse wait.]=])
    set(contents "${contents}" PARENT_SCOPE)
endfunction()

function(jfg_video_patch_present)
    jfg_video_replace([=[                if (skipPresent) {
                    skipInterpolation();]=] [=[                if (skipPresent) {
                    jfg::trace_interpolation("cpu-present-skipped", present.presentId,
                        present.screenVI.fbAddress(), present.fbOperations.size(), 0U, 0U, 0.0f,
                        present.workloadId, 0U);
                    skipInterpolation();]=])

    jfg_video_replace([=[#include "rt64_present_queue.h"]=] [=[#include "rt64_present_queue.h"
#include "rt64_cpu_overlay.hpp"]=])
    jfg_video_replace([=[        RenderTarget *colorTarget = nullptr;]=] [=[        RenderTarget *colorTarget = nullptr;
        std::shared_ptr<const jfg::CpuOverlayFrame> jfgCpuOverlay;]=])

    jfg_video_replace([=[                commandList->begin();
                commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(swapChainTexture, RenderTextureLayout::COLOR_WRITE));]=] [=[                commandList->begin();
                bool jfgPresentOverlayApplied = false;
                // Texture allocations are padded to 32 pixels; the visible
                // framebuffer dimensions remain 320x240.
                // VI-time CPU writes use a different pool than submit-time
                // writes. Its entries remain owned by this Present until this
                // entire sequence finishes, including the GPU fence per output.
                if (presentationMode == EnhancementConfiguration::Presentation::Mode::SkipBuffering &&
                    !jfg::diagnostic_present_overlay_baseline() && colorTarget && !colorTarget->isEmpty() &&
                    present.screenVI.fbSiz() == G_IM_SIZ_16b &&
                    present.screenVI.fbSize().x == 320U && present.screenVI.fbSize().y == 240U) {
                    const std::scoped_lock changesLock(screenFbChangePoolMutex);
                    for (const auto& operation : present.fbOperations) {
                        if (operation.type != FramebufferOperation::Type::WriteChanges ||
                            operation.writeChanges.address != present.screenVI.fbAddress()) continue;
                        const auto* changes = screenFbChangePool.get(operation.writeChanges.id);
                        if (!changes || !changes->used || changes->type != FramebufferChange::Type::Color ||
                            changes->width < 320U || changes->height < 240U) continue;
                        colorTarget->copyFromChanges(ext.presentGraphicsWorker, *changes,
                            320U, 240U, 0U, ext.shaderLibrary);
                        jfgPresentOverlayApplied = true;
                        jfg::trace_interpolation("cpu-present-overlay", jfgOutputSample.tick,
                            present.screenVI.fbAddress(), operation.writeChanges.address, unsigned(i),
                            frameCounters.count, jfgOutputSample.alpha, present.presentId, operation.writeChanges.id);
                    }
                }
                // A fresh VI mask supersedes an older cached mask; do not
                // leave two reticles when the cursor moves between updates.
                if (!jfgPresentOverlayApplied && jfgCpuOverlay && colorTarget && !colorTarget->isEmpty()) {
                    const auto* changes = jfgCpuOverlay->pool.get(jfgCpuOverlay->id);
                    if (changes) {
                        colorTarget->copyFromChanges(ext.presentGraphicsWorker, *changes,
                            jfgCpuOverlay->width, jfgCpuOverlay->height, 0U, ext.shaderLibrary);
                        jfg::trace_interpolation("cpu-overlay-presented", jfgOutputSample.tick,
                            present.screenVI.fbAddress(), jfgCpuOverlay->address, unsigned(i),
                            frameCounters.count, jfgOutputSample.alpha, jfgCpuOverlay->tick, 0U);
                    }
                }
                commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(swapChainTexture, RenderTextureLayout::COLOR_WRITE));]=])

    jfg_video_replace([=[        bool lockedWorkloadMutex = false;
        InterpolatedFrameCounters &frameCounters = ext.sharedResources->interpolatedFrames[ext.sharedResources->interpolatedFramesIndex];]=] [=[        bool lockedWorkloadMutex = false;
        const uint32_t jfgTargetBank = ext.sharedResources->interpolatedFramesIndex;
        InterpolatedFrameCounters &frameCounters = ext.sharedResources->interpolatedFrames[jfgTargetBank];]=])
    jfg_video_replace([=[        int32_t framesToPresent = 1;]=] [=[        int32_t framesToPresent = 1;
        bool jfgRepeatCompleted = false;
        bool jfgRepeatHasTarget = false;
        uint32_t jfgRepeatTarget = 0;]=])
    jfg_video_replace([=[                if (presentFb->interpolationEnabled) {
                    framesToPresent = frameCounters.count;
                }]=] [=[                if (presentFb->interpolationEnabled) {
                    std::scoped_lock<std::mutex> lock(ext.sharedResources->interpolatedMutex);
                    const bool jfgCaptureRequested = jfg::presentation_capture_pending().load(std::memory_order_acquire);
                    jfgRepeatCompleted = frameCounters.count > 1U &&
                        frameCounters.presented >= frameCounters.count;
                    if (jfgRepeatCompleted) {
                        jfg::trace_interpolation("completed", present.presentId, viOriginalRate,
                            targetRate, 0U, frameCounters.count, 1.0f,
                            present.fbOperations.size(), present.paused ? 1U : 0U);
                        if (jfg::interpolation_trace_enabled()) for (const auto& operation : present.fbOperations) {
                            if (operation.type == FramebufferOperation::Type::WriteChanges)
                                jfg::trace_interpolation("completed-write", present.presentId,
                                    presentFb->addressStart, operation.writeChanges.address,
                                    static_cast<unsigned>(operation.type), frameCounters.count,
                                    0.0f, operation.writeChanges.id, 0U);
                        }
                    }
                    if (jfgRepeatCompleted && !present.paused && present.fbOperations.empty() && !jfgCaptureRequested &&
                        frameCounters.count != jfg::kUnlimitedPresentation) {
                        // CPU framebuffer writes are real new content, not a
                        // redundant VI; preserve their existing presentation path.
                        // This VI names a scene whose entire finite sequence
                        // was already consumed. A second full output window
                        // freezes motion and makes the next scene arrive late.
                        // Release the producer immediately; its next scene owns
                        // the next interpolation window, not this duplicate VI.
                        notifyPresentId(present);
                        return;
                    }
                    if (jfgRepeatCompleted) {
                        // A later VI can refer to an already consumed scene.
                        // Replaying its old sequence rewinds motion at capped rates;
                        // the unlimited ring additionally overwrites old slots.
                        // Repeat the latest image, never restart old motion.
                        // CPU-write VIs update both the base and retained endpoint.
                        // They do not own another scene-time window:
                        // holding the endpoint for that whole window delays the
                        // next scene even though the output counter stays high.
                        // Paused presentation retains its existing refresh loop.
                        if (present.paused && frameCounters.count != jfg::kUnlimitedPresentation && !jfgCaptureRequested)
                            framesToPresent = frameCounters.count;
                        jfgRepeatHasTarget = frameCounters.available > 0;
                        jfgRepeatTarget = jfgRepeatHasTarget ?
                            (frameCounters.count == jfg::kUnlimitedPresentation ?
                                ((frameCounters.available - 1) % jfg::kPresentationRingSize) :
                                ((frameCounters.available - 1) * 2U + jfgTargetBank)) : 0;
                        jfg::trace_interpolation("repeat", present.presentId, viOriginalRate,
                            targetRate, jfgRepeatTarget, frameCounters.count, 1.0f, 0U, 0U);
                    }
                    else framesToPresent = frameCounters.count;
                }]=])
    jfg_video_replace([=[                RenderTargetKey colorTargetKey(presentFb->addressStart, presentFb->width, presentFb->siz, Framebuffer::Type::Color);]=] [=[                if (jfgRepeatCompleted) {
                    lockedWorkloadMutex = true;
                    ext.sharedResources->workloadMutex.lock();
                }
                RenderTargetKey colorTargetKey(presentFb->addressStart, presentFb->width, presentFb->siz, Framebuffer::Type::Color);]=])
    jfg_video_replace([=[                if (!present.paused && (viHistory.top().vi != present.screenVI)) {
                    viHistory.pushVI(present.screenVI, viFb->width);]=] [=[                if (jfgRepeatHasTarget) {
                    {
                        std::scoped_lock<std::mutex> targetsLock(ext.sharedResources->interpolatedMutex);
                        colorTarget = ext.sharedResources->interpolatedColorTargets[jfgRepeatTarget].get();
                    }
                    // The ordinary operation pass updated the base framebuffer.
                    // A completed scene displays its retained endpoint instead.
                    // Apply the same changed-pixel mask to that endpoint, keeping
                    // all untouched interpolated pixels and the original phase.
                    if (colorTarget && !colorTarget->isEmpty() && !present.fbOperations.empty() &&
                        (presentationMode != EnhancementConfiguration::Presentation::Mode::SkipBuffering ||
                            jfg::diagnostic_present_overlay_baseline() ||
                            present.screenVI.fbSiz() != G_IM_SIZ_16b ||
                            present.screenVI.fbSize().x != 320U || present.screenVI.fbSize().y != 240U)) {
                        const std::scoped_lock changesLock(screenFbChangePoolMutex);
                        RenderWorkerExecution workerExecution(ext.presentGraphicsWorker);
                        for (const auto& operation : present.fbOperations) {
                            if (operation.type != FramebufferOperation::Type::WriteChanges ||
                                operation.writeChanges.address != presentFb->addressStart) continue;
                            const auto* changes = screenFbChangePool.get(operation.writeChanges.id);
                            if (!changes || changes->type != FramebufferChange::Type::Color) continue;
                            colorTarget->copyFromChanges(ext.presentGraphicsWorker, *changes,
                                presentFb->width, presentFb->height, 0U, ext.shaderLibrary);
                            jfg::trace_interpolation("completed-write-applied", present.presentId,
                                presentFb->addressStart, operation.writeChanges.address,
                                jfgRepeatTarget, frameCounters.count, 1.0f, operation.writeChanges.id, 0U);
                        }
                    }
                }
                if (!present.paused && (viHistory.top().vi != present.screenVI)) {
                    viHistory.pushVI(present.screenVI, viFb->width);]=])
    jfg_video_replace([=[        // Since the swap chain might not need a resize right away, detect present wait.]=] [=[        ext.swapChain->setVsyncEnabled(jfg::presentation_vsync());
        // Since the swap chain might not need a resize right away, detect present wait.]=])
    jfg_video_replace([=[                if ((presentTimestamp != Timestamp()) && (targetRate > 0) && (targetRate > viOriginalRate)) {]=] [=[                if (!jfg::presentation_unlimited() && (presentTimestamp != Timestamp()) && (targetRate > 0) && (targetRate > viOriginalRate)) {]=])
    jfg_video_replace([=[                if (presentWaitEnabled) {]=] [=[                if (presentWaitEnabled && jfg::presentation_vsync()) {]=])
    jfg_video_replace([=[                    colorTarget = ext.sharedResources->interpolatedColorTargets[targetIndex].get();]=] [=[                    const bool jfgUnlimited = frameCounters.count == jfg::kUnlimitedPresentation;
                    const uint32_t storageIndex = jfgUnlimited ? (targetIndex % jfg::kPresentationRingSize) : (targetIndex * 2U + jfgTargetBank);
                    colorTarget = ext.sharedResources->interpolatedColorTargets[storageIndex].get();]=])
    jfg_video_replace([=[            uint32_t swapChainIndex = 0;]=] [=[            // available/presented normally count only extra targets, whereas
            // count includes the base frame. Mark the whole sequence consumed
            // after its final output so the next VI cannot start it again.
            if (framesToPresent > 1 && i + 1 == framesToPresent)
                frameCountersNextPresented = frameCounters.count;
            uint32_t swapChainIndex = 0;]=])
    jfg_video_replace([=[            if ((framesToPresent > 1) && (usingMSAA || (i > 0))) {]=] [=[            if (!jfgRepeatCompleted && (framesToPresent > 1) && (usingMSAA || (i > 0))) {]=])
    jfg_video_replace([=[            uint32_t swapChainIndex = 0;]=] [=[            const unsigned jfgSampleIndex = jfgRepeatHasTarget ? jfgRepeatTarget :
                ((!usingMSAA && i == 0) ? UINT32_MAX :
                    (frameCounters.count == jfg::kUnlimitedPresentation ?
                        (unsigned(i) - unsigned(!usingMSAA)) % jfg::kPresentationRingSize :
                        (unsigned(i) - unsigned(!usingMSAA)) * 2U + jfgTargetBank));
            const auto jfgOutputSample = jfg::presentation_output_sample(&frameCounters, jfgSampleIndex);
            jfg::current_capture_output() = jfgOutputSample;
            uint32_t swapChainIndex = 0;]=])
    jfg_video_replace([=[                if (swapChainValid) jfg::frame_diagnostic_state().presented();]=] [=[                if (swapChainValid) {
                    jfg::frame_diagnostic_state().presented();
                    jfg::trace_interpolation("display", jfgOutputSample.tick, jfgOutputSample.source,
                        targetRate, jfgOutputSample.frame, jfgOutputSample.count,
                        jfgOutputSample.alpha, present.presentId, jfgSampleIndex);
                }]=])

    jfg_video_replace([=[            jfg::current_capture_output() = jfgOutputSample;]=] [=[            jfg::current_capture_output() = jfgOutputSample;
            if (i == 0 && presentationMode == EnhancementConfiguration::Presentation::Mode::SkipBuffering &&
                !jfg::diagnostic_cpu_write_baseline())
                jfgCpuOverlay = jfg::cpu_overlay_for(present.screenVI.fbAddress(), jfgOutputSample.tick);]=])
    set(contents "${contents}" PARENT_SCOPE)
endfunction()
