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
| `hdr` | `off` (default), `on` | `on`: HDR whenever the screen offers it. On Android, only when the system says the display is in HDR mode. |
| `hdr_paper_white` | `0`, `80` to `1000` nits | How bright the game's white is. `0`: the system's SDR brightness, else 203 |
| `hdr_peak` | `0`, `400` to `10000` nits | The brightest the effects may go. `0`: what the display reports, else 1000 |
| `hdr_highlights` | `0` to `100` % (default `100`) | Light above white. `0`: as in SDR |
| `hdr_sdr_hue` | `0` (default) to `100` % | Bright colours take their SDR hue |
| `hdr_sdr_chroma` | `0` (default) to `100` % | Bright colours take their SDR saturation |
| `hdr_blowout` | `0` (default) to `100` % | Very bright light fades to white |

The ranges follow the brightest screens on sale in 2026. Mini-LED TVs (TCL X11L) and some phones
(Honor X80 Pro Max, Samsung Display's M16 panel) are sold at 10000 nits in a small window, which is
also the top of PQ (HDR10), so the peak stops there. Paper white is the whole picture's
brightness (203 by BT.2408); above 1000 the game's white reaches the peak and the highlights stop
standing out, so it stops at 1000. The display's own number (Windows' EDID peak,
Android's desired max luminance or HDR/SDR headroom) stays the default.

You can set the mode in the Video options page, the in-game menu or the settings file. The other
keys are in the in-game menu (desktop and Android) and the settings file. `OPENRAC_HDR=off|on`
overrides the mode for one run.

Turning HDR on from `off` needs a restart. Other changes apply at once.

### Bright colours

SDR clips each channel at white, so bright colours shift to yellow and white: fire looks orange.
HDR keeps the mix, so fire looks red. The last four keys bring back the SDR look, like RenoDX's
"SDR Hue" and "SDR Chrominance". They change only pixels above white. The defaults keep the
plain HDR picture, bit for bit.

## Where it works

HDR needs the Vulkan renderer, so `on` starts the game with Vulkan even if OpenGL or Metal was
picked. If Vulkan can't start, the game falls back to OpenGL in SDR.

`on` sends HDR whenever the screen offers an HDR format, even if the system says the display is
not in HDR mode. A compositor can get that wrong (Steam Deck's gamescope). Only Android follows
the system, because its Vulkan offers HDR10 on every display.

| Platform | Format | What the system tells the game |
|---|---|---|
| Windows | HDR10, else scRGB | SDL: HDR on, SDR brightness, peak |
| Linux, Wayland | HDR10, else scRGB | SDL (needs a compositor with colour management) |
| Linux, X11 | whatever the driver offers | nothing |
| macOS | scRGB (EDR), else HDR10 | SDL: EDR headroom |
| Android 14+ with headroom | scRGB, white = the system's SDR white | `GameActivity.displayHdr` (decides) |
| Other Android | HDR10, else scRGB | `GameActivity.displayHdr` (decides) |

When the system says the display is not in HDR mode, its SDR white and peak are not used:
paper white is 203 and the peak 1000 unless you set them.

`on` still gives SDR when:

- the system offers no HDR format: Windows with HDR off, a Wayland compositor without colour
  management, X11 without a driver that offers one, a Steam Deck LCD, or Android saying the
  display is not in HDR mode;
- HDR was `off` at start (restart once);
- Vulkan can't start, or the encode pass can't be built.

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
- Bright colours (`to_light`, `apply_look`): the SDR pixel is the frame clipped to 0..1. Hue and
  saturation are mixed in OKLab; lightness stays HDR's. A colour outside BT.709 moves towards
  grey until it fits. It never goes past the peak.
- The HDR frame: the game's frame and the default framebuffer are RGBA16F while HDR is on.
  `vkgl::clamp_colour_writes` makes each program use a fragment shader with a clamp at the end
  (`clamped_fragment`, built the first time it's needed). Copies into the game's 8-bit textures
  clamp as before, so the game reads back the same values.
- `renderer/vulkan/presentation/surface_format.h` picks the swapchain format. SDR picks the same format as
  before. HDR picks HDR10 or scRGB if the device can write it, directly or through a blit, else falls
  back to SDR.
- The Vulkan presentation graph encodes the frame with `shaders/hdr_encode.comp` and transitions
  the result for presentation. If encoder setup fails, the graph uses its SDR copy path.
  `hdr_output.cpp` also sends HDR metadata (peak, MaxCLL = peak,
  MaxFALL = paper white). If the swapchain images can't be written by a shader, the pass writes
  its own image and copies it. If the screen's format can't be stored at all (A2R10G10B10, or no
  storage support), the pass writes half floats and blits them into it.

## Tests

- `hdr`: PQ against the standard's values, paper white, the roll-off, the settings logic and the
  bright-colour keys.
- `surface_format`: SDR picks what it did before; HDR falls back correctly.
- `vulkan_hdr`: the GPU pass against `common/hdr.h` on lavapipe, and the float frame (clamped
  draws, blending above white, the 8-bit frame stopping at 1), the bright-colour keys, and the
  blit path matching the direct one.

`OPENRAC_HDR_FRAME=1` draws the float frame without an HDR screen and logs how much of it is above
white every 60 frames. Use it to compare with the normal frame.

Known differences while HDR is on: blending over a pixel that is already above white starts from
that value, not from 1. Alpha that adds up past 1 also stays there.

To check on a real HDR screen: start with `hdr = on` and Vulkan, look for
`vulkan: HDR output HDR10 at ... nits` in the log, and compare the menu's white and a dark scene
with the same scene in SDR.
