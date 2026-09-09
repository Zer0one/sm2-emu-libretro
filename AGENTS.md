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

Current state: upstream macOS baseline built and exercised, development
repository prepared. The Libretro core has not been implemented. Follow the
current user request for scope; PORTING_PLAN.md lists proposed later work.

Distinguish compilation, application launch and actual gameplay verification.
Do not claim runtime input/audio correctness from a successful build.

## Build

Run `./scripts/build-upstream-macos.sh` to rebuild the adjacent mainstream.
The script does not install packages or change upstream sources.
The upstream revision initially checked out is
`8b3a468c5b51387093811cb16b076e6fd9289d66` (version 0.9.4).
