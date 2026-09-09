# Renderer Vulkan del core Libretro

Implementato e verificato su Apple M4/macOS il 9 settembre 2026, anticipando
il punto 5 della roadmap rispetto all'implementazione dei profili. La build
software resta disponibile per i confronti e per frontend senza Vulkan.

## Funzioni disponibili

- Rendering 2D tramite compute shader e rendering 3D tramite i passaggi e gli
  shader originali SM2-Emu, senza copiarli nell'adattatore.
- Immagine Vulkan consegnata direttamente al frontend: il percorso normale
  non legge i pixel sulla CPU e non usa il renderer software per disegnare il 3D.
- Risoluzione interna 1×, 2×, 3× o 4×: da 496 × 384 a 1984 × 1536. Le tilemap
  mantengono il dettaglio nativo, come nell'upstream; aspect sempre 4:3.
- Core Options v2, categoria Video: `sm2_renderer` (Auto/Vulkan/Software) e
  `sm2_internal_resolution` (1–4). Entrambe si applicano al successivo
  caricamento del contenuto. Per frontend precedenti è presente il menu legacy.
- Auto segue l'API preferita dal frontend: Vulkan se richiesto, software negli
  altri casi. La scelta esplicita Vulkan fallisce se mancano i requisiti, senza
  dichiarare falsamente attivo un percorso GPU.
- Ricreazione delle risorse in `context_reset` e rilascio in `context_destroy`;
  il reset della macchina invalida anche le cache GPU dei contatori generazione.
- In render test mode si conserva la composizione CPU dei framebuffer speciali
  prevista dallo standalone. Questo ramo è presente ma non è stato esercitato
  specificamente nelle prove elencate sotto.

## Proprietà delle risorse e aggiornamenti upstream

RetroArch possiede istanza, dispositivo, code, finestra e swapchain. Il core
negozia le funzioni richieste usando il callback v2 del frontend, alloca solo
le proprie risorse e non crea una finestra SDL o un overlay ImGui.
Tutti gli entry point Vulkan vengono risolti attraverso il frontend: la
libreria del core non collega un secondo loader Vulkan/MoltenVK.

`src/render/vk/pass_context.h` contiene l'interfaccia minima condivisa dai
passaggi 2D/3D. Il contesto standalone la implementa con le risorse originali;
`src/libretro/vulkan_renderer.cpp` la implementa con quelle del frontend.
Le modifiche ai passaggi upstream sono limitate al tipo del contesto e ai suoi
include: algoritmi, shader, formati e regole di composizione restano originali.

I tre slot upstream per comandi e dati temporanei sono protetti da fence del
core. Le immagini consegnate al frontend sono invece associate ai suoi indici
di sincronizzazione, che possono avere una cardinalità diversa. Il core
attende il riutilizzo consentito dal frontend e gestisce i cambi della maschera.
Gli invii alla coda condivisa rispettano lock/unlock; gli stalli dell'intero
dispositivo sono limitati a inizializzazione, ricostruzione e rilascio.

## Requisiti e particolarità macOS

Il renderer upstream richiede Vulkan 1.3 con `dynamicRendering`,
`synchronization2` e `shaderDemoteToHelperInvocation`. La negoziazione controlla
supporto e abilitazione; l'adattatore richiede l'interfaccia Vulkan Libretro v5
con negoziazione v2.

La RetroArch Stable installata, 1.21.0 (`05f94af4`), include MoltenVK 1.2.11,
che nella prova rifiuta la richiesta Vulkan 1.3. Il MoltenVK 1.4.2 già installato
con Homebrew espone invece Vulkan 1.3.357 su Apple M4 e permette l'esecuzione.
Non è stata modificata l'app RetroArch né sostituita la libreria nel bundle.

Il launcher seleziona MoltenVK soltanto per il processo avviato, usando una
directory temporanea. Passare le normali opzioni RetroArch dopo `--`:

```sh
python3 scripts/run-retroarch-vulkan-macos.py \
  --retroarch /percorso/RetroArch.app/Contents/MacOS/RetroArch \
  --moltenvk /opt/homebrew/lib/libMoltenVK.dylib -- \
  -L build-libretro-gpu/libretro/sm2_libretro.dylib /percorso/roms/vf2.zip
```

La directory System configurata in RetroArch deve contenere
`sm2-emu/games.xml`, come nella build software. Il launcher seleziona il driver
Vulkan e disabilita il video threaded per questa esecuzione; nel menu del core
scegliere Auto o Vulkan. Il contesto non viene negoziato su un secondo loader.

Un'altra particolarità verificata nel sorgente RetroArch 1.21 è che il
callback `create_device2` è invocato solo se è presente anche `create_device`.
L'adattatore fornisce il callback legacy come guardia, pur richiedendo v2, e
rifiuta un dispositivo che non provenga dalla negoziazione riuscita. Questo
impedisce di usare silenziosamente il dispositivo predefinito del frontend
senza le funzioni shader richieste.

