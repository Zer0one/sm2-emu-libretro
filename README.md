# SM2-Emu Libretro

Independent development repository for a Libretro adaptation of
[SM2-Emu](https://github.com/dmanlfc/sm2-emu), the Sega Model 2 emulator.
Development takes place on `main`; the original source history is retained
for attribution, comparison and future upstream updates.

**Status: SM2-Emu 0.9.9 Libretro core implemented and tested on macOS arm64.**
The isolated adapter uses the upstream machine and software renderer, with
native timing/audio by default, an optional speed-preserving 60 Hz cadence,
a Libretro timing/FPS overlay, reviewed control profiles and frontend-managed save RAM.
1800-frame video/audio/NVRAM comparisons pass on all four board variants.
The data-driven NVRAM Core Options currently cover 289 reviewed operator
settings across 35 parent sets and fourteen clones with distinct service menus.
Each supported title validates its native
layout and updates the corresponding integrity field and settings mirror.
Experimental linked-cabinet networking uses Libretro Netpacket; Daytona USA
has completed a two-car race. Daytona and STCC have formed three-cabinet
rosters, while Sega Rally parent/B/C, the Indy 500 family, Motor Raid, Wave
Runner and Sega Ski Super G have formed two-cabinet rosters in isolated local
RetroArch instances. Virtual On has formed both a two-Twin roster and a
three-participant roster using the dedicated `vonr` Relay program.
The core also provides Vulkan/OpenGL rendering, upstream 3D/2D enhancement
filters, gamepad rumble, optional Enhanced Audio Balance and separate DSB/MPEG
music volume. Changes from
upstream 0.9.9 relevant to Libretro are integrated selectively; standalone GUI,
packaging and SDL-only changes remain in the adjacent upstream reference.

See [Libretro build and validation](LIBRETRO.md) for the artifact, commands
and limits. Physical controllers, linked play across two hosts and audible
quality need manual checks.
The standalone and [headless validation runner](HEADLESS.md) remain available.

## Get the project

```sh
git clone --recurse-submodules https://github.com/Zer0one/sm2-emu-libretro.git
cd sm2-emu-libretro
git remote add upstream https://github.com/dmanlfc/sm2-emu.git
git fetch upstream
```

`origin` is this independent project. `upstream` is the original emulator;
fetching from it does not modify the working tree. A separate adjacent clone
named `sm2-emu-mainstream` is used for original standalone builds and comparisons.

- [Libretro build and validation](LIBRETRO.md)
- [Roadmap](PORTING_PLAN.md)
- [Libretro menu and control design](LIBRETRO_DESIGN.md)
- [Authoritative screenshot-derived Game Settings catalog](GAME_SETTINGS_CATALOG.md)
- [Frontend-free build and validation](HEADLESS.md)
- [macOS baseline build and validation](MACOS_BUILD.md)
- [Baseline build script](scripts/build-upstream-macos.sh)
- [NVRAM sampling script for gameplay-option analysis](scripts/libretro_nvram_samples.py)
- [RetroArch linked-cabinet test runner](scripts/test-retroarch-netpacket.py)

Original notices and source headers are preserved. See [LICENSE](LICENSE),
[NOTICE](NOTICE) and the source headers for terms and attribution. No ROMs or
prebuilt emulator binaries are included in this repository.

## Utility di campionamento NVRAM

Nel percorso `scripts/` trovi `libretro_nvram_samples.py` con un esempio di
campagna `libretro_nvram_samples.vf2.toml`. Lo script crea una directory di
lavoro isolata per RetroArch, applica una sequenza di input definita e salva il
file frontend `.srm`, più gli estratti `.nv` e `.eeprom` per l'analisi.

```sh
python3 scripts/libretro_nvram_samples.py scripts/libretro_nvram_samples.vf2.toml --dry-run
python3 scripts/libretro_nvram_samples.py scripts/libretro_nvram_samples.vf2.toml --overwrite
```

Il flusso prevede un baseline frontend facoltativo (`standard_srm`) oppure
l'import iniziale dei file nativi (`standard_nvram` / `standard_eeprom`),
supporto a modalità interattiva e salvataggio del report `summary.json`.
La campagna VF2 inclusa genera 30 acquisizioni: un baseline comune e 29 campioni
che coprono ogni valore delle 11 opzioni Game Assignment. Il primo lotto aggiunge le campagne
`skytargt`, `vcop2`, `zeroguna` e `airwlkrs`; i relativi YAML verificati sono in
`data/diagnostic-menus/`. Ogni sequenza completa seleziona `EXIT` nel sottomenu,
esce dal Test Menu e attende il ritorno al gioco, oppure usa il comando esplicito
`Save Setting & Exit` quando previsto dal titolo. `validate_vf2_nvram_samples.py`
controlla la campagna VF2; `validate_nvram_batch1.py` verifica i 102 campioni del
primo lotto. Il secondo lotto comprende `bel`, `daytona`, `desert` e `doa`: 157
campioni descritti nei rispettivi YAML e verificati da `validate_nvram_batch2.py`.
Il terzo lotto comprende `vcop`, `fvipers`, `srallyc` e `hotd`: altri 102
campioni verificati da `validate_nvram_batch3.py`. Il quarto lotto comprende
`gunblade`, `lastbrnx`, `indy500` e `von`: 215 campioni verificati da
`validate_nvram_batch4.py`, dopo avere escluso i valori duplicati che il menu
ciclico non espone realmente. Il quinto lotto comprende `overrev`, `sgt24h`,
`stcc` e `rchase2`: 130 campioni verificati da `validate_nvram_batch5.py`,
inclusi i sottomenu annidati di Over Rev e il parametro Link Max condizionale
di Sega GT 24h. Il sesto lotto comprende `manxtt`, `motoraid`, `segawski`,
`waverunr` e `skisuprg`: 220 campioni verificati da
`validate_nvram_batch6.py`, inclusi i quattro timer 0–20 di Ski Super G e la
sequenza diagnostica di avvio specifica del gioco. Il settimo lotto comprende
`dynabb`, `dynabb97`, `hpyagu98`, `schamp` e `vstriker`: 173 campioni verificati
da `validate_nvram_batch7.py`. I due Dynamite Baseball condividono menu e
struttura EEPROM, mentre Hanguk Pro Yagu 98 condivide il menu ma, nel core
corrente, nello standalone mainstream 0.9.4 e in MAME 0.289, perde le modifiche
al riavvio; MAME riproduce la perdita anche per `pltkids`. Il tracciamento ha
però identificato i layout EEPROM validi e una patch diretta prima del boot ha
ripristinato correttamente `FAVORITE=TIGERS` e `Demo Sound=On`. I relativi YAML
registrano layout, mirror, CRC e offset dimostrati. Sonic Championship e Virtua
Striker includono anche la verifica CRC16-CCITT delle rispettive strutture in
backup RAM. I nuovi YAML classificano
separatamente tipologia di gioco, famiglia dei controlli, produttore dichiarato
e famiglia tecnica osservata del menu/NVRAM; lo sviluppatore resta distinto e
non viene dedotto quando i metadata non lo dichiarano. L’ottavo e ultimo lotto comprende `dynamcop`, `pltkids`, `rascot2`, `topskatr`
e `zerogun`: 130 acquisizioni verificate da `validate_nvram_batch8.py`.
Dynamite Cop espone 33 valori di Life Amount; Pilot Kids usa i direzionali nel
menu e mostra correttamente 8 opzioni, ma nel core corrente non ripristina le
modifiche dopo il riavvio. Per Royal Ascot II è stata acquisita la procedura
speciale mostrata dal gioco: Button 1 avvia il titolo senza SegaNetCom, mentre
Test torna alla diagnostica SegaNet. Nell’emulazione corrente non viene esposto
un normale menu Game Settings, quindi non ci sono righe NVRAM da catalogare.

La campagna è completa per tutte le 36 parent censite: 1.259 acquisizioni, 35
menu mappati e una parent classificata come caso speciale privo di un normale
menu Game Settings nell’emulazione corrente, senza
parent ancora non esaminate. I controlli coprono contenitori, estratti,
schermate, campi, copie speculari, copie incrociate EEPROM/backup RAM, ordine dei
byte EEPROM e gli algoritmi di integrità conosciuti.

## Utility di test Netpacket

`scripts/test-retroarch-netpacket.py` avvia istanze RetroArch isolate per
verificare i roster dei set supportati. Copia la configurazione base del frontend,
applica soltanto override temporanei, assegna i valori NVRAM di ruolo/cabinet,
registra subito ogni processo e produce `manifest.json`, `result.json` e un log
per istanza. Il runner rifiuta di partire se la stessa applicazione RetroArch è
già aperta e attende sempre la chiusura dei soli processi creati dal test. Daytona
e Indy 500 ammettono da 2 a 8 istanze; STCC da 2 a 9 e Sega Rally da 2 a 5,
contando il Relay finale ai rispettivi valori massimi. Motor Raid, Wave Runner,
Sega Ski Super G, Super GT 24h e Over Rev vanno da 2 a 4; Manx TT usa 2 o 3 istanze,
con la terza configurata come Relay. Virtual On usa due istanze Twin o due Twin
più il programma Relay dedicato `vonr`. `daytona93` non è esposto
perché il suo menu ridotto non contiene le impostazioni di collegamento.

```sh
python3 scripts/test-retroarch-netpacket.py \
  --retroarch /percorso/RetroArch.app/Contents/MacOS/RetroArch \
  --core /percorso/sm2_libretro.dylib \
  --rom /percorso/roms/daytona.zip \
  --system-assets /percorso/system/sm2-emu \
  --cabinets 3
```

Per STCC usare `stcc.zip` e aggiungere `--set-name stcc`; il runner assegna
`Car 1`, `Car 2` e così via alle rispettive istanze e, con nove partecipanti,
`Relay` all'ultima. Per Sega Rally fa lo stesso con quattro auto più il Relay.
Con `--include-relay`, STCC e Sega Rally possono usare il Relay anche con un
totale inferiore al massimo. La stessa opzione assegna `Live` all'ultima istanza
di Motor Raid; il totale deve comprendere almeno Master, Slave e Live.
Applica automaticamente
anche i campi specifici di Super GT 24h e Over Rev, compresi i rispettivi cloni
supportati. Per Manx TT assegna `Master`, `Slave` e, con tre istanze, `Relay`;
`manxttdx` resta escluso perché non espone `Link Type`.

Per Virtual On, due istanze usano la stessa ROM Twin. Per aggiungere il live
monitor, selezionare tre cabinet e fornire il programma Relay separato:

```sh
python3 scripts/test-retroarch-netpacket.py \
  --retroarch /percorso/RetroArch.app/Contents/MacOS/RetroArch \
  --core /percorso/sm2_libretro.dylib \
  --rom /percorso/roms/von.zip \
  --relay-rom /percorso/roms/vonr.zip \
  --system-assets /percorso/system/sm2-emu \
  --set-name von --cabinets 3
```

Il test automatico dimostra la formazione del roster e lo scambio Netpacket;
una gara sincronizzata e i controller fisici restano verifiche manuali distinte.

---

## Original upstream README

The following is SM2-Emu's upstream README at the imported revision
`8b3a468c5b51387093811cb16b076e6fd9289d66` (0.9.4). Its descriptions refer to
the standalone emulator, not to an implemented Libretro core.

```
  ____  __  __  ____         _____ __  __ _   _
 / ___||  \/  ||___ \       | ____|  \/  | | | |
 \___ \| |\/| |  __) |_____ |  _| | |\/| | | | |
  ___) | |  | | / __/|_____|| |___| |  | | |_| |
 |____/|_|  |_||_____|      |_____|_|  |_|\___/

 A   S E G A   M O D E L   2   E M U L A T O R
```

Background: I started this emulation journey back in February 2025 to look to
improve upon Model 2 emulation for Linux, since my favourite OS lacked a native
emulator and at the time MAME had incompatibility issues and was just slow for
small ARM-based SBCs. The mission was to look into what MAME did well and learn
more from research and analysis of Supermodel (a Model 3 emulator) as
inspiration. Supermodel actually led me to wire up OpenGL ES and Vulkan for
that emulator, as I could get quicker results on the possibility of running
Model 2 emulation on a Raspberry Pi 5 and bringing it to the emulation
community.

Linux is the primary target. macOS is supported just because that's partly what
I used for development and runs Vulkan through MoltenVK. I don't care for
Windows... there, I said it.

The journey included a lot of discussions with GenAI. I'm not going to lie, but
its ability to really lean in and help tackle the hard parts was somewhat
limited early on. Providing it bite-sized tasks sped up my part-time
development from November 2025 onwards, especially around how all the
components hang together, and later on it was genuinely helpful with the
graphical quirks.

**Status: playable.** All four boards run — original Model 2, 2A, 2B and 2C —
with picture and sound. Of the 83 sets in the database, the majority draw full
3D scenes and produce audio, across three renderers: Vulkan, OpenGL and OpenGL
ES.

All three geometry coprocessors are there — the MB86234 TGP, the ADSP-21062
SHARC and the MB86235 TGPx4 — along with the System 24 tilemap hardware, a
textured renderer with the hardware's own colour chain evaluated per texel, and
both sound boards: the 68000/SCSP the CRX family uses, and the Model 1 audio
board (a 68000 with a YM3438 and two MultiPCMs) that Daytona USA, Desert Tank
and Virtua Cop carry instead. Drive boards, lightguns, the link board and the
protection devices are wired.

Input comes from SDL gamepads with the keyboard live alongside them, and the
machine is paced to its own 57.5245 Hz rather than to the display. The frame is
composited at the hardware's 496x384 and magnified once at the end, so it runs
on the hardware's pixels rather than on colours a filter has already blurred.

## What Model 2 is, and why the renderer looks unusual

Model 2 is an i960KB paired with a geometry coprocessor (a Fujitsu MB86234
"TGP" on Model 2 and 2A, an ADSP-21062 SHARC on 2B, an MB86235 on 2C), a custom
Sega/Lockheed-Martin rasterizer, and Sega System 24 tilemap hardware for the 2D
layers. Output is 496x384 at roughly 57.5 Hz.

Four properties of that rasterizer shape the whole design, because none has a
direct modern equivalent:

- **No depth buffer.** Polygons are bucket-sorted by depth on the CPU and drawn
  front to back against a one-bit fill mask: first writer wins. Reproduced with
  a stencil attachment rather than a depth test.
- **No RGB textures.** A texel is a 4-bit *intensity*; colour arrives through a
  tone curve, a base colour, a translation table and a gamma ramp. The curve is
  applied *after* filtering, so it cannot be baked in — the whole chain runs
  per texel in the fragment shader.
- **No alpha blending.** Translucency is an alpha test on one texel value, or a
  screen-locked stipple. Both discard fragments, which leaves the fill mask
  unclaimed so what is behind still gets the pixel.
- **No per-vertex shading.** One 8-bit luminance scalar per polygon.

So the geometry pipeline runs on the CPU as the hardware's did, and the GPU
backend is handed pre-projected screen-space triangles. No vertex
transformation on the GPU, no geometry shaders.

## Building

Requirements:

- CMake 3.24 or newer, and Ninja
- A C++20 compiler
- `glslc` (from shaderc or the Vulkan SDK) — used at build time to compile and
  lint the shaders
- SDL3, pugixml, miniz, the LZMA SDK, stb_image, Dear ImGui (with its SDL3
  backend) and — for the Vulkan backend — VulkanMemoryAllocator
- libcurl, for the game picker's artwork scraping. Optional: without it the
  picker still lists and launches every game, just with no downloaded art or
  descriptions.
- For the OpenGL / OpenGL ES backends (built by default): the system GL/GLES
  and EGL libraries (Mesa on Linux). No extra headers are needed — SDL3
  provides GL loading.
- For the Vulkan backend (off by default): Vulkan 1.3 headers and loader.

Each dependency is taken from the system when `find_package` locates it, and
otherwise built from the copy bundled under `3rdparty/`. Most of those are git
submodules; the LZMA SDK and stb_image have no git repository of their own, so
their sources are committed directly. A recursive checkout therefore builds
with no network access at configure time — nothing is downloaded on the fly.
Clone accordingly:

```sh
git clone --recurse-submodules https://github.com/dmanlfc/sm2-emu.git
# or, in an existing checkout:
git submodule update --init --recursive
```

The Musashi 68000 core and the ymfm FM library have no system-package form, so
they always come from `3rdparty/`. `glslc` is the one build-time tool that must
be on `PATH` (it runs on the build host, not the target).

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

The default build produces a binary with the software renderer and the OpenGL
4.3 core desktop backend — no Vulkan driver or SDK required, which is what
lower-end ARM boards want. Add Vulkan with `-DSM2_BUILD_VULKAN=ON`.

On **macOS** the default is not enough: Apple's desktop OpenGL tops out at 4.1,
below this renderer's 4.3 floor, so a windowed build needs
`-DSM2_BUILD_VULKAN=ON` (MoltenVK). See the [macOS](#macos) section for a
step-by-step guide.

`Release` (`-O3`) is the default and what you want for running games. For
debugging, use `-DCMAKE_BUILD_TYPE=Debug` (unoptimised, with symbols; also
turns Vulkan validation on by default when the Vulkan backend is built).

Useful options: `-DSM2_ENABLE_VALIDATION=ON` (default in Debug),
`-DSM2_WERROR=ON`, `-DSM2_BUILD_TESTS=OFF`.

### Graphics backends

Up to three renderers are available, chosen at runtime with
`--graphics-backend software|vulkan|opengl`:

- **software** — the CPU rasteriser (also the correctness oracle). Always
  built. It presents through whichever GPU backend was compiled in.
- **vulkan** — the Vulkan backend. Opt-in at build time.
- **opengl** — whichever OpenGL flavour the binary was built with.

Every GPU backend is a build-time choice. Software is always built; the rest
are gated by CMake options so a build only carries what its target needs:

| Option | Default | Backend |
|--------|:-------:|---------|
| `SM2_BUILD_VULKAN`         | OFF | Vulkan 1.3 (needs the Vulkan headers + loader) |
| `SM2_BUILD_OPENGL_DESKTOP` | ON  | OpenGL 4.3 core (desktop x86_64, macOS) |
| `SM2_BUILD_OPENGL_ES`      | OFF | OpenGL ES 3.1 (ARM devices, e.g. Raspberry Pi 5) |

`SM2_BUILD_OPENGL_DESKTOP` and `SM2_BUILD_OPENGL_ES` are mutually exclusive — a
binary carries one GL flavour. Vulkan can be combined with either.

```sh
# Default: software + OpenGL 4.3 core, no Vulkan required
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Desktop with Vulkan as well
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DSM2_BUILD_VULKAN=ON

# ARM / GLES (e.g. Raspberry Pi 5): software + OpenGL ES 3.1
cmake -S . -B build-gles -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DSM2_BUILD_OPENGL_DESKTOP=OFF -DSM2_BUILD_OPENGL_ES=ON
```

The floor is OpenGL 4.3 core / OpenGL ES 3.1 — the renderer uses compute
shaders and storage buffers, which do not exist below that line. On X11 the
GLES backend forces the EGL path automatically (GLX cannot provide a GLES
context); it also works under Wayland.

### Linux dependencies

```sh
# Debian / Ubuntu — default build (software + OpenGL)
sudo apt install cmake ninja-build build-essential \
                 glslc \
                 libgl-dev libgles-dev libegl-dev \
                 libsdl3-dev libpugixml-dev libcurl4-openssl-dev

# Add these only if building the Vulkan backend (-DSM2_BUILD_VULKAN=ON)
sudo apt install libvulkan-dev vulkan-validationlayers \
                 libvulkan-memory-allocator-dev
```

`libcurl` is optional (drop it to build without artwork scraping).
Debian/Ubuntu has no packages for miniz, the LZMA SDK, stb_image or a Dear ImGui
with the SDL3 backend; those come from `3rdparty/` automatically, so nothing
extra is needed as long as the submodules are checked out.

```sh
# Arch / Manjaro — default build (software + OpenGL)
sudo pacman -S --needed base-devel cmake ninja shaderc mesa sdl3 pugixml miniz curl

# Add these only if building the Vulkan backend (-DSM2_BUILD_VULKAN=ON)
sudo pacman -S --needed vulkan-headers vulkan-icd-loader \
                        vulkan-validation-layers vulkan-memory-allocator
```

`mesa` provides the GL, GLES and EGL libraries and `shaderc` provides `glslc`;
`curl` is optional (artwork scraping). Arch has no package for the LZMA SDK,
stb_image or a Dear ImGui with the SDL3 backend; those come from `3rdparty/`
automatically.

### macOS

On macOS you want the **Vulkan backend**, built on top of MoltenVK. This is not
optional in practice: Apple's desktop OpenGL is capped at 4.1, and this
renderer needs OpenGL 4.3 core (for compute shaders and storage buffers). The
default OpenGL backend therefore cannot create a context on macOS, and because
even the software renderer presents its frame through a GPU backend, a
Vulkan-less build can only run headless (`--boot-test`). Build with
`-DSM2_BUILD_VULKAN=ON` and you get a working window, with either the Vulkan or
the software renderer selectable at runtime.

Step by step, from a clean machine:

1. **Install the Xcode command-line tools** (the C++20 compiler and system
   headers):

   ```sh
   xcode-select --install
   ```

2. **Install [Homebrew](https://brew.sh)** if you don't have it, then the build
   tools and dependencies:

   ```sh
   brew install cmake ninja pkg-config \
                shaderc \
                sdl3 pugixml curl \
                vulkan-headers vulkan-loader molten-vk
   ```

   - `shaderc` provides `glslc`, which compiles and lints the shaders at build
     time and is required for every build.
   - `vulkan-headers`, `vulkan-loader` and `molten-vk` are the Vulkan backend's
     dependencies. `molten-vk` is the driver that runs Vulkan on Metal.
   - `curl` is optional — it powers the game picker's artwork scraping. Drop it
     and everything still lists and launches, just without downloaded art.
   - miniz, the LZMA SDK, stb_image and Dear ImGui (with its SDL3 backend) have
     no Homebrew formula, so they are built from `3rdparty/` automatically.

   Alternatively, install the [LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
   instead of the three `vulkan-*` / `molten-vk` formulae. It is not on a
   default search path, so source its environment once **before configuring**
   so `find_package(Vulkan)` can locate the headers and loader:

   ```sh
   . ~/VulkanSDK/setup-env.sh    # or wherever the SDK lives
   ```

   This is a configure-time step only. The build records where the loader was
   found and bakes it into the binary, so the finished executable runs without
   any environment set up first.

3. **Clone with submodules** (see the top of this section):

   ```sh
   git clone --recurse-submodules https://github.com/dmanlfc/sm2-emu.git
   cd sm2-emu
   ```

4. **Configure and build** with Vulkan enabled:

   ```sh
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
       -DSM2_BUILD_VULKAN=ON
   cmake --build build
   ctest --test-dir build
   ```

5. **Run**, selecting a renderer. Both present through MoltenVK:

   ```sh
   ./build/bin/sm2-emu --graphics-backend vulkan   vf2.zip
   ./build/bin/sm2-emu --graphics-backend software vf2.zip
   ```

   Confirm MoltenVK was found with `./build/bin/sm2-emu --list-gpus`; it should
   name your Metal device. Nothing needs to be sourced to run the binary — on
   macOS the build records the loader's path in the executable. If `--list-gpus`
   reports that no Vulkan devices were found, `molten-vk` is missing: install it
   (`brew install molten-vk`) and reconfigure so it is picked up.

### Cross-compilation (Buildroot, Yocto, Batocera, embedded)

sm2-emu builds for `x86_64`, `aarch64` and `riscv64`. Cross-compiling needs no
special dependency handling: pass a toolchain file and build.

```sh
# GLES backend shown, typical for an ARM target:
cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DSM2_BUILD_OPENGL_DESKTOP=OFF -DSM2_BUILD_OPENGL_ES=ON \
    -DSM2_BUILD_TESTS=OFF
cmake --build build
```

`m68kmake` (a small C program that generates the 68000 opcode table) has to run
on the build host, not the target. CMake handles this automatically: it is
configured and built as a separate host-toolchain sub-project during the build,
so there is no manual pre-build step and no flag to pass, native or cross.

`CMAKE_BUILD_TYPE` and its optimisation flags (`-O3` for `Release`) are a
CMake-level setting, so they apply to the target compiler the toolchain file
selects — a cross build gets the same optimisation as a native one, for its
own architecture. There is no `-march=native` anywhere, so a build stays
portable across the boards it targets.

The target sysroot may provide any of the dependencies below; each one CMake
does not find there is built from the copy under `3rdparty/` instead, so a
recursive checkout cross-compiles with no network access:

- SDL3, pugixml, miniz, the LZMA SDK, stb_image and Dear ImGui
- libcurl, if artwork scraping is wanted (optional; omit for an offline picker)
- the GL/GLES and EGL libraries, if a GL backend is built (the usual case)
- Vulkan 1.3 headers (`vulkan/vulkan.h`), loader (`libvulkan.so`) and
  VulkanMemoryAllocator, only if `-DSM2_BUILD_VULKAN=ON`
- `glslc` on the host PATH (it runs at build time, not on the target)

The Musashi 68000 core and the ymfm FM library always come from `3rdparty/`,
having no system-package form.

## Running

```sh
./build/bin/sm2-emu --list-games
./build/bin/sm2-emu --list-gpus
./build/bin/sm2-emu [--graphics-backend <software|vulkan|opengl>] \
                    [--render-scale <1-4>] \
                    [--fullscreen] [--no-vsync] [--game <set>] vf2.zip
```

No ROM data is distributed with this software. Games are identified by the
CRC32 of their contents rather than by filename, so a merged archive holding
several revisions resolves correctly and `--game <set>` picks one out of it —
the four Virtua Fighter 2 revisions share a single file, as do the Virtua Cop
and Sega Rally families. A clone declares only the chips it respins and
inherits the rest from its parent, including out of the parent's archive if it
has none of its own.

```sh
./build/bin/sm2-emu --game vf2o vf2.zip
```

ROM layouts live in `data/games.xml`, so adding a game is a data edit. A region
is a flat byte array and each chip contributes `chunk` bytes every `stride`
bytes, which expresses every interleaving the hardware uses. The schema is
documented at the top of that file.

## Controls

Gamepads are read through SDL's gamepad layer, so anything with a mapping works
without configuration. The first pad to connect is player 1, pads can come and
go while the game runs, and `--list-gamepads` shows what was recognised. Face
buttons are read by position rather than by label. Driving games take a wheel
and pedals from the pad's stick and triggers; the gun games take aim from the
mouse.

| Gamepad | Function |
|---------|----------|
| D-pad or left stick | Stick |
| A B X Y | Buttons 1 to 4 |
| Left / right shoulder | Buttons 3 and 4 again |
| Start | Start |
| Back | Insert a coin |

The keyboard is live at the same time, so a second player can join on it and
the operator controls stay reachable without a pad:

| Keys | Function |
|------|----------|
| `5` `6` | Coin 1, coin 2 |
| `1` `2` | Start 1, start 2 |
| `9` `0` | Service, test |
| Arrows, `Z` `X` `C` `V` | Player 1 stick and buttons |
| `W` `A` `S` `D`, `G` `H` `J` `K` | Player 2 stick and buttons |
| `Escape` | Quit |
| `P` | Pause |
| `F1` | Toggle the settings menu |
| `F2` | Toggle fullscreen |
| `F12` | Save a screenshot |
| `Tab` (held) | Fast-forward |

The renderer (GPU or software) is chosen at launch with `--graphics-backend`
and cannot be switched at runtime.

## Settings

`--write-config` creates a `sm2-emu.ini` with every setting at its default and
a comment explaining each, which is the quickest way to see what can be set. It
is looked for in the working directory first and otherwise in the platform's
config directory (`$XDG_CONFIG_HOME/sm2-emu` on Linux, `~/Library/Application
Support/sm2-emu` on macOS); `--config <path>` overrides both, and whichever
file was used is named in the log. A command-line flag always beats the file,
and an unparseable line is reported and skipped rather than refused, so a file
from a later version cannot stop an earlier binary from starting.

## Frame pacing

The machine runs at 57.5245 Hz — 434600 cycles of a 25 MHz clock — which
divides into no monitor's refresh rate. Presenting one emulated frame per
display refresh would run the game four percent fast at 60 Hz, so it is paced
against real time instead and every emulated frame is presented exactly once:
nothing duplicated, nothing dropped, no input lost. On a 60 Hz display a frame
is occasionally held for two refreshes, which is unavoidable at this rate
without inventing frames.

Vsync and pacing compose rather than conflict — whichever wants the longer
frame wins. On a display slower than 57.5 Hz vsync would win and the game would
run slow, which is what `--no-vsync` is for. `--no-throttle`, or holding `Tab`,
runs as fast as the machine manages.

## Roadmap

| Phase | Milestone |
|:-----:|-----------|
| 0 | Window, Vulkan 1.3 device, swapchain, shader pipeline **(done)** |
| 1 | ROM loader, i960KB core, Model 2A memory map, timers and interrupts **(done)** |
| 2 | System 24 tilemaps — the first real picture **(done)** |
| 3 | TGP coprocessor, geometry engine, flat-shaded 3D **(done)** |
| 4 | Textures, the colour chain, translucency **(done)** |
| 5 | Gamepad input, configuration, frame pacing, 68000 + SCSP sound **(done)** |
| 6 | Presentation **(done)**, tilemap edge cases, accuracy, more games |
| 7 | Expand compatibility to load and run more games; Model 1 audio board **(done)** |
| 8 | Accelerate performance with Vulkan, offloading to the GPU **(done)** |
| 9 | OpenGL 4.3 desktop and OpenGL ES 3.1 backends **(done)** |
| 10 | Tidy everything up for a release with an associated GUI and options |

## Known gaps

- **The geometry engine is a high-level model, not an emulation.** Its
  microcode has never been dumped; what is emulated, following MAME, is what
  that microcode does, reconstructed from the equivalent program later boards
  upload. Results should match, timing does not.
- **The tilemap sky repeats visibly** on some sets. The name table really does
  repeat characters where the scenery is distant, so this is either a
  perspective stretch working as intended or something upstream of the tile
  chip. Unresolved.
- **Neither sound board has been diffed sample for sample.** The Model 1 board
  agrees with MAME's reference output on onset and roughly on level, but that
  is an envelope, not a waveform. The SCSP, which MAME itself marks imperfect,
  has not been checked to that depth.

## Future work

Roughly in order of increasing difficulty.

### Games with known issues

Everything with a local ROM archive runs. The exceptions:

- A handful of sets produce nothing here *and nothing in MAME*, because they
  are marked not-working upstream: Manx TT (both DX sets), Motor Raid DX,
  Virtual-On Relay and Royal Ascot II. There is no reference to work against
  for these. Sega Ski Super G still lacks the external Drive Board response,
  but the optional Libretro `Drive Board Error Bypass` reproduces the verified
  Test press at error `FF` and allows the game to proceed without claiming that
  the board is emulated.

Separately, the Manx TT Deluxe cabinet carries a Model 1 audio board *on top
of* the 68000/SCSP board every Model 2A has. The audio board itself works, but
the machine has no slot for a second board yet, so those ROMs load unread.

### Internal resolution scaling

The 3D can be rendered above the native 496×384. `--render-scale <1-4>` (also a
setting, and in the Video tab of the overlay) rasterises the 3D pass at N times
native — 2× is 992×768, 4× is 1984×1536 — for crisper polygon edges and
textures. The 2D tilemap and HUD are fixed ROM bitmaps that cannot gain detail,
so they are upscaled nearest-neighbour and stay pixel-sharp; the whole frame is
composited at the scaled resolution and fitted to the window at the end. GPU
backends only (the software renderer stays native); 1× is the default and is
byte-identical to the pre-feature output. Stipple transparency stays locked to
the native grid, so translucent surfaces keep their hardware look at any scale.
The extra cost is GPU fill-rate only — the emulated machine runs identically at
every scale. Fancier 2D upscaling filters (bilinear, xBRZ, and similar) are a
possible later addition; only nearest-neighbour is offered today.

### Input and peripherals

Gamepad axes already drive the analog channels the driving games read, and the
mouse already drives the lightgun channels. What is missing:

- **Dedicated wheel and pedal devices**, with their own axis layout rather than
  being mapped from a pad's stick and triggers.
- **USB lightguns**, as opposed to the mouse.
- **Force feedback and rumble.** The drive board's commands are already on the
  serial link; translating them to SDL haptic events is not done.

### GUI and usability

- **Expand the settings GUI** with input binding, per-game overrides, a ROM
  path browser and volume control.
- **Game launcher.** A ROM directory scanner, so the command line is optional.
- **Save states**, with multiple slots. Arcade games have no native save.

### Additional features

- **Netplay.** The fixed-rate frame clock and deterministic emulation make
  rollback feasible.
- **Shader post-processing.** User-loadable GLSL/SPIR-V for CRT simulation,
  scanlines and colour grading, after the native frame is composed.
- **Run-ahead**, **rewind** and **input recording** — all natural extensions
  once save states exist.

## Licence and credits

BSD 3-Clause. See `LICENSE`.

I'm standing on the shoulders of giants, and SM2-Emu exists because of the MAME
project's reverse engineering of this hardware. The emulation is derived from
MAME's Sega Model 2 driver and its device cores, which their authors released
under the same licence. See `NOTICE` for per-component attribution.

## Core Libretro: renderer Vulkan e OpenGL

Il core offre Vulkan 1.3 e OpenGL 4.3/OpenGL ES 3.1 con passaggi upstream
condivisi, risoluzione 1×–4× e Core Options Video. Vulkan è verificato in
RetroArch macOS/Batocera; OpenGL è verificato con EGL/Mesa e resta da provare
su un frontend e driver hardware. Build, avvio e prove: [GPU.md](GPU.md).

Build multipiattaforma e pacchetti GitHub Actions: [CI.md](CI.md).
