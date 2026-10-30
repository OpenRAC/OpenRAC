# Arm ASR temporal upscaling

Arm Accuracy Super Resolution is a temporal upscaler derived from AMD FSR 2 and made for phone
GPUs: most of its passes are fragment shaders with explicit 16-bit arithmetic, which keep their
work in tile memory on tile-based GPUs, where FSR 3.1's compute passes cost most. The temporal
module (`enhanced/fsr3`) uses it in place of FSR 3.1 behind the same frame hooks and inputs.

## Interface

- `asr_context.h`: `AsrContext`, with the same calls and inputs as `fsr3_detail::SdkContext`
  (colour, float depth, motion, reactive mask, output), and `preferred()`.

`preferred()` follows `OPENRAC_TEMPORAL_UPSCALER` (`asr` or `fsr3`) when it is set, otherwise
`vkgl::tile_based()`. ASR needs shaderFloat16, shaderInt16, shaderSubgroupExtendedTypes and
fragmentStoresAndAtomics: its prebuilt SPIR-V uses 16-bit types, in subgroup operations too, and
its fragment passes write storage images. Where it cannot start, the module logs why and keeps
FSR 3.1. The capture report's `enhanced` line names the upscaler in use.

## Depends on

The Vulkan backend and the MIT-licensed Arm ASR generic library, release 25.06, which
`cmake/ArmAsr.cmake` fetches at commit `0501f490bd9946a2e1806b5363d7ab8a9a6a5e0a`. Its shaders come
prebuilt as SPIR-V headers in that library; nothing is compiled from HLSL. Its Vulkan backend is
built against OpenRAC's volk loader: `asr_compat.h`, force-included, brings `volk.h` and the standard headers the library leaves out. `OPENRAC_ARM_ASR=OFF` builds
`asr_stub.cpp` instead, and FSR 3.1 alone upscales.

## Quality

ASR runs its Quality shader preset at the same render scale, jitter sequence, sharpening and
reactive mask as FSR 3.1. It is FSR 2 based, so its image differs from FSR 3.1's in detail; set
`OPENRAC_TEMPORAL_UPSCALER=fsr3` to compare on the same device.
