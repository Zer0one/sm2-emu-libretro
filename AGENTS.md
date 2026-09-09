# SM2-Emu Libretro workspace

## Scope and repositories

- `sm2-emu-libretro/`: independent Git repository; develop on `main`.
- `../sm2-emu-mainstream/`: separate, clean upstream clone on `main`.
- They have independent Git metadata and submodules; they are not linked worktrees.
- In the Libretro repository, `upstream` points to
  `https://github.com/dmanlfc/sm2-emu.git` for source reference and future updates.
- Preserve the original Git history for provenance and comparisons. This does not
  require a GitHub fork relationship. The user wants a new standalone repository,
  not a GitHub fork. Do not create a fork.
- `origin` is `https://github.com/Zer0one/sm2-emu-libretro.git`, the standalone
  project repository. Keep `upstream` as the original emulator reference.
- Libretro `main` does not track `upstream/main`; development belongs here.
  Mainstream `main` continues to track its own `upstream/main`.
- Verify branch, status, remotes and submodules before editing. Keep conversion
  changes in the Libretro repository and upstream baseline builds in mainstream.
- Do not commit, push or create a remote repository unless the user asks.

## Working agreement

Use small, reviewable changes and preserve emulation accuracy, timing, audio
and existing controls. Ask before installing/upgrading software or dependencies
unless already authorized in the current session. Never add ROMs to Git.

Current state: upstream macOS baseline built and exercised; frontend-free
`sm2-headless` build verified against upstream on all four board variants.
The first software Libretro core is implemented and verified through ABI
comparisons and VF2 gameplay in RetroArch macOS. See HEADLESS.md and LIBRETRO.md
for evidence and limits. The user then prioritized GPU ahead of profile
implementation: the Vulkan adapter and Video options are implemented and tested
on macOS, including real RetroArch gameplay; see GPU.md. Complete profiles,
Input options and SRAM remain milestone 3.
Follow the current user request for scope; PORTING_PLAN.md lists later work.

Distinguish compilation, application launch and actual gameplay verification.
Do not claim runtime input/audio correctness from a successful build.

## Upstream integration

Future upstream updates are an explicit architectural requirement. Keep the
original source layout and minimize edits to imported files. Put Libretro ABI,
callbacks, options, input adaptation and frontend persistence in `src/libretro/`
with its own CMake target. Dependencies point from adapters to emulation;
machine/CPU code must not include Libretro headers or use frontend callbacks.
Reuse existing machine interfaces; add small frontend-neutral accessors only
when required. Do not duplicate emulation or scatter Libretro conditionals
through upstream code. Avoid unrelated renames, formatting and abstractions.

Keep required upstream-facing changes documented in LIBRETRO_DESIGN.md.
Separate adapter work, generic machine changes and upstream imports when
commits are authorized. Record the integrated upstream revision and submodule
pins; fetch alone does not update that baseline. Before accepting an update,
review upstream API/data changes, build standalone and headless targets, and
compare against a matching pristine upstream build. Compare old and new
baselines separately so upstream behavior changes are not mistaken for port
regressions. Add actual RetroArch checks once the core exists.

## Libretro interface reference

Use the user's `../libretro-supermodel-modern` project as a read-only guide
for menu organization, control profiles and established preferences.
Follow LIBRETRO_DESIGN.md for applicability, defaults and validation rules.
Milestone 3 is split into bounded substeps in PORTING_PLAN.md. Start from the
proposed CONTROL_PROFILES.md catalog (3.1); review it before implementing
recognition/mappings. Metadata gaps are explicit, not guessed profiles.
Preserve canonical actions and add aliases; use frontend remapping and
per-game options. Adapt to SM2 metadata and hardware rather than copying
Model 3 hardware mappings or advertising unimplemented features.
For equivalent controls in Model 2 games and Model 3 sequels, retain the
Supermodel profile name, physical layout and option conventions. Translate
hardware bits/channels per game; expose only verified actions. Follow the
cross-generation matrix in CONTROL_PROFILES.md, including Soccer's bit
permutation and Sega Rally's unresolved Handbrake metadata gap.

## Build

Run `./scripts/build-upstream-macos.sh` to rebuild the adjacent mainstream.
The script does not install packages or change upstream sources.
Use `SM2_BUILD_STANDALONE=OFF` and `SM2_BUILD_HEADLESS=ON` for the
frontend-free runner. Keep it free of SDL/GPU dependencies and compare changes
with `scripts/compare-headless.py` when they affect machine integration.

Use `SM2_BUILD_LIBRETRO=ON` to build the isolated adapter;
`SM2_LIBRETRO_VULKAN=ON` adds the frontend-owned GPU path. Preserve the shared
upstream shaders/passes and keep SDL/swapchain ownership out of Libretro.
The macOS RetroArch build needs the newer MoltenVK selected by the launcher;
never overwrite its app bundle. GPU.md lists lifecycle/comparison tests.
`SM2_LIBRETRO_CHECKS=ON` adds ROM-free input checks. Keep emulator code free of
frontend dependencies. Run `scripts/smoke-libretro.py` for ABI regressions and
see LIBRETRO.md for the separate RetroArch runtime test.

The upstream revision initially checked out is
`8b3a468c5b51387093811cb16b076e6fd9289d66` (version 0.9.4).