Fonti API consultate: header `libretro_vulkan.h` con provenienza e hash in
`src/libretro/include/README.md`; sorgenti ufficiali RetroArch
[gfx/common/vulkan_common.c](https://github.com/libretro/RetroArch/blob/05f94af4/gfx/common/vulkan_common.c)
e [dynamic/dylib.c](https://github.com/libretro/RetroArch/blob/05f94af4/libretro-common/dynamic/dylib.c).

## Compilazione

```sh
cmake -S . -B build-libretro-gpu -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DSM2_BUILD_STANDALONE=OFF -DSM2_BUILD_LIBRETRO=ON \
  -DSM2_LIBRETRO_VULKAN=ON -DSM2_BUILD_HEADLESS=ON \
  -DSM2_LIBRETRO_CHECKS=ON -DSM2_BUILD_OPENGL_DESKTOP=OFF \
  -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build-libretro-gpu -j8
```

Il core richiede header Vulkan, glslc e VMA per la compilazione. VMA è già nel
repository. Il frontend di test, attivato da `SM2_LIBRETRO_CHECKS`, collega il
loader Vulkan di sistema; il core distribuito non lo collega. Senza
`SM2_LIBRETRO_VULKAN` restano valide le istruzioni della build solo software.

Artefatto macOS arm64: `build-libretro-gpu/libretro/sm2_libretro.dylib`.
Conserva i 25 export Libretro e dipende solo da libc++ e libSystem.

## Verifiche eseguite

Risultati in `build-libretro-gpu/validation/`, esclusi da Git; riepilogo verificato
in `GPU_REPORT.json`.

| Prova | Risultato |
| --- | --- |
| Daytona / Model 2, VF2 / 2A, Virtual On / 2B, STCC / 2C, 1800 frame GPU ciascuno | Esecuzione e acquisizione native riuscite; intero PCM e hash NVRAM uguali alla baseline software |
| VF2, 1800 frame, confronto core Vulkan / standalone upstream Vulkan a 1× e 4× | Identico pixel per pixel a 496 × 384 e 1984 × 1536 |
| VF2, core Vulkan / software allo stesso frame | 1938 pixel diversi su 190464; errore medio assoluto 0,048084 su canali 0–255. La stessa differenza appartiene al renderer GPU upstream |
| VF2, dispositivo distrutto e ricreato al frame 950 | Immagine finale, PCM e NVRAM identici alla corsa senza ricreazione |
| Indici frontend con maschere 7 → 15 → 7 | Riutilizzo immagini e variazione del numero di slot superati |
| Tre cicli load/reset/unload GPU, entrambi gli ordini di rilascio | Superati |
| Risoluzioni 1×, 2×, 3×, 4× nel frontend GPU di prova | 1800 frame e acquisizione nativa riusciti per ogni risoluzione |
| RetroArch Stable, VF2 con replay, 1× e 4× | Partita visibile, screenshot GPU, audio registrato, uscita regolare e ricreazione del contesto durante l'avvio della registrazione |
| Audio RetroArch GPU / precedente prova software | Prefisso comune identico: 1786258 frame stereo a 1× e 1793924 a 4×; durata della coda diversa fra le acquisizioni |
| Regressione ABI software nella libreria con Vulkan | Video/audio/NVRAM identici; backpressure audio, errori di caricamento, reset e transizioni del sample rate superati |
| Build solo software e standalone Vulkan | Ricompilate; anche lo standalone adattato produce lo stesso frame GPU del pristine upstream |

Per ripetere una prova GPU con immagini native:

```sh
python3 scripts/smoke-vulkan.py \
  --core build-libretro-gpu/libretro/sm2_libretro.dylib \
  --host build-libretro-gpu/libretro-checks/libsm2-vulkan-test-host.dylib \
  --system build-libretro-gpu/libretro/system \
  --rom /percorso/roms/vf2.zip --frames 1800 \
  --output /percorso/nuovo-risultato --exercise
```

Usare `--context-cycle 950 --reference /percorso/risultato-precedente` per
verificare la continuità dopo la ricreazione del dispositivo. Il frontend di
prova serializza il lavoro GPU per le acquisizioni deterministiche: la
presentazione asincrona viene verificata separatamente in RetroArch.
Gli hash NVRAM nel report si riferiscono ai 1800 frame iniziali; con `--exercise`
i salvataggi di lavoro vengono poi usati dai cicli successivi.

Per la prova RetroArch, usare `scripts/smoke-retroarch.py` con gli argomenti
abituali e `--renderer vulkan --scale 4 --moltenvk /opt/homebrew/lib/libMoltenVK.dylib`.
Il test crea configurazione, opzioni, replay, salvataggi e loader temporaneo
nel proprio output; non utilizza i salvataggi personali.

## Limiti del supporto dichiarato

Verificato macOS arm64/Apple M4 con le versioni indicate. Non ancora verificati
Linux/Batocera, altri driver GPU, la compatibilità dell'intero catalogo o una
misura comparativa delle prestazioni. Le prove non usano i Vulkan validation
layers. Il ripristino dopo ricreazione ordinata del contesto è provato;
un guasto fisico o `VK_ERROR_DEVICE_LOST` richiede il riavvio del contenuto.
La build non include ancora un percorso GPU OpenGL/Metal nativo alternativo.
I profili completi, le opzioni Input e i save state restano nei punti successivi.
