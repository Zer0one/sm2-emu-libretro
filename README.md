# SM2-Emu Libretro

[![CI](https://github.com/Zer0one/sm2-emu-libretro/actions/workflows/libretro-ci.yml/badge.svg)](https://github.com/Zer0one/sm2-emu-libretro/actions/workflows/libretro-ci.yml)
[![Latest release](https://img.shields.io/github/v/release/Zer0one/sm2-emu-libretro)](https://github.com/Zer0one/sm2-emu-libretro/releases/latest)
[![License](https://img.shields.io/github/license/Zer0one/sm2-emu-libretro)](LICENSE)

SM2-Emu Libretro is a Libretro port of
[SM2-Emu](https://github.com/dmanlfc/sm2-emu), an emulator for Sega Model 2
arcade hardware. It keeps the original source layout and history while placing
the frontend integration in `src/libretro` to simplify future upstream updates.

The core is playable across the Model 2, Model 2A, Model 2B, and Model 2C board
families. Automated release builds are available for Linux, macOS, and Windows.

## Download

Download the current platform package from
[GitHub Releases](https://github.com/Zer0one/sm2-emu-libretro/releases/latest).

Each package contains:

- the Libretro core and matching `.info` file;
- `system/sm2-emu/games.xml`;
- licence notices and checksums.

ROMs are not included. Use legally obtained sets that match the game definitions
shipped with the core.

## Installation

1. Copy `sm2_libretro` to the frontend's core directory.
2. Copy `sm2_libretro.info` beside the core when the frontend uses Libretro
   information files.
3. Copy the packaged `system/sm2-emu` directory into the frontend's system
   directory. The resulting path must be:

   ```text
   <system directory>/sm2-emu/games.xml
   ```

4. Load the core, then open a supported `.zip` or `.7z` ROM set.

Save RAM is managed by the frontend as a normal `.srm` file.

## Features

- Software rendering plus Vulkan and OpenGL hardware rendering at 1x–4x
  internal resolution.
- Native Model 2 timing and an optional 60 Hz compatibility mode.
- Game-aware RetroPad, analog, mouse, and lightgun profiles.
- Per-game operator settings backed by validated NVRAM layouts.
- Automatic first-boot NVRAM setup for required cabinet and region defaults.
- Save RAM and Libretro Save State support.
- Crosshairs, mouse-edge off-screen reload, gamepad rumble, and service controls.
- Optional upstream 2D enhancement filters and configurable audio balance.
- Experimental linked-cabinet play through the Libretro Netpacket interface.
- In-frame timing overlay for repeatable performance diagnostics.

Rendering, input, NVRAM, and networking options are exposed through RetroArch's
Core Options menu. Options that represent operator settings are shown only for
the loaded game.

## Platform notes

| Platform | Release target | Notes |
| --- | --- | --- |
| Linux x86_64 | `sm2_libretro.so` | LTO-enabled release build; tested on Batocera |
| Linux arm64 | `sm2_libretro.so` | Native CI build |
| macOS Apple Silicon | `sm2_libretro.dylib` | Vulkan uses MoltenVK |
| macOS Intel | `sm2_libretro.dylib` | Vulkan uses MoltenVK |
| Windows x86_64 | `sm2_libretro.dll` | MinGW release build |

Vulkan requires a Vulkan 1.3-capable driver with the features requested by the
core. OpenGL requires desktop OpenGL 4.3; supported ARM builds may use OpenGL ES
3.1. The software renderer uses the frontend's standard video output.

## Build from source

Clone the repository with its submodules:

```sh
git clone --recurse-submodules https://github.com/Zer0one/sm2-emu-libretro.git
cd sm2-emu-libretro
```

Configure and build the core with CMake and Ninja:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSM2_BUILD_STANDALONE=OFF \
  -DSM2_BUILD_TESTS=OFF \
  -DSM2_BUILD_LIBRETRO=ON \
  -DSM2_LIBRETRO_VULKAN=ON \
  -DSM2_LIBRETRO_OPENGL=ON \
  -DSM2_LIBRETRO_CHECKS=ON

cmake --build build --parallel
```

For Linux x86_64 release builds, add `-DSM2_LTO=ON`. The exact dependencies and
commands used for every supported platform are recorded in
[the CI workflow](.github/workflows/libretro-ci.yml).

## Documentation

- [Core build, configuration, and validation](LIBRETRO.md)
- [Control profiles](CONTROL_PROFILES.md)
- [Game Settings catalog](GAME_SETTINGS_CATALOG.md)
- [NVRAM Settings audit](NVRAM_SETTINGS_AUDIT.md)
- [Architecture and frontend boundaries](LIBRETRO_DESIGN.md)
- [Continuous integration and release packages](CI.md)
- [Roadmap](PORTING_PLAN.md)

The NVRAM acquisition and linked-cabinet test tools are documented under
[`scripts/`](scripts/). Detailed validation evidence remains in the technical
documents instead of this project overview.

## Upstream relationship

This is an independent repository rather than a GitHub fork. The original
SM2-Emu project remains the upstream source reference:

```sh
git remote add upstream https://github.com/dmanlfc/sm2-emu.git
git fetch upstream
```

Please confirm that an emulation issue also occurs in the current standalone
SM2-Emu build before reporting it to the upstream project.

## Licence

SM2-Emu is distributed under the BSD 3-Clause licence. See [LICENSE](LICENSE),
[NOTICE](NOTICE), source headers, and the third-party licence files for complete
terms and attribution.

No ROMs, game data, or other copyrighted arcade assets are distributed by this
repository.
