# SM2-Emu: contesto e piano

Obiettivo: valutare un core Libretro per RetroArch/Batocera partendo da
SM2-Emu. Primo passo richiesto: checkout distinti e standalone macOS da provare.

## Checkout locali

- Mainstream: `$HOME/dev/sm2-emu-mainstream`, branch `main`.
- Port: `$HOME/dev/sm2-emu-libretro`, repository indipendente, branch `main`.
- Base: upstream `dmanlfc/sm2-emu`, commit
  `8b3a468c5b51387093811cb16b076e6fd9289d66`, versione 0.9.4.
- I due checkout hanno repository Git e submodule indipendenti.
- Il progetto Libretro è un repository autonomo: `Zer0one/sm2-emu-libretro`.
  Conserva la cronologia originale come base tecnica; `upstream` resta
  `dmanlfc/sm2-emu` e `origin` identifica il progetto Libretro.

## Milestone proposte dopo la baseline

1. Completare le prove di gameplay dello standalone: build, avvio e catture
   delle sequenze dimostrative di tre giochi sono già stati verificati.
2. Creare un target senza finestra/SDL per la macchina e il renderer software.
3. Integrare caricamento ROM, un frame per `retro_run()`, video, audio, input
   e persistenza NVRAM. Provare inizialmente un titolo.
4. Estendere i controlli e verificare le diverse revisioni Model 2.
5. Valutare in seguito rendering GPU e serializzazione degli stati.

Non sono ancora presenti un target o un binario Libretro. Questo piano non è
una prova di compatibilità o di prestazioni; evitare semplificazioni della
fedeltà per guadagnare velocità.

## Riscontri preliminari sui sorgenti

- `src/hw/model2_machine_base.h`: init/reset/run_frame, input e NVRAM;
  `compose_video()` è esposto dalla macchina.
- `src/hw/model2_softrender.h`: output RGBA8 a 496 × 384.
- `src/CMakeLists.txt`: `sm2_hw` non linka SDL o `sm2_osd`; la configurazione
  globale include però ancora OSD e dipendenze grafiche. Il target headless
  deve quindi essere dimostrato con una build dedicata.
- `src/hw/sound_board.h`: controllare il sample rate effettivo per scheda;
  non assumere 44100 Hz per tutte le famiglie audio.
- Il renderer software standalone presenta attraverso un backend GPU.
  Il README prescrive Vulkan/MoltenVK su macOS; OpenGL 4.3 non è disponibile.
- Il checkout pubblico non contiene `tests/`, ma CMake abilita i test per
  default. Per questa baseline si passa `SM2_BUILD_TESTS=OFF`; nessun test
  upstream può essere dichiarato eseguito.
- Conservare `LICENSE`, `NOTICE` e gli avvisi originali negli header.
  La base importata mantiene i termini del progetto originale.

## Riprodurre la build

```sh
$HOME/dev/sm2-emu-libretro/scripts/build-upstream-macos.sh
```

Prerequisiti: Apple Command Line Tools, CMake, Ninja, shaderc (`glslc`),
Vulkan headers/loader e MoltenVK. SDL3 e pugixml vengono cercati nel sistema,
con fallback ai submodule upstream. Lo script non installa dipendenze.

Output atteso:
`$HOME/dev/sm2-emu-mainstream/build-macos-release/bin/sm2-emu.app`.
La build locale usa librerie Homebrew: non è un pacchetto autonomo per altri Mac.
I log e il successivo resoconto di verifica sono nella directory di build.

## Riferimento upstream e aggiornamenti futuri

Nel repository Libretro, il remote `upstream` punta a
`https://github.com/dmanlfc/sm2-emu.git`. La cronologia originale è conservata
per attribuzione, confronto e aggiornamenti, senza una relazione di fork su
GitHub. Lo sviluppo avviene su `main`, che non segue automaticamente
`upstream/main`. `origin` punta a `Zer0one/sm2-emu-libretro`.

Per leggere gli aggiornamenti, dal repository Libretro:

```sh
git fetch upstream
git log --oneline HEAD..upstream/main
git diff HEAD...upstream/main
```

Il fetch non modifica i sorgenti locali. Gli aggiornamenti vanno valutati e
integrati su `main` con patch selettive, cherry-pick o merge secondo l'entità
delle modifiche, verificando lo stato locale e preservando il lavoro in corso.
Non eseguire merge o commit automaticamente senza autorizzazione.

Il mainstream è un clone indipendente: dal suo checkout, `git fetch upstream`
e `git merge --ff-only upstream/main` permettono di aggiornare la baseline
quando richiesto. I due repository devono essere aggiornati separatamente.
