# Build automatiche Libretro

Il workflow `.github/workflows/libretro-ci.yml` compila da `main`, nelle pull
request e su avvio manuale cinque core: Linux x86_64, Linux arm64, Windows
x86_64, macOS Apple Silicon e macOS Intel. I renderer Software, Vulkan e
OpenGL sono inclusi. Non servono ROM nei runner e nessuna ROM viene distribuita.

Ogni job esegue i controlli ROM-free di input, SAVE_RAM/Core Options,
Netpacket e rumble, quindi verifica caricamento della libreria, 25 funzioni ABI,
dipendenze e tre cicli init/deinit senza contenuto. Questi controlli non
sostituiscono una prova con giochi e GPU reali.

Gli artifact privati restano disponibili per 14 giorni. Contengono il core,
`sm2_libretro.info`, `system/sm2-emu/games.xml`, licenze, revisione sorgente e
SHA256SUMS. Copiare il core nella directory core del frontend, il file `.info`
nella directory informazioni e `sm2-emu/games.xml` nella directory system.
Per una prova usare prima directory separate per configurazione e salvataggi.
La CI controlla dopo il confezionamento che la struttura sia completa, che i
checksum corrispondano, che non siano presenti ROM e che le licenze delle
dipendenze incorporate siano incluse in `licenses/`.

Il controllo di distribuzione locale ha inoltre rilevato una possibile
contraddizione da chiarire prima di una pubblicazione esterna: il file radice
`LICENSE` dichiara BSD-3-Clause, mentre 114 file conservano nell'intestazione
upstream una limitazione all'uso commerciale o monetario. Il port mantiene
intatte entrambe le dichiarazioni finché upstream non ne chiarisce la relazione.

Entrambe le architetture Linux usano runner nativi Ubuntu 24.04 (glibc 2.39) e
incorporano i runtime GCC/C++: la compatibilità con distribuzioni precedenti
non è garantita. Il job arm64 verifica inoltre `uname -m = aarch64` prima della
build. Linux x86_64 abilita inoltre LTO: la prova nativa descritta sotto ha
mostrato che è necessario per ottimizzare il percorso emulativo attraverso le
librerie statiche del progetto. Windows usa MinGW64 con runtime statici. macOS
richiede almeno macOS 13.
La GPU richiede Vulkan 1.3 con le feature verificate dal core; su macOS vedere
`GPU.md` nel repository per la selezione di una versione compatibile di MoltenVK.

## Verifica prestazioni Batocera del 25 settembre 2026

Su Batocera 43.1 e Ryzen 5 3400G, due build Linux x86_64 dello stesso sorgente
0.9.9.0, entrambe GCC 15.2 e con configurazione identica salvo LTO, hanno
eseguito la stessa sequenza deterministica di Indy 500 in Vulkan 1x. Dopo il
warm-up, la build senza LTO ha richiesto 15,21 ms mediani per `machine` e ha
offerto 62,6 FPS mediani di capacità emulativa. Con LTO i valori sono passati a
10,44 ms e 90,6 FPS: 31,4% in meno nel percorso emulativo, con correlazione
temporale 0,99 tra i campioni. Entrambe le esecuzioni hanno mantenuto circa 60
FPS reali e sono terminate con codice 0.

Una prova precedente GCC 13 contro GCC 15, entrambe senza LTO, aveva mostrato
un costo normalizzato per clock sostanzialmente invariato; l'aggiornamento del
solo compilatore non risolve il limite. La build LTO porta il tempo `machine`
Linux vicino al campione Windows equivalente e costituisce quindi la
configurazione predefinita del solo artifact Linux x86_64.

I profili completi dei controlli e i Save State Libretro sono implementati.
Le build CI verificano compilazione e ABI; la matrice Save State con ROM reali è
documentata in `LIBRETRO.md` e non viene eseguita sui runner privi di ROM.

## Verifica del 9 settembre 2026

