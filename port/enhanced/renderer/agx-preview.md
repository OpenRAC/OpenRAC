# AgX

Find **AgX** under **Post Processing** in the in-game Enhanced Settings window. The setting is on by default at 0.25 EV;
`agx_preview = 1` and `agx_exposure = 0.25` can also be saved in settings.ini.
**Exposure (EV)** remains the base exposure (-4 to +4 stops). **Brightness** is independently
saved as `agx_brightness` from 0 to 1, default 0.5. Its offset is `(brightness - 0.5) * 1.6` EV,
so the endpoints add -0.8 and +0.8 EV while the midpoint leaves base exposure unchanged.
Both controls are available only while Color Correction / AgX is enabled. Classic graphics and OpenGL bypass the pass. World colour is transformed after FSR
and motion blur, before native HUD rendering. Loading/menu snapshots bypass the pass.

This is an SDR colour-treatment experiment, not an HDR lighting conversion. Current RGBA8 world
and reconstruction buffers have already clipped highlights. The preview interprets their colour
as sRGB, decodes it to linear Rec.709, applies exposure, converts to Rec.2020 and the AgX inset,
uses a log2 range of -12.47393 to +4.02607 stops and the neutral sigmoid with 40% hue blending,
then applies the outset, converts to Rec.709 and encodes sRGB once. Alpha is preserved.
A separate reusable image prevents framebuffer feedback. No CPU pixel processing or readback is used.
The GPU scope **AgX** includes the source copy and analytical shader cost.

AgX matrices adapted from Google Filament ToneMapper.cpp,
commit be858f42ab95cf0ba6e224ac2dc3950bdb172080 (Apache-2.0, see agx-LICENSE.txt and shader notice).
Neutral curve mathematics follow Blender 4.0 AgX. GLSL matrices are multiplied as row vectors
because their constructor lists are columns. No LUT is used in this prototype.

A production scene-linear version needs HDR world/transparent targets, explicit texture/light colour
encoding and compatible temporal reconstruction; this preview cannot demonstrate highlight recovery.
