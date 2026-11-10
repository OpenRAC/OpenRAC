# Enhanced renderer bloom

The enhanced renderer's bloom is intentionally small and conservative. It gives bright authored
materials a soft halo without changing the game's overall contrast or making ordinary geometry look
self-lit.

The pass runs after completed world colour and before the native HUD:

1. half-resolution soft bright pass;
2. a five-level Gaussian reduction pyramid;
3. additive reconstruction from the smallest level;
4. one additive fullscreen composite.

Each reduction pass renders directly into its pyramid texture. Reconstruction samples only the
smaller level and adds it to the larger level using attachment blending. The final composite uses
the same approach to add the half-resolution result to the world colour.

The five RGBA8 pyramid textures take about 10.5 MiB at 3840×2160. The world colour can remain HDR.

Bloom is off by default. In Enhanced Settings, open Post processing to enable it or adjust its
strength, brightness threshold, soft knee and spread live. The slider defaults are 0.780 strength,
0.78 threshold, 0.27 knee and 0.90 spread. Reset bloom restores those values and turns bloom off.

## Lifetime and fallback

Bloom is created only when enhanced rendering is available, which currently means the Vulkan
enhanced renderer. Compatibility rendering is unchanged. Resources are released explicitly before
the graphics context closes and when the frontend resets a world.

If resource preparation or graph recording fails, bloom disables itself for the current graphics
lifetime and leaves subsequent world colour unchanged rather than repeatedly failing every frame.

## Follow-up

Reduce the Gaussian reduction from 25 texel fetches to nine bilinear samples by pairing adjacent
taps on both axes. Preserve the current weights and compare clamped edges and odd-sized targets.

## Test scene

Planet Aridia is a useful visual test because the enhanced material work already marks several
surfaces emissive there. Bloom should be visible around those materials while remaining subtle on
ordinary sunlit geometry and UI.

The GPU regression test checks compatibility bypass, multiple target sizes, alpha preservation,
visible spread outside a bright patch, a weaker default-strength result and Vulkan validation.
