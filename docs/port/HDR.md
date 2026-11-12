# HDR output

The game was made for SDR TVs. On a desktop in HDR mode, SDR games look dim and washed out. HDR
output fixes that: the game's white is shown at a brightness you pick ("paper white"), so the game
looks like it does on an SDR screen.

Bright effects get real HDR light. While HDR is on, the frame is drawn in half floats instead of
bytes. Each draw is still clamped to 0..1, like on an 8-bit frame, so nothing changes on its own.
Only blending can go past white: glows, explosions and lasers that stack up. That light rolls off
smoothly towards the screen's peak. The HUD stays at paper white. This is the same idea as RenoDX.

HDR is off by default. When it is off, nothing here runs and the picture is the same as before,
byte for byte.

## Settings

| Key | Values | Meaning |
|---|---|---|
| `hdr` | `off` (default), `on` | `on`: HDR when the system says the display is in HDR mode. Where it can't say (X11), HDR whenever the screen offers it. |
| `hdr_paper_white` | `0`, `80` to `1000` nits | How bright the game's white is. `0`: the system's SDR brightness, else 203 |
| `hdr_peak` | `0`, `400` to `10000` nits | The brightest the effects may go. `0`: what the display reports, else 1000 |
| `hdr_highlights` | `0` to `100` % (default `100`) | How much light above white the effects get. `0`: as in SDR |
| `hdr_sdr_hue` | `0` (default) to `100` % | Light above white takes the hue it has in SDR |
| `hdr_sdr_chroma` | `0` (default) to `100` % | Light above white takes the saturation it has in SDR |
| `hdr_blowout` | `0` (default) to `100` % | Very bright light fades towards white |

The ranges follow the brightest screens on sale in 2026. Mini-LED TVs (TCL X11L) and some phones
(Honor X80 Pro Max, Samsung Display's M16 panel) are sold at 10000 nits in a small window, which is
also the top of PQ (HDR10), so the peak stops there. Paper white is the whole picture's
brightness (203 by BT.2408); above 1000 the game's white reaches the peak and the highlights stop
standing out, so it stops at 1000. The display's own number (Windows' EDID peak,
Android's desired max luminance or HDR/SDR headroom) stays the default.

You can set the mode in the Video options page, the options drawer, the settings file, or the
launcher's Settings > HDR. The launcher's Off or On is given to every start it makes
(`OPENRAC_HDR`), over the game's own. Paper white, peak and the look's sliders are in the in-game
menu (desktop and Android) and the settings file.

Turning HDR on from `off` needs a restart. Other changes apply at once.

### The look of light above white

In SDR each channel stops at white on its own. So bright colour mixes turn towards yellow and
white: stacked fire sprites look orange. In HDR the mix is kept, so the same fire looks red. The
sliders give back as much of the SDR look as you want, like RenoDX's "SDR Hue" and "SDR
Chrominance". They only change pixels above white; the rest of the picture stays as it is.

- **SDR hue**: the hue of the pixel as SDR shows it, mixed in OKLab. Lightness and saturation stay
  HDR's.
- **SDR chrominance**: the saturation of the pixel as SDR shows it.
- **HDR highlights**: between the SDR pixel (`0`) and the full HDR light (`100`).
- **HDR blowout**: saturation falls as brightness goes past white (a "path to white").

The defaults (`100`, `0`, `0`, `0`) are the plain HDR look, bit for bit.

## Where it works

HDR needs the Vulkan renderer. OpenGL always stays SDR.

| Platform | Format | How `on` knows the display is in HDR |
|---|---|---|
| Windows | HDR10, else scRGB | SDL: HDR on, SDR brightness, peak |
| Linux, Wayland | HDR10, else scRGB | SDL (needs a compositor with colour management) |
| Linux, X11 | whatever the driver offers | it can't: HDR whenever the driver offers it |
| macOS | scRGB (EDR), else HDR10 | SDL: EDR headroom |
| Android 14+ with headroom | scRGB, white = the system's SDR white | `GameActivity.displayHdr` |
| Other Android | HDR10, else scRGB | `GameActivity.displayHdr` |

The options drawer shows what is sent and what the display reported. The log prints the same
line (`HDR: ...`) when it changes.

### Android

Android shows HDR by headroom: the panel gets brighter for HDR parts while SDR stays at the
brightness slider's level. When the display reports headroom, the game sends scRGB where 1.0 is
the system's white, and asks the window for HDR (`COLOR_MODE_HDR`, `setDesiredHdrHeadroom`).
Paper white doesn't apply there, because white follows the slider. The extra light is real, but
only as much as the panel has left: at full brightness there may be none.

Without headroom, the game sends HDR10 if the display supports it. Android's Vulkan lists HDR10
on every display, so the game goes by what the activity says, not by the formats.

## How it works

- `common/hdr.h`: all the math, with no GPU code, so tests can check it. Bytes go to linear
  light with a 2.2 gamma (what monitors really show). Then they are scaled to paper white, and
  turned into PQ in BT.2020 for HDR10. For scRGB, 1.0 is 80 nits.
- Above white, the 2.2 curve continues and then rolls off: `1 + h (1 - e^(-(m - 1) / h))` on the
  brightest channel `m`, where `h` is the headroom over paper white. All channels scale together,
  so colours keep their hue. Nothing at or below white changes.
- The look (`to_light`, `apply_look`): only for pixels above white and only when a slider is not
  at its default. The SDR pixel is the frame clamped to 0..1, as an 8-bit frame would hold it.
  A turned hue can leave BT.2020; the colour then moves towards grey of the same lightness until
  it fits. It never goes past the peak.
- The HDR frame: the game's frame and the default framebuffer are RGBA16F while HDR is on.
  `vkgl::clamp_colour_writes` makes each program use a fragment shader with a clamp at the end
  (`clamped_fragment`, built the first time it's needed). Copies into the game's 8-bit textures
  clamp as before, so the game reads back the same values.
- `renderer/vulkan/surface_format.h` picks the swapchain format. SDR picks the same format as
  before. HDR picks HDR10 or scRGB if the device can write it, else falls back to SDR.
- The Vulkan presentation graph encodes the frame with `shaders/hdr_encode.comp` and transitions
  the result for presentation. If encoder setup fails, the graph uses its SDR copy path.
  `hdr_output.cpp` also sends HDR metadata (peak, MaxCLL = peak,
  MaxFALL = paper white). If the swapchain images can't be written by a shader, the pass writes
  its own image and copies it.

## Tests

- `hdr`: PQ against the standard's values, paper white, the roll-off, the settings logic, and
  the look (fire back to its SDR hue, nothing below 0 or past the peak, grey stays grey).
- `surface_format`: SDR picks what it did before; HDR falls back correctly.
- `vulkan_hdr`: the GPU pass against `common/hdr.h` on lavapipe, and the float frame (clamped
  draws, blending above white, the 8-bit frame stopping at 1). Also a float picture above white
  through the pass with several looks.

`OPENRAC_HDR_FRAME=1` draws the float frame without an HDR screen and logs how much of it is above
white every 60 frames. Use it to compare with the normal frame.

Known differences while HDR is on: blending over a pixel that is already above white starts from
that value, not from 1. Alpha that adds up past 1 also stays there.

To check on a real HDR screen: start with `hdr = on` and Vulkan, look for
`vulkan: HDR output HDR10 at ... nits` in the log, and compare the menu's white and a dark scene
with the same scene in SDR.
