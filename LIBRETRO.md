# Primo core Libretro software

Milestone 2 completata il 9 settembre 2026 nei limiti delle prove sotto.
Adattatore in `src/libretro/`, indipendente da SDL, ImGui e API grafiche.
Riutilizza loader, macchina, scheda audio e renderer software upstream.
Nessun cambiamento ulteriore a CPU o scheduling rispetto alla milestone 1.

Aggiornamento: è disponibile anche la build con renderer Vulkan e Core Options
Video, documentata in [GPU.md](GPU.md). Le funzioni e le prove descritte qui
riguardano il percorso software originario.

## Build macOS arm64

Dalla radice del repository, con le dipendenze della build headless disponibili:

```sh
cmake -S . -B build-libretro -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DSM2_BUILD_STANDALONE=OFF -DSM2_BUILD_HEADLESS=ON \
  -DSM2_BUILD_LIBRETRO=ON -DSM2_LIBRETRO_CHECKS=ON
cmake --build build-libretro -j8
build-libretro/bin/sm2-libretro-input-checks
```

Artefatti:

- `build-libretro/libretro/sm2_libretro.dylib`
- `build-libretro/libretro/system/sm2-emu/games.xml`

Il core macOS espone i 25 simboli dell'ABI Libretro e dipende soltanto da
libc++ e libSystem. Nessuna dipendenza dal checkout Supermodel. La provenienza
dell'header API è in `src/libretro/include/README.md`.

Per una prova manuale, caricare la libreria in RetroArch e collocare il database
in `<directory system>/sm2-emu/games.xml`. Il frontend deve fornire anche una
directory save. Le ROM restano esterne: il core accetta i percorsi ZIP/7z e
chiede al frontend di non estrarre gli archivi. Se il loader richiede parent
o firmware separati, valgono le regole del database upstream.

## Funzioni disponibili

- Init/deinit, load/unload, reset e un frame emulato per `retro_run()`.
- Frame 496 × 384, aspect 4:3, pixel XRGB8888: conversione esplicita dalla
  disposizione dei canali upstream, pitch 1984 byte, nessuna inversione verticale.
- Timing derivato dalle costanti delle quattro schede: 57.524160147 Hz alla
  revisione attuale. Il frontend gestisce sincronizzazione e presentazione.
- Audio stereo a 44100 Hz oppure 44642 Hz secondo la scheda; mantenimento dei
  campioni parzialmente accettati, fallback al callback per singolo campione,
  pulizia della coda al reset/unload. Una coda bloccata oltre un secondo causa
  un errore esplicito e richiesta di chiusura, evitando crescita illimitata.
- Persistenza nativa `.nv`/`.eeprom` sotto `<save>/sm2-emu/<set>/`, tramite le
  funzioni upstream. RetroArch può aggiungere il nome del core alla directory
  save che comunica. L'esposizione SRAM e i campi NVRAM restano al punto 3.
- Errori di caricamento segnalati al frontend; dettagli del loader nel log
  stderr. Nessun percorso implicito nella directory corrente.

I profili digitali semplici usano il D-pad e i pulsanti per posizione del pad:

| RetroPad | Azione |
| --- | --- |
| Select / Start | Moneta / Start, indipendenti per P1/P2 |
| South / East / West | Pulsanti originali 1 / 2 / 3 |
| LB | Alias aggiuntivo del pulsante 3 |
| North / RB | Pulsante 4 e alias, soltanto nei profili che non sono a tre pulsanti |
| L3 / R3, porta 1 | Test / Service |

Il bit Start specifico del gioco viene rispettato. Le azioni digitali di gioco
sono abilitate solo nei profili semplici riconosciuti: guida, pistole,
analogici e Virtual On ricevono per ora soltanto Coin/Start/Test/Service con
un avviso. Le posizioni analogiche di riposo restano quelle della macchina.
I descrittori vengono aggiornati al caricamento e rimossi all'unload.

## Prove ripetibili

Controlli ABI e confronto con un risultato headless per lo stesso ROM set,
numero di frame e NVRAM inizialmente vuota:

