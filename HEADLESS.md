# Verifica della macchina senza frontend

`sm2-headless` esercita le stesse librerie di emulazione dello standalone, senza
linkare SDL o una libreria grafica. È il primo passo verso il core Libretro;
non implementa ancora l'ABI Libretro.

## Build

Richiede CMake >= 3.24, Ninja e un compilatore C++20. I submodule necessari
alla macchina devono essere inizializzati; SDL, ImGui, VulkanMemoryAllocator,
i backend grafici, curl e il compilatore degli shader non sono utilizzati.

```sh
cmake -S . -B build-headless -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSM2_BUILD_STANDALONE=OFF \
  -DSM2_BUILD_HEADLESS=ON
cmake --build build-headless --parallel 6
```

Su macOS genera un normale eseguibile `build-headless/bin/sm2-headless`,
con `games.xml` accanto. Con `--database` si può indicare un altro database.
`SM2_BUILD_STANDALONE` rimane ON per default: la build originale resta
disponibile. Il default dei test segue la presenza di `tests/CMakeLists.txt`;
questa revisione upstream non distribuisce tale suite.

## Esecuzione

```sh
./build-headless/bin/sm2-headless \
  --game daytona --frames 1800 \
  --output build-headless/run-daytona /percorso/daytona.zip
```

La directory di output deve essere nuova, per evitare che NVRAM preesistente
o vecchie catture influenzino il confronto. Il loader mantiene la gestione
upstream dei parent/clone e degli archivi ZIP/7z.

Il programma chiama `run_frame()`, raccoglie e svuota l'audio della scheda,
compone e renderizza ogni frame con `SoftRenderer`. Produce:

- `software_frame.ppm`: ultimo frame RGB, 496 × 384.
- `audio.wav`: tutti i campioni stereo al sample rate nativo della scheda.
- `nvram/`: EEPROM e memoria persistente dopo l'esecuzione.
- Un riepilogo su stdout con frame, cicli, stato CPU e quantità di audio.

Gli input restano ai valori iniziali della macchina, come nello standalone
`--boot-test` senza input programmati. Il test controlla il funzionamento della
macchina senza OSD; non sostituisce una prova di gioco con controller.

La registrazione audio viene mantenuta in RAM fino alla scrittura del WAV:
usare esecuzioni brevi per le regressioni, non sessioni prolungate.

## Confronto ripetibile con upstream

Lo script usa soltanto la libreria standard Python. La build di riferimento
deve essere lo standalone originale della revisione con cui si vuole confrontare
il port, con un `games.xml` corrispondente. Non usare come riferimento una build
derivata dalle stesse modifiche da verificare.

```sh
python3 scripts/compare-headless.py \
  --headless build-headless/bin/sm2-headless \
  --upstream ../sm2-emu-mainstream/build-macos-release/bin/sm2-emu.app/Contents/MacOS/sm2-emu \
  --rom /percorso/daytona.zip --game daytona --frames 1800 \
  --output build-headless/comparisons/daytona
```

Anche qui la directory deve essere nuova. Lo script esegue lo standalone con
`--boot-test` e il runner con NVRAM separate ma inizialmente vuote. Confronta
immagine finale, formato e contenuto PCM, file NVRAM, numero di frame, cicli e
stato CPU. Un errore di esecuzione o una differenza produce exit nonzero.

`comparison.json` contiene risultati, hash e comandi; i log e gli artefatti
restano nella directory di build, esclusa da Git. Nessuna ROM viene inclusa.
Il test dello standalone esporta anche i dump diagnostici upstream: prevedere
spazio su disco per questi file.

## Risultati del primo punto — 9 settembre 2026

Host: Apple M4, macOS 26.5.1, Apple Clang 21.0.0, Release arm64.
Riferimento: standalone originale `8b3a468c5b51387093811cb16b076e6fd9289d66`,
SHA-256 `75b2ad12cbfde5a87599148094ec496f1a88ece387528057f9c338f2b51b337e`.

| Gioco | Scheda | Frame | Rate audio | Frame stereo | Confronti |
|---|---|---:|---:|---:|---|
| Daytona USA | Model 2 | 1800 | 44642 Hz | 1396928 | Tutti identici |
| Virtua Fighter 2 | 2A | 1800 | 44100 Hz | 1379941 | Tutti identici |
| Virtual On | 2B | 1800 | 44100 Hz | 1379941 | Tutti identici |
| Sega Touring Car | 2C | 1800 | 44100 Hz | 1379941 | Tutti identici |

Per ciascun titolo: immagine finale byte per byte, formato e intero PCM audio,
file NVRAM, stato CPU e cicli identici al riferimento. Tutte le registrazioni
contengono campioni non nulli. Il runner renderizza ogni frame; il confronto
visivo automatico riguarda l'ultimo, non l'intera sequenza. Le immagini finali
Daytona e Touring Car sono state anche ispezionate visivamente.

La traccia della configurazione CMake non contiene ricerche di SDL3, Vulkan,
ImGui, curl, VMA o glslc. Nei 71 file compilati non compaiono OSD o backend
GPU; `otool -L` mostra soltanto libc++ e libSystem per `sm2-headless`, sia
nella build senza frontend sia in quella che include anche lo standalone.

La build combinata standalone + headless è riuscita. Lo standalone modificato
ha eseguito 120 frame di Virtua Fighter 2 con Cocoa/Vulkan su Apple M4,
terminando con exit 0. Il clone mainstream e il suo binario sono rimasti intatti.
Otto casi di input CLI non valido (inclusi conteggi errati, ROM assente e ZIP
invalido) sono stati rifiutati, senza creare output.

Restano gli avvisi upstream sui campi privati inutilizzati e le librerie
statiche duplicate; nella configurazione con trace CMake emette anche avvisi
di policy CMP0156/CMP0181. Non sono errori di compilazione. Non è stata
eseguita la suite upstream `tests/`, assente in questa revisione.

Queste verifiche dimostrano l'equivalenza nelle esecuzioni descritte, non la
compatibilità completa dei giochi, l'audio udibile o gli input fisici. Le build
Linux/Windows, l'ABI Libretro, gli input dal frontend e i save state sono
milestone successive. Per questo punto non è stato necessario cambiare modello.
