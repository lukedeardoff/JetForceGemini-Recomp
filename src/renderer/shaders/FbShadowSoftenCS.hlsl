//
// Jet Force Gemini: GPU actor-shadow softening for RT64.
//
// Derived from RT64's src/shaders/FbReinterpretCS.hlsl (MIT, see
// patches/rt64/LICENSE.upstream). Compiled into the pinned RT64 target by
// cmake/rt64_shadow_soften_shader.cmake; the RT64 checkout stays pristine.
//
// Used only for small 8-bit -> IA8 tile reinterpretations (the game's 64x64
// actor shadow masks). It reinterprets like FbReinterpretCS, then:
//   1. box-averages each native texel cell of the high-resolution copy, so
//      thin HD details become partial coverage as in the 64x64 original;
//   2. reconstructs with a Gaussian in native-texel space (soft edge);
//   3. applies the game's shadow opacity (black, alpha scaled), which its
//      CPU pass would otherwise apply to the mask in RDRAM.
//
// Parameters ride in FbReinterpretCB.usesHDR so the upstream constant buffer
// layout is unchanged:
//   bit  0     usesHDR (upstream meaning)
//   bits 1-7   opacity scale in 1/127 units (0 = keep the raw mask)
//   bits 8-19  softness in native texels, 1/64 units
//   bits 20-31 resolution scale, 1/64 units
//

#include "shared/rt64_f3d_defines.h"
#include "shared/rt64_fb_common.h"
#include "shared/rt64_fb_reinterpret.h"

#include "TextureDecoder.hlsli"

[[vk::push_constant]] ConstantBuffer<FbReinterpretCB> gConstants : register(b0);
Texture2D<float4> gInputColor : register(t1);
Texture1D<uint> gInputTLUT : register(t2);
RWTexture2D<float4> gOutput : register(u3);

float4 LoadIA8(int2 hiresCoord, int2 maxCoord) {
    uint2 sampleCoord = uint2(clamp(hiresCoord, int2(0, 0), maxCoord));
    uint2 inputCoord = uint2(floor(sampleCoord.x * gConstants.sampleScale), sampleCoord.y);
    float4 inputColor = gInputColor.Load(uint3(inputCoord, 0));
    // Same channel selection as FbReinterpretCS's ANY8toIA8.
    float inputChannel = (inputCoord.x & 1) ? inputColor.g : inputColor.r;
    return IA8ToFloat4(FloatToUINT8(inputChannel));
}

[numthreads(FB_COMMON_WORKGROUP_SIZE, FB_COMMON_WORKGROUP_SIZE, 1)]
void CSMain(uint2 coord : SV_DispatchThreadID) {
    if ((coord.x >= gConstants.resolution.x) || (coord.y >= gConstants.resolution.y)) {
        return;
    }

    const int2 maxCoord = int2(gConstants.resolution) - 1;
    const float softness = float((gConstants.usesHDR >> 8) & 0xFFFu) / 64.0f;
    const float scale = max(float(gConstants.usesHDR >> 20) / 64.0f, 1.0f);
    const float sigma = max(softness * 0.5f, 0.35f);

    const float2 nativePos = (float2(coord) + 0.5f) / scale - 0.5f;
    const int2 baseCell = int2(floor(nativePos));
    const float cellStep = scale / 3.0f;
    float4 sum = 0.0f;
    float weightSum = 0.0f;
    [unroll]
    for (int cy = -1; cy <= 2; cy++) {
        [unroll]
        for (int cx = -1; cx <= 2; cx++) {
            const int2 cell = baseCell + int2(cx, cy);
            const float2 delta = nativePos - float2(cell);
            const float weight = exp(-dot(delta, delta) / (2.0f * sigma * sigma));
            const float2 cellOrigin = float2(cell) * scale;
            float4 cellSum = 0.0f;
            [unroll]
            for (int sy = 0; sy < 3; sy++) {
                [unroll]
                for (int sx = 0; sx < 3; sx++) {
                    cellSum += LoadIA8(int2(cellOrigin + (float2(sx, sy) + 0.5f) * cellStep), maxCoord);
                }
            }

            sum += (cellSum / 9.0f) * weight;
            weightSum += weight;
        }
    }

    float4 result = sum / max(weightSum, 0.0001f);
    const uint opacityQ = (gConstants.usesHDR >> 1) & 0x7Fu;
    if (opacityQ != 0u) {
        result = float4(0.0f, 0.0f, 0.0f, result.a * (float(opacityQ) / 127.0f));
    }
    gOutput[coord] = result;
}
