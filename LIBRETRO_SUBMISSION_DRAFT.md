# Libretro submission draft

Status: **review draft only**. This document does not open a pull request and
does not assume that Libretro has already created a dedicated SM2-Emu source
repository.

The structure follows the presentation used for
[libretro/Libretro-Supermodel#13](https://github.com/libretro/Libretro-Supermodel/pull/13),
adapted to the implemented and verified SM2-Emu features.

## Recommended submission route

As of 21 September 2026, the Libretro GitHub organization has no repository
dedicated to SM2-Emu or Sega Model 2. This does not block the initial proposal:
`libretro-super` already supports cores whose source remains in an external
public repository.

The first proposed pull request therefore targets
[`libretro/libretro-super`](https://github.com/libretro/libretro-super) and
would add:

- the SM2-Emu core rule, pointing to
  `https://github.com/Zer0one/sm2-emu-libretro.git` with recursive submodules;
- `dist/info/sm2_libretro.info`;
- only the desktop build recipes demonstrated by the project CI and accepted
  by the Libretro build infrastructure.

Any official Libretro mirror or dedicated repository can be discussed with
the maintainers after review. It is not required for the first proposal and
must not turn this project into a GitHub fork of the standalone emulator.

## Items to complete before submission

1. Finalize and commit the current framebuffer timing-overlay work, then replace
   the validation placeholders below with the resulting commit and green CI run.
2. Exercise the proposed `libretro-super` rule in its actual build environment.
   The project currently builds with CMake, while Libretro uses both central
   recipes and per-core GitLab CI depending on the target architecture.
3. Recheck the final `.info` metadata against the accepted recipe, including
   serialized Save State capability and the external `games.xml` system asset.
4. Obtain an upstream clarification of the licensing text. The root `LICENSE`
   declares BSD-3-Clause, but source headers retained from upstream add a
   restriction on commercial or monetary use. Until that relationship is
   clarified, presenting the complete source as unqualified BSD-3-Clause would
   be ambiguous.

## Proposed pull request

**Target:** `libretro/libretro-super:master`

**Proposed title:** `Add the experimental SM2-Emu Sega Model 2 core`

### Proposed body

## Summary

This PR registers **SM2-Emu**, an experimental Sega Model 2 Libretro core, and
adds its frontend metadata and supported desktop build recipes.

The source is maintained at
[`Zer0one/sm2-emu-libretro`](https://github.com/Zer0one/sm2-emu-libretro).
It is an independent Libretro adaptation of
[`dmanlfc/sm2-emu`](https://github.com/dmanlfc/sm2-emu), currently aligned with
standalone version 0.9.13. The standalone source history is retained for
attribution and future updates, while the Libretro adapter remains isolated
under `src/libretro/`.

There is currently no dedicated SM2-Emu repository in the Libretro
organization. This proposal therefore builds the existing public source
repository directly. An official mirror or repository can be considered
separately by the maintainers.

## Libretro integration

- implements the complete content lifecycle, reset, audio/video delivery,
  geometry changes and clean shutdown;
- supports ZIP and 7z content through full-path loading and identifies games
  from the matching `system/sm2-emu/games.xml` database;
- keeps Libretro callbacks, options, input adaptation and frontend persistence
  inside the adapter instead of introducing frontend dependencies into the
  emulated machine;
- provides Core Options v2, input descriptors, frontend-managed Save RAM,
  serialized Save States and game-aware option visibility;
- preserves frontend runtime state needed by rewind and run-ahead while
  validating content identity and rejecting malformed state data.

## Input and controls

- provides reviewed per-game profiles for driving, fighting, joystick,
  twin-stick, lightgun and special-control cabinets;
- exposes only controls supported by the loaded game, with separate profile
  variants for normal play and Service/Test access;
- supports Lightgun, Mouse and Analog Stick pointing paths, off-screen reload,
  optional crosshairs and two-player gun input;
- maps centered and unidirectional analog controls without duplicating
  frontend analog-to-digital conversion;
- uses the Libretro rumble interface for supported drive-board and recoil
  commands, with lifecycle-safe shutdown behavior.

## NVRAM and persistence

- stores native backup RAM and EEPROM in frontend-managed `.srm` files through
  `RETRO_MEMORY_SAVE_RAM`;
- provides optional per-game NVRAM Settings based on an authoritative catalog
  reconstructed from Service Menu captures;
- applies field-level changes while preserving unrelated operator settings,
  redundant copies and each game's verified checksum or CRC algorithm;
- provides Automatic Initial NVRAM Setup for settings required at first boot,
  including agreed region, cabinet and I/O defaults;
- covers parent and clone layouts independently when their menus, defaults or
  integrity data differ.

## Rendering, timing and audio

- provides the upstream software renderer plus frontend-owned Vulkan and
  OpenGL hardware contexts; the core creates no SDL window or swapchain;
- reuses the upstream 3D and 2D passes for internal resolution scaling,
  xBR and ScaleFX enhancement, stipple transparency and blended translucency;
- supports automatic 4:3/16:9 geometry for the verified wide cabinet modes;
- composites crosshairs and the optional timing/FPS diagnostic overlay inside
  the core framebuffer so their scale and background remain renderer-neutral;
- offers native Model 2 timing and an optional speed-preserving 60 Hz cadence,
  with fractional audio packetization and board-specific sample rates;
- provides separate DSB/MPEG music volume and optional enhanced audio balance.

## Linked cabinets

- implements experimental linked-cabinet play through the official Libretro
  Netpacket interface;
- preserves the emulated Model 2 communication protocols and replaces only
  the standalone transport at the frontend boundary;
- supports the verified master/slave, twin and relay/live-monitor topologies,
  including sessions distributed across different host platforms;
- keeps participant limits and per-game roles visible only for titles whose
  captured Service Menu exposes the required network settings;
- disables Save States during linked sessions because restoring one cabinet
  independently cannot restore the shared network state safely.

## Upstream relationship

- starts from the original SM2-Emu history and records the standalone revision
  used as the integration baseline;
- keeps upstream-facing changes small and documented, with the Libretro ABI and
  frontend behavior isolated from machine and CPU code;
- selectively integrates standalone changes through 0.9.13 that are relevant to the core
  while leaving SDL UI, desktop configuration and packaging changes outside the
  Libretro target;
- retains `dmanlfc/sm2-emu` as the upstream source reference without claiming a
  GitHub fork relationship.

## Validation

The final submission head (`<commit>`) passes the five-platform project CI in
[`<run>`](<CI URL>):

- Linux x86_64;
- Linux ARM64;
- Windows x86_64;
- macOS x86_64;
- macOS ARM64.

Every job builds Software, Vulkan and OpenGL support where applicable, runs
ROM-free checks for the Libretro ABI, input profiles, Save RAM/Core Options,
Save States, Netpacket and rumble, and verifies the packaged core, game database,
licenses and checksums.

Real-ROM comparison covered all four Model 2 board generations with matching
video, PCM and NVRAM against the frontend-free baseline. Runtime testing also
covered RetroArch on macOS ARM64, Windows x86_64 and Batocera Linux x86_64,
including gameplay, audio, all three render paths, Save States and frontend
persistence.

Linked-cabinet sessions were exercised between local RetroArch instances and
across macOS/Batocera and macOS/Windows hosts. Tested topologies include
two-player races, three-participant relay sessions and the supported maximum
rosters used by the automated runner.

No ROM, NVRAM sample or copyrighted game data is included in the repository,
CI artifacts or release packages.

## Known limitations and reviewer attention

- the core and linked-cabinet transport are intentionally marked experimental;
- Vulkan requires Vulkan 1.3 and the OpenGL paths require OpenGL 4.3 or OpenGL
  ES 3.1; software rendering remains available;
- the `games.xml` version packaged with the core is a required system asset;
- linked play depends on matching content, NVRAM roles and core versions across
  every participant;
- renderer integration, Save State validation, per-game NVRAM mutation and
  Netpacket transport deserve particular independent review.

## AI-assisted development disclosure

The Libretro adaptation was developed as a study project with substantial
assistance from OpenAI Codex.

The human maintainer defined requirements, selected implementation directions
and performed extensive real-ROM and cross-platform validation. Codex assisted
with source analysis, implementation, debugging, documentation and automated
testing.

CI and runtime tests provide validation evidence but are not a substitute for
independent human review, particularly for renderer integration, Save States,
NVRAM handling and linked-cabinet transport.

## Review notes

The final PR body should contain only the text beginning at **Summary**. The
submission route and prerequisites above are private preparation notes for this
repository and should be removed from the text pasted into GitHub.

The initial `libretro-super` change should stay narrow: registration, metadata
and verified recipes. Source changes belong in this repository, not in the
catalog PR.
