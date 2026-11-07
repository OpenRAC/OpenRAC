# iPhone and iPad

The iOS target uses the same native C/C++ game engine, SDL UIKit/audio/input,
and Vulkan through MoltenVK. Gameplay needs no JIT. The launcher uses the
existing Svelte interface and shared Rust core through a small iOS adapter.

## Ownership

| Location | Responsibility |
| --- | --- |
| `launcher/src` | Shared UI and optional host capability checks |
| `launcher/core` | Platform-independent catalogue, settings, controls, saves and reports |
| `launcher/ios` | Rust C ABI adapter; the only launcher crate producing an iOS static library |
| `port/ios` | UIKit lifecycle, Files access, signed importer runtime and native engine handover |
| `port/cmake/IosApp.cmake` | iOS bundle and framework assembly |
| `tools/build_ios.py` | Reproducible frontend, Rust and CMake build entry point |

Desktop builds do not depend on `launcher/ios`. The installed apps and the
working device branch can be kept while reviewing this integration separately.

## Build

Use macOS, Xcode, CMake, Ninja, Python 3.10+, Rust and the Yarn version named in
`launcher/package.json`. Install the frontend dependencies with `yarn install
--immutable` in `launcher`, and add the Rust target with `rustup target add
aarch64-apple-ios`.

Obtain the official MoltenVK 1.4.2 iOS distribution and Python Apple Support
3.14-b14 iOS XCFramework. Preserve their supplied licenses. The Python archive
used by the public bridge check has SHA-256
`703ff822529f46edcbcd8b88096b2544f6990f3109ad38b4b4b61bb489a7c1ee`.
These frameworks are build inputs outside the checkout, never source commits.

```sh
python3 tools/build_ios.py --build-dir /private/path/build-ios \
  --moltenvk /private/path/MoltenVK.xcframework/ios-arm64/MoltenVK.framework \
  --python-xcframework /private/path/Python.xcframework \
  --disc /private/path/SCUS_971.99.iso --team YOUR_TEAM \
  --bundle-id your.reverse.domain.openrac
```

The command builds the shared frontend, `openrac-launcher-ios` for
`aarch64-apple-ios`, and the native application. To use already-built launcher
artifacts, pass `--skip-launcher-build --launcher-library /private/path/libopenrac_launcher_ios.a`.
The output is `OpenRAC.app`; sign all its frameworks and then the application
with an appropriate provisioning profile before installation.

`--build-number` sets Apple's build number. `--store-url` optionally sets the
App Store HTTPS page of a distribution. The application version comes from
`launcher/package.json`. No personal team, Store record or signing profile is
required in the repository. Builds default to `dev.openrac.launcher.ios`.

Enable **Extended Virtual Addressing** for the chosen App ID and sign with
`port/ios/OpenRAC.entitlements`: the engine reserves a 4 GB address range, with
only the game's regions backed by physical pages. Deployment starts at iOS 17;
that minimum is not a performance claim for every supported device.

## Engines and game data

`port/ios/engines.json` registers supported engine/importer pairs. The bundle
configuration generator reads disc sizes, boot checksums and serials from the
shared game catalogue; UIKit and the Rust adapter consume this generated file.
A new engine needs its native port and signed importer, not just a catalogue
entry. The current registered engine is `rac1-ntsc`; its specific export steps
live in `port/ios/importers/rac1.py`.

Use **Set up from your disc** and choose your own NTSC-U ISO in Files. The app
copies it into `Documents/OpenRAC/active/rac1/data`, validates it and prepares
the levels locally with the existing exporter. CPython runs bundled preparation
code only; no code from the disc is executed or downloaded. A failed or
cancelled import remains separate from the last working copy. Previous imports
are retained, and existing saves are preserved. Keep the app open during import.

Legacy `Documents/data` installations are moved into the active game folder
when it has no existing installation. Prepared desktop data is accepted by the
import backend as well. The public bundle contains code, libraries, notices and
the desktop catalogue's thumbnails, tiles and game-page backgrounds. Retail discs and extracted gameplay
data must remain outside the application bundle and Git.

The current iOS host supports native play, local disc preparation, game fixes,
control settings, save inspection/backups and reports. Mod loading, save editing
or transfer, desktop folder opening/scanning and self-updating are not enabled.
The host reports these limits to the shared UI. Unsupported commands fail
explicitly rather than simulating successful operations.

During gameplay, the on-screen options button opens the same touch drawer as
Android. Under Touch controls, change action/classic buttons, button size and
camera speed, or choose Edit the on-screen buttons to drag the controls. The
game pauses during editing; Done saves the layout and returns to the game.

## Controller navigation

The launcher reads connected extended gamepads through Apple's GameController
framework, including DualSense and Xbox controllers, and forwards the same
button events as Android to the shared `launcher/src/lib/pad.ts` navigation.
The UIKit root is a `GCEventViewController` with controller UI interaction
disabled, so inputs reach the profiles instead of WebKit's responder chain.
It does not require a touch to activate WebKit's Gamepad API.

| Control | Launcher action |
| --- | --- |
| D-pad or left stick | Move focus |
| Cross / A, or Menu | Activate the focused control |
| Circle / B | Close a menu/dialog, then return to the library |
| L1/R1 / LB/RB | Previous/next tab |
| Triangle / Y | Open available context actions |

A white outline with a dark border marks the controller target independently
of the opened game. This target remains visible and actionable if WebKit
declines programmatic DOM focus; touching the interface clears it.

Select fields cycle through choices with Cross/A or left/right; they do not
open a system picker that cannot receive these navigation events. Files and
confirmation dialogs retain their native interface and may require touch.
Native navigation pauses while these dialogs are open or the app is inactive.
Held buttons must be released before they act again after a dialog or app
transition. Controller polling stops before handing the engine to SDL, so
launcher navigation cannot consume gameplay input.

## Checks and release

`.github/workflows/ios.yml` runs memory/lifecycle tests on macOS, cross-compiles
the iOS runtime/window without a game, tests and cross-compiles the Rust adapter,
and compiles the UIKit/WebKit/Python bridge against the iOS SDK. It needs no
retail image, Apple certificate or signing secret. Full engine builds need the
contributor's private disc; gameplay verification remains a separate device step.

Before release, check physical iPhone/iPad startup, Files import, touch/gamepad
input, sound, saves, suspend/resume and sustained gameplay. Source tests, an
unsigned build, a signed device installation, a Store upload and Apple approval
are separate pieces of evidence. Keep the project's licenses and credits, and
ensure Store descriptions, declarations and screenshots match the tested build.
