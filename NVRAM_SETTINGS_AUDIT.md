# NVRAM Settings consistency audit

Date: 2026-09-19

## Scope

This audit compares the operator Game Settings acquired during the screenshot
campaign with the NVRAM Settings exposed by the Libretro core. It covers:

- the 36 parent sets catalogued in `Docs/revisione_core_options_model2.xlsx`;
- the 35 parents that expose reviewed Core Options (`rascot2` has no acquired
  editable Game Setting);
- all 48 known clones;
- the option labels, order, accepted values, defaults and template routing used
  by `src/libretro/nvram_settings_data.inc` and `src/libretro/core.cpp`.

The archived screenshots and the corresponding YAML files under
`data/diagnostic-menus/` are the source of truth. The game's own Service Menu
has precedence over earlier spreadsheet entries.

## Result

The implemented catalog is complete after the corrections recorded below:

| Check | Result |
| --- | ---: |
| Selected parent options matching the acquired labels, order and values | 197 / 197 |
| Clone-specific options matching their acquired menus | 92 / 92 |
| Total implemented NVRAM Settings accounted for | 289 / 289 |
| Clones with a dedicated option catalog | 14 / 14 |
| Clones intentionally using the compatible parent catalog | 34 / 34 |
| Total clone option routing covered | 48 / 48 |
| Validated dedicated clone templates | 27 |
| Byte-compatible clones inheriting a parent template | 21 |

No acquired Game Setting selected for RetroArch is missing, and no extra
unreviewed Game Setting is exposed.

## Corrected defaults

The strict comparison against the native templates found ten Core Option
defaults that still reflected old spreadsheet entries instead of the values
visible in the archived base screenshots:

| Set | Setting | Previous | Native default |
| --- | --- | --- | --- |
| `indy500` | Engine Volume | 1 | 3 |
| `indy500` | Default View | 1 | 4 |
| `lastbrnx` | Master Volume | 1 | 3 |
| `rchase2` | Game Difficulty | 1/8 | 4/8 |
| `rchase2` | Shifting Difficulty | 10 sec Every | 50 sec Every |
| `stccb` | Default View | Bird's | Driver's |
| `zerogun` | Difficulty | Easy | Normal |
| `zerogun` | Fighters | 1 fighter | 3 fighters |
| `zeroguna` | Difficulty | Easy | Normal |
| `zeroguna` | Fighters | 1 fighter | 3 fighters |

The implementation now uses the native values above. The workbook was also
updated wherever it contains the affected parent rows. Its two stale Gunblade
NY defaults were corrected from 1/8 to the 4/8 values already used correctly by
the core and visible in the base screenshot. `stccb` is clone-specific and has
no row in the parent-only workbook.

`lastbrnx` Display Type remains `C.R.T.`. Selecting it writes captured companion
calibration bytes in addition to the visible field, so byte identity is not a
valid default test for this compound option; its visible default was confirmed
directly from the base screenshot.

## Deliberate policy overrides

These defaults intentionally differ from a game's native factory value:

- Country/Nation uses USA where available, with Export as fallback;
- network-related fields use their safe offline or stand-alone value;
- Daytona USA uses Deluxe cabinet mode;
- Manx TT uses Twin cabinet mode;
- Super GT 24h uses I/O Type C.

The automated checks distinguish these reviewed policies from ordinary native
defaults.

## Verification

`sm2-libretro-save-ram-checks` now verifies:

- all 289 catalog entries and their unique keys;
- ordinary Core Option defaults against each native NVRAM template;
- the deliberate policy exceptions listed above;
- all 14 clone-specific catalogs;
- all 34 clone-to-parent catalog mappings;
- all 27 dedicated clone templates and all supported values;
- integrity and persistence through the Libretro save container.

The complete check passes after the corrections. This audit changes only option
defaults, documentation and regression coverage; it does not alter option
offsets, value encodings or integrity algorithms.
