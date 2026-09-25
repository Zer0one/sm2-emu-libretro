# NVRAM Settings consistency audit

Date: 2026-09-20

> This file audits the Libretro implementation. It is not the authoritative
> inventory of Service Menu contents. Presence, order, visible defaults and
> screenshot-backed values are catalogued in `GAME_SETTINGS_CATALOG.md`.

## Scope

This audit compares the operator Game Settings acquired during the screenshot
campaign with the NVRAM Settings exposed by the Libretro core. It covers:

- the 36 parent sets catalogued in `Docs/revisione_core_options_model2.xlsx`;
- the 35 parents that expose reviewed Core Options (`rascot2` has a completed
  special-case acquisition but no ordinary editable Game Settings);
- all 48 known clones;
- the option labels, order, accepted values, defaults and template routing used
  by `src/libretro/nvram_settings_data.inc` and `src/libretro/core.cpp`.

The screenshot-derived `GAME_SETTINGS_CATALOG.md` is the source of truth. The
game's own Service Menu has precedence over YAML, implementation tables and
earlier spreadsheet entries.

## Result

The implemented catalog is complete after the corrections recorded below:

| Check | Result |
| --- | ---: |
| Selected parent options matching the acquired labels, order and values | 197 / 197 |
| Clone-specific options matching their acquired menus | 114 / 114 |
| Total implemented NVRAM Settings accounted for | 311 / 311 |
| Clones with a dedicated option catalog | 17 / 17 |
| Clones intentionally using the compatible parent catalog | 31 / 31 |
| Total clone option routing covered | 48 / 48 |
| Validated dedicated clone templates | 27 |
| Byte-compatible clones inheriting a parent template | 21 |

No acquired Game Setting selected for RetroArch is missing, and no extra
unreviewed Game Setting is exposed. The repeatable audit covers all 84 sets in
`games.xml`, including the catalog that the core resolves for every clone.

## Corrections from the authoritative catalog comparison

The final comparison against `GAME_SETTINGS_CATALOG.md` corrected three Libretro
integration inconsistencies:

- Motor Raid Deluxe now presents `NETWORK TYPE`, `CABINET ID` and then
  `CABINET TYPE`, matching the acquired Service Menu order;
- Sega Rally Championship - DX is counted and tested as the dedicated
  four-option catalog it already implements, instead of being routed through
  the six-option parent catalog;
- Manx TT Superbike - DX now uses its dedicated six-option Core Options subset;
  Cabinet Type, Link Type and the two Revise Mode rows absent from its acquired
  Service Menu are no longer inherited from the parent catalog.
- Sonic the Fighters now uses a dedicated option catalog. Its native
  `Automatic = Off` default is therefore preserved instead of inheriting Sonic
  Championship's `On` default.

Super GT 24h's visible `Link Max = Not Link` is not a selectable field value:
the row becomes editable only when Link Type is `Car No1 Master`, where its
selectable range is 2, 3 or 4. The Core Option therefore correctly defaults to
2 while `Link Type = Not Link` keeps networking disabled.

The same pass corrected three transcription issues in the reference generator:

- Behind Enemy Lines visibly defaults to Difficulty 6, not 1;
- the House of the Dead prototype attempts labelled Blue and Purple visibly
  wrap to Red and Green, so only those two observed values are catalogued;
- similarly prefixed rows such as `BARRIER` and `BARRIER RESET` are now matched
  by their complete acquisition suffix.

After these corrections, all 311 exposed settings match the authoritative
catalog in label, relative order and accepted values. Remaining default
differences are limited to the reviewed policies below.

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

- Country/Nation uses Export where available, with USA as fallback;
- network-related fields use their safe offline or stand-alone value;
- Daytona USA uses Deluxe cabinet mode;
- Manx TT uses Twin cabinet mode;
- Super GT 24h uses I/O Type C.

The automated checks distinguish these reviewed policies from ordinary native
defaults.

## Verification

`sm2-libretro-save-ram-checks` and `scripts/audit-nvram-core-catalog.py` now verify:

- all 311 catalog entries and their unique keys;
- ordinary Core Option defaults against each native NVRAM template;
- the deliberate policy exceptions listed above;
- all 17 clone-specific catalogs;
- all 31 clone-to-parent catalog mappings;
- all 27 dedicated clone templates and all supported values;
- all 53 diagnostic YAML files against the screenshot-derived visible rows,
  order, declared defaults and acquired values;
- integrity and persistence through the Libretro save container.

The complete check passes after the corrections. The current pass changes one
presentation order plus catalog routing coverage and documentation; it does not
alter option offsets, value encodings or integrity algorithms.