```sh
python3 scripts/smoke-libretro.py \
  --core build-libretro/libretro/sm2_libretro.dylib \
  --system build-libretro/libretro/system \
  --rom /percorso/roms/vf2.zip --frames 1800 \
  --reference build-headless/comparisons/vf2/headless \
  --output build-libretro/validation/vf2-new \
  --backpressure --exercise --switch-rom /percorso/roms/daytona.zip
```

Le directory di output devono essere nuove. Il riferimento si genera con
`scripts/compare-headless.py`, descritto in [HEADLESS.md](HEADLESS.md).

Test runtime specifico per **RetroArch Stable 1.21.0, commit 05f94af4, macOS**:

```sh
python3 scripts/smoke-retroarch.py \
  --retroarch /percorso/RetroArch.app/Contents/MacOS/RetroArch \
  --core build-libretro/libretro/sm2_libretro.dylib \
  --system build-libretro/libretro/system \
  --rom /percorso/roms/vf2.zip \
  --output build-libretro/validation/retroarch-vf2-new
```

Lo script crea configurazione, salvataggi, replay e registrazioni dedicati.
Il replay v1 fornisce anche gli stati dei pulsanti rilasciati, richiesti dal
[lettore ufficiale di questa versione](https://github.com/libretro/RetroArch/blob/05f94af4/input/input_driver.c).
Non include save state e non implica supporto alla serializzazione. Il formato
non è dichiarato compatibile con tutte le versioni successive di RetroArch.
La configurazione di prova usa CoreAudio e `audio_max_timing_skew = 0.0` per
mantenere il rate nativo, lasciando la sincronizzazione al frontend.

## Risultati verificati

Su Apple M4/macOS, build Release arm64:

| Titolo | Scheda | Frame del confronto | Video finale / tutto il PCM / NVRAM |
| --- | --- | ---: | --- |
| Daytona USA | Model 2 | 1800 | Identici alla baseline |
| Virtua Fighter 2 | Model 2A | 1800 | Identici alla baseline |
| Virtual On | Model 2B | 1800 | Identici alla baseline |
| Sega Touring Car | Model 2C | 1800 | Identici alla baseline |

Risultati locali in `build-libretro/validation/`, esclusi da Git. Per VF2 sono
passati anche rifiuto del pixel format, directory mancanti, archivio invalido,
caricamento fallito seguito da caricamento valido, tre cicli load/reset/unload,
accettazione audio parziale e passaggio VF2 → Daytona → VF2 nella stessa libreria
con callback audio per singolo campione. I controlli senza ROM verificano
separazione P1/P2, alias, rilascio, disconnessione, Start specifico e filtro dei
profili non supportati. La build combinata standalone/headless/Libretro passa.

RetroArch Stable ha avviato VF2 con CoreAudio e presentazione GL, eseguito il
replay di monete/start/azioni e prodotto una cattura di una partita in corso e
un WAV stereo non silenzioso. La prova termina con exit code 0. Screenshot,
WAV, comando e log sono conservati nel risultato del test runtime. I cicli
ripetuti e il confronto PCM sono provati dal client ABI; il runtime RetroArch
verifica separatamente l'integrazione nel frontend reale.

## Limiti

Nel percorso software: nessun profilo completo volante/lightgun/twin-stick,
SRAM esposta al frontend, cheat o save state. Renderer Vulkan e Core Options
Video sono disponibili nella build descritta in [GPU.md](GPU.md). La geometria
nativa software è fissa. Rewind, run-ahead e netplay non sono dichiarati supportati.
Le etichette delle azioni specifiche dei giochi e le varianti dei dispositivi
sono da completare nella milestone 3, seguendo [LIBRETRO_DESIGN.md](LIBRETRO_DESIGN.md).

La prova di gameplay usa input sintetici attraverso RetroArch: non convalida
un controller fisico. CoreAudio e PCM registrato confermano il percorso audio;
qualità percepita e ascolto manuale restano da verificare. Non è una matrice di
compatibilità completa né una misura delle prestazioni su altre piattaforme.

La Nightly installata 1.22.2 ha prodotto un'immagine, ma ha mostrato problemi
nel ciclo di avvio da CLI; la validazione conclusiva usa la Stable indicata.
Un primo replay incompleto sulla Stable terminava subito e faceva fallire la
cattura finale del frontend: il test consegnato include tutti gli stati input.