CI verde su tutte e quattro le piattaforme, revisione core `f14d97a`:
[esecuzione 34388054838](https://github.com/Zer0one/sm2-emu-libretro/actions/runs/34388054838).
Il pacchetto Linux scaricato da questa esecuzione ha SHA-256 del core
`7e9007bef9a68972ad14db6e40e2351c9622069ea038f33c601c36d81a450898`.
Checksum del pacchetto ricontrollati sul dispositivo, nessuna dipendenza ELF
mancante: soltanto libc, libm e il loader di sistema.

Test reale: Batocera 43.1 x86_64, glibc 2.40, RetroArch 1.22.2, Radeon Vega 11
(RADV RAVEN), Mesa 25.3.6, Vulkan 1.4.328. Virtua Fighter 2, con ROM già presente
sul dispositivo, replay di monete/Start/azioni e salvataggi inizialmente vuoti.

| Renderer | Risoluzione interna | Frame | Tempo totale | Uscita |
|---|---|---:|---:|---:|
| Software | 496×384 | 2300 | 40,90 s | 0 |
| Vulkan 1× | 496×384 | 2300 | 40,90 s | 0 |
| Vulkan 2× | 992×768 | 2300 | 41,14 s | 0 |

Screenshot di gioco verificati, audio stereo 44.100 Hz registrato. Tutte le
esecuzioni producono esattamente 1.763.259 campioni stereo: PCM identico
byte per byte (SHA-256 `77750549589f83efe198849477c787ab684402403d0bd1d4c9dafff89ef8848c`),
così come NVRAM ed EEPROM. I tempi comprendono avvio, cattura e chiusura;
non costituiscono un benchmark completo. Test e file risiedono soltanto nella
directory dedicata `/userdata/system/sm2-libretro-tests/f14d97a`.

La prima prova ha rilevato un crash alla chiusura Vulkan: RetroArch conserva
`destroy_device` fino a dopo lo scaricamento della libreria core. Il core
rilascia già tutte le proprie risorse in `context_destroy`, quindi la callback
opzionale di distruzione del dispositivo è ora nulla. Il dispositivo appartiene
al frontend. La correzione è nel core CI sopra; i tre test finali terminano
regolarmente. Anche il test GPU locale con ricreazione del dispositivo al frame
950, tre cicli load/reset/unload e confronto a 1800 frame resta identico per
video, PCM e NVRAM.

Comando riproducibile dalla directory di test, usando un output nuovo:

```sh
DISPLAY=:0 XDG_RUNTIME_DIR=/var/run python3 scripts/smoke-retroarch.py \
  --retroarch /usr/bin/retroarch --core ./sm2_libretro.so --system ./system \
  --rom /userdata/roms/model2/vf2.zip --output ./nuova-prova \
  --renderer vulkan --scale 2 --replay-reader 1.22 --audio-driver pulse
```

Lo script contiene due adattamenti del solo test a RetroArch 1.22.2:

- Il [lettore replay](https://github.com/libretro/RetroArch/blob/v1.22.2/input/bsv/bsvmovie.c)
  legge 40 byte di intestazione anche per un replay v1 senza savestate;
  `--replay-reader 1.22` aggiunge i 16 byte attesi da questo lettore.
- Il [registratore WAV](https://github.com/libretro/RetroArch/blob/v1.22.2/record/drivers/record_wav.c)
  scrive una struttura C con tag di 5 byte e padding. Lo script riconosce e
  verifica esattamente questa intestazione e la lunghezza finale, conserva
  `vf2.wav` originale e scrive `vf2-normalized.wav` con gli stessi campioni PCM.

Le evidenze locali sono in `build-ci-artifacts/34388054838/batocera/REPORT.json`
e nelle tre sottodirectory (log, screenshot, WAV e salvataggi), escluse da Git.
Il core Apple Silicon del primo artifact CI era stato anche confrontato con
la baseline headless a 1800 frame (video/audio/NVRAM identici) ed eseguito con
Vulkan in RetroArch macOS. La prova Windows con gioco e GPU reale della revisione
`1d8a1ab` è documentata in `GPU.md`; macOS Intel conserva soltanto i controlli
di build/ABI. Una precedente build Linux
arm64 ha già completato 2300 frame di VF2 in RetroArch 1.18 con OpenGL ES e
Save RAM; il nuovo job nativo deve ancora essere eseguito dopo commit/push e il
suo artefatto resta da provare su hardware reale. Restano da verificare altri
giochi, controller fisici e altre combinazioni di frontend/driver.

## Verifica Windows del 20 settembre 2026

L'artifact `windows-x86_64` dell'esecuzione
[35509380925](https://github.com/Zer0one/sm2-emu-libretro/actions/runs/35509380925),
corrispondente esattamente alla revisione `1d8a1ab6b69d0393ce4247100056e853dacb85ae`,
è stato verificato su Windows con RetroArch 1.22.2 e Radeon Vega 11, driver AMD
`31.0.21925.1001`. I checksum del pacchetto corrispondono; il core ha SHA-256
`a37e87f26663827006cb00dd5912d77c5ba2d0c5f1a737e03dfded8f727ee66a`.

VF2 ha completato 2300 frame in Vulkan 1×/4× e OpenGL 1×/4×; una quinta
esecuzione Vulkan 1× dopo i cambi di backend ha verificato il riavvio. Tutti i
processi sono usciti con codice 0 senza residui. Vulkan 1.3.260 e OpenGL Core
4.6 hanno prodotto gameplay visibile e identico byte per byte a parità di
scala, WAV RIFF non silenziosi identici e SRAM valide da 16.576 byte. Il test è
isolato dall'installazione globale e non dimostra controller fisici o ascolto
soggettivo. Dettagli e limiti sono in `GPU.md`; i risultati grezzi locali sono
esclusi da Git sotto
`build-windows-x86_64/validation/gpu-matrix-1d8a1ab-20260920/`.

## Verifica Batocera del worktree corrente del 20 settembre 2026

Il worktree corrente, basato su `1d8a1ab` e comprendente la coda Libretro dei
Save State, è stato compilato localmente per Linux x86_64 con la stessa
configurazione Software/Vulkan/OpenGL della CI. I controlli ROM-free e
l'ispezione ABI sono verdi; il core, SHA-256
`895620d76a683bc1689fe803b244dedb854a1eb4d485f31b3f2ed9c5b31f9555`,
dipende soltanto da libc, libm e dal loader x86_64.

In una directory isolata di Batocera 43.1, Daytona ha completato 2300 frame in
RetroArch 1.22.2 con OpenGL Core 4.6, risoluzione interna 496x384, gara visibile,
audio stereo non silenzioso, SRAM da 16.576 byte e uscita 0. Sul medesimo core,
il test ABI Linux ha superato 20 cicli Save State nei pattern run-ahead e rewind,
round-trip e replay deterministici e rifiuto transazionale degli stati corrotti.
Le evidenze locali, escluse da Git, sono in
`build-linux-x86_64/batocera-current-validation-results/`; la directory remota
marcata è stata conservata per revisione.

## Verifica delle Core Options del 12 settembre 2026

La [CI della revisione `04a2132`](https://github.com/Zer0one/sm2-emu-libretro/actions/runs/34670297828)
è verde per Linux x86_64, Windows x86_64, macOS Apple Silicon e macOS Intel.
Tutti e quattro i job hanno compilato il core, eseguito i controlli input,
SAVE_RAM e ABI e pubblicato il proprio artifact. Il pacchetto Linux riporta la
revisione completa `04a21325caa0cfb1344e3a33a9d8bfbddc1fd709`; tutti i
checksum inclusi sono validi e il core ha SHA-256
`8e35bce806149a61d8449eb4260fc7c9839fe68c304a88d8d5411dbcd7b888df`.
Sul dispositivo dipende soltanto da `libm`, `libc` e dal loader x86_64.

Lo stesso artifact è stato copiato nella directory isolata
`/userdata/system/sm2-libretro-tests/04a2132` del Batocera 43.1 già descritto.
VF2 è stato eseguito per 2300 frame con Vulkan 2x, replay di gameplay e NVRAM
inizialmente assente. Il core ha negoziato Vulkan 1.3 sulla Radeon Vega 11,
renderizzato a 992x768 e applicato simultaneamente `Country=USA`, `Drink=OK`,
`Difficulty=Hardest` e `Display Type=C.R.T.`. Il test è terminato con codice 0
in 41,65 secondi, ha salvato una `.srm` valida da 16.576 byte e prodotto
1.762.492 campioni stereo non silenziosi a 44.100 Hz. Lo screenshot finale di
gameplay ha SHA-256
`ab7fdcbc72f0be46c96349ecf9f1c5adb50d7884e2ea02a3628fab7cea864b05`.
