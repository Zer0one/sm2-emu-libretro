# Verifica build macOS SM2-Emu — 9 settembre 2026

## Risultato

Compilazione completata con successo, senza modificare i sorgenti upstream.
Versione 0.9.4, commit `8b3a468c5b51387093811cb16b076e6fd9289d66`.
Binario Mach-O arm64 Release; bundle Info.plist valido.

App: `$HOME/dev/sm2-emu-mainstream/build-macos-release/bin/sm2-emu.app`

```sh
open "$HOME/dev/sm2-emu-mainstream/build-macos-release/bin/sm2-emu.app"
```

Senza ROM, l'app mostra l'interfaccia iniziale. Impostare la cartella delle ROM
nelle impostazioni, oppure passare direttamente un archivio:

```sh
"$HOME/dev/sm2-emu-mainstream/build-macos-release/bin/sm2-emu.app/Contents/MacOS/sm2-emu" --graphics-backend vulkan /percorso/gioco.zip
```

Per scegliere il renderer software, usare `--graphics-backend software`.
Anche in questo caso la presentazione della finestra usa MoltenVK.

## Ambiente e configurazione

- Mac Apple M4, macOS 26.5.1; Apple Clang 21.0.0.
- CMake/Ninja, `CMAKE_BUILD_TYPE=Release`, `CMAKE_OSX_ARCHITECTURES=arm64`.
- Vulkan ON; OpenGL desktop/ES OFF; LTO rimasto al default upstream OFF.
- SDL3 3.4.12 e pugixml già presenti nel sistema.
- Dipendenze della build: shaderc 2026.3, vulkan-headers 1.4.357.0,
  vulkan-loader 1.4.357.0, molten-vk 1.4.2.
- `SM2_BUILD_TESTS=OFF`: questo checkout upstream non contiene `tests/`.
  Non è stata eseguita una suite di unit test.

## Verifiche eseguite

- `--help`: exit 0.
- `--list-games`: exit 0, 83 definizioni lette da games.xml nel bundle.
- `--list-gpus`: exit 0, Apple M4 riconosciuta.
- Avvio Cocoa/Vulkan con `--run-frames 120`: exit 0; finestra 992 × 768,
  MoltenVK/Vulkan 1.3.357, swapchain e overlay ImGui inizializzati.
- Cattura interna prodotta. Senza ROM il framebuffer è nero: non costituisce
  una verifica visiva di un gioco e non include l'overlay GUI.
- Secondo avvio della GUI limitato a 3600 frame: exit 0. Il controllo
  visivo tramite Computer Use non ha risposto ed è stato interrotto;
  inizializzazione e chiusura della GUI sono verificate tramite log.
- Successivamente eseguiti Daytona USA (`daytona`), Sega Rally (`srallyc`)
  e Virtua Fighter 2 (`vf2`), 2101 frame ciascuno, con Vulkan/MoltenVK a 2×:
  tutti terminati con exit 0. Catture dei frame 900, 1500 e 2100 ispezionate,
  con scene 3D delle sequenze dimostrative a 992 × 768.
- Audio instradato al driver SDL dummy per le catture; audio udibile e input
  fisici non verificati. Le prove non equivalgono a una sessione di gameplay.
  Nessun gamepad era rilevato durante il test.

Log e comandi precisi: `verification/results.json` e `verification/*.log`.
Log compilazione: `configure.log` e `build.log`.
Tre avvisi non fatali: due campi privati inutilizzati e librerie duplicate
ignorate dal linker. Nessuna patch applicata per questi avvisi.

## Limiti del pacchetto

È una build locale per questo Mac. SDL3 e il loader Vulkan sono collegati alle
librerie sotto `/opt/homebrew`; MoltenVK deve essere disponibile sul sistema.
Non è un bundle autonomo o notarizzato per distribuirlo ad altri Mac.

SHA-256 dell'eseguibile: `75b2ad12cbfde5a87599148094ec496f1a88ece387528057f9c338f2b51b337e`.

## Repository e riproducibilità

Mainstream pulito sul branch `main`; progetto Libretro indipendente su
`main`. Nel progetto Libretro, `origin` punta a
`https://github.com/Zer0one/sm2-emu-libretro.git` e `upstream` al repository
originale. Sono conservati i sorgenti e la cronologia upstream. Il core
Libretro non è ancora implementato.

Per riprodurre la baseline, con i prerequisiti sopra elencati già disponibili,
creare i due clone come cartelle adiacenti:

```sh
git clone --recurse-submodules https://github.com/dmanlfc/sm2-emu.git sm2-emu-mainstream
git clone --recurse-submodules https://github.com/Zer0one/sm2-emu-libretro.git sm2-emu-libretro
./sm2-emu-libretro/scripts/build-upstream-macos.sh
```

Lo script compila il checkout mainstream corrente; per riprodurre esattamente
questa revisione, prima eseguire nel clone mainstream:

```sh
git checkout --detach 8b3a468c5b51387093811cb16b076e6fd9289d66
git submodule update --init --recursive
```
