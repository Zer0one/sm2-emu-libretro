# Primo core Libretro software

Milestone 2 completata il 9 settembre 2026 nei limiti delle prove sotto.
Adattatore in `src/libretro/`, indipendente da SDL, ImGui e API grafiche.
Riutilizza loader, macchina, scheda audio e renderer software upstream.
Nessun cambiamento ulteriore a CPU o scheduling rispetto alla milestone 1.

Aggiornamento: è disponibile anche la build con renderer Vulkan/OpenGL e Core Options
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
build-libretro/bin/sm2-libretro-save-ram-checks
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
- `RETRO_MEMORY_SAVE_RAM` gestita dal frontend in un contenitore versionato di
  16.576 byte: header, 16 KiB di backup RAM e 128 byte di EEPROM. Il nome del
  set e il checksum del payload impediscono l'import silenzioso di file errati.
  Un `.srm` valido prevale; in sua assenza i file nativi `.nv`/`.eeprom` sono
  importati senza essere riscritti dal core Libretro.
- Core Option v2 generale `NVRAM Settings`, Disabled per default come nel core
  Supermodel. Quando è Enabled mostra soltanto le opzioni del parent caricato e
  applica tutti i valori scelti all'avvio. Non usa `Keep Current`: disabilitando
  l'opzione generale il core lascia invariati i campi NVRAM.
- 196 impostazioni operatore verificate per 35 parent. Oltre ai primi nove,
  sono coperti `airwlkrs`, `bel`, `dynabb`, `dynabb97`, `dynamcop`, `hotd`, `hpyagu98`, `indy500`,
  `gunblade`, `lastbrnx`, `manxtt`, `motoraid`, `overrev`, `rchase2`, `segawski`,
  `pltkids`, `sgt24h`, `skisuprg`, `skytargt`, `srallyc`, `stcc`, `topskatr`, `von`, `waverunr`,
  `zerogun` e `zeroguna`. I valori e le patch specifiche restano in una
  tabella separata dal motore generico, così gli aggiornamenti upstream non
  richiedono modifiche alle macchine emulate.
- Country/Nation usa USA come default quando disponibile e prevede Export come
  fallback per i giochi futuri che non espongono USA. Daytona usa inoltre
  `SINGLE` come Link ID predefinito, evitando l'attesa di un cabinet collegato.
  In VF2 Country e Drink sono indipendenti, anche se il Service Menu originale
  modifica Drink durante alcune selezioni di Country.
- Ogni formato viene riconosciuto prima della scrittura. Il core rigenera CRC o
  checksum e sincronizza copie speculari ed EEPROM soltanto per i 34 layout
  dimostrati dai campioni reali; un layout non riconosciuto resta intatto.
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

Il 10 settembre 2026 RetroArch Nightly 1.22.2 ha inoltre creato un `.srm` di
16.576 byte con `Country=USA`, checksum del contenitore valido e CRC VF2
`0xd671`. Una seconda sessione ha registrato l'import della SRAM e ha mantenuto
il valore USA; entrambe le esecuzioni sono terminate con exit code 0 e hanno
prodotto video e audio non silenzioso.
Una prova successiva ha confermato in RetroArch `NVRAM Settings=Enabled` con la
combinazione autonoma `Country=USA`, `Drink=OK`, `.srm` valido ed exit code 0.
La prova combinata delle quattro opzioni ha inoltre confermato Difficulty
Hardest e Display Type C.R.T. nello stesso salvataggio.

Il 12 settembre 2026 la stessa Nightly ha verificato la nuova implementazione
generica su ROM reali. VF2 ha mantenuto contemporaneamente Country=USA,
Drink=OK, Difficulty=Hard e Display Type=C.R.T.; Daytona ha applicato
Link ID=SINGLE e Country=USA con CRC, mirror ed EEPROM coerenti; Dead or Alive
ha applicato Nation=USA con checksum additivo e mirror EEPROM coerenti. Le tre
esecuzioni sono terminate con exit code 0 e hanno prodotto audio non silenzioso.

Una successiva prova isolata con Air Walkers ha registrato tutte le 174 opzioni
specifiche più l'interruttore generale tramite Core Options API v2. La ROM è
arrivata alla schermata di avvio per 700 frame; `Difficulty=Hard` ha modificato
entrambi i banchi EEPROM da 2 a 3, rigenerando il CRC e lasciando le copie
speculari identiche. In questa prova l'audio era intenzionalmente disabilitato.

La prova di Super GT 24h ha eseguito la ROM reale per 2300 frame con le sette
Core Options approvate. Il gioco è arrivato alla schermata coin-ready in
modalità `STAND ALONE`; i valori scelti sono rimasti nel `.srm`, il contenitore
è valido, il checksum additivo EEPROM a 16 bit è stato rigenerato e i due banchi
sono rimasti identici. Un secondo avvio con `NVRAM Settings=Disabled` ha
ricaricato lo stesso `.srm` e mantenuto invariati tutti i campi selezionati.
Questa verifica copre avvio e persistenza NVRAM: non attribuisce a ogni opzione
un effetto di gameplay, audio o rete.

La prova di Gunblade NY ha eseguito la ROM reale fino all’attract mode con le
cinque Core Options approvate: Advertise Sound, Country, Game Difficulty,
Shifting Difficulty e Cabinet Type. Il primo avvio ha applicato valori non
predefiniti, rigenerato la word d’integrità EEPROM e salvato un contenitore
valido. Un secondo avvio con `NVRAM Settings=Disabled` ha importato lo stesso
`.srm` e conservato i cinque campi. Entrambe le sessioni sono terminate con
exit code 0. La verifica copre avvio e persistenza; non attribuisce a ciascuna
opzione un effetto specifico durante il gioco.

La prova di Behind Enemy Lines ha distinto il checksum reale da una formula
equivalente solo sui campioni già acquisiti: ogni banco EEPROM somma le 31
parole little endian da 16 bit e aggiunge `0x000c`. L’algoritmo con riporto è
stato verificato sui 53 campioni, sui 106 banchi speculari e sullo stato di
fabbrica appena creato dal gioco. La ROM reale ha applicato Country=EXPORT,
Advertise Sound=OFF e Difficulty=10, raggiungendo l’attract mode con contenitore,
checksum e mirror validi. Un secondo avvio con `NVRAM Settings=Disabled` ha
ricaricato il `.srm` e conservato i tre valori; entrambe le sessioni sono
terminate con exit code 0. La prova copre avvio e persistenza.

Hanguk Pro Yagu 98 e Pilot Kids sono stati verificati con due avvii reali per
gioco. Il primo ha applicato rispettivamente Difficulty=HARDEST, Advertise
Sound=OFF, Cabinet Type=MEGALO e Favorite=TIGERS; Difficulty=More Difficult,
Demo Sound=On e Continue=Off. Entrambi hanno raggiunto il gioco con contenitore,
CRC e mirror EEPROM validi. Il secondo avvio con `NVRAM Settings=Disabled` ha
ricaricato il `.srm` e mantenuto l’intera EEPROM invariata, inclusi il prefisso
di protezione di `hpyagu98` e la firma `S32A` di `pltkids`. Le quattro sessioni
sono terminate con exit code 0. La prova copre avvio e persistenza.

## A/V Timing e overlay diagnostico

Le Core Options Video includono `A/V Timing`, con `Native (57.524160 Hz)` come
default e `60 Hz Compatibility` come alternativa. La seconda non esegue un
frame macchina per ogni callback a 60 Hz, perché ciò accelererebbe il gioco:
mantiene la cadenza hardware e duplica periodicamente l'ultimo frame. L'audio
resta al sample rate effettivo della scheda e viene distribuito su pacchetti da
60 Hz; eventuali campioni non accettati o non ancora disponibili restano in
coda e vengono recuperati nelle callback successive.

`Timing / FPS Overlay` segue il diagnostico del core Supermodel adattandolo alle
fasi disponibili in SM2. Ogni 61 callback pubblica tramite lo status OSD
Libretro tempi medi di macchina, video e audio/pacing, durata media e peggiore
di `retro_run`, FPS effettivi e capacità stimate di engine e callback. Funziona
con output software, Vulkan e OpenGL senza introdurre ImGui o un secondo overlay nel
renderer. L'opzione si aggiorna immediatamente; il cambio di timing richiede il
riavvio del contenuto perché modifica le informazioni A/V dichiarate al frontend.

## Limiti

Nel percorso software: nessun profilo completo volante/lightgun/twin-stick,
cheat o save state. Renderer Vulkan/OpenGL e Core Options
Video sono disponibili nella build descritta in [GPU.md](GPU.md). La geometria
nativa software è fissa. Rewind, run-ahead e netplay non sono dichiarati supportati.
Le etichette delle azioni specifiche dei giochi e le varianti dei dispositivi
sono da completare nella milestone 3, seguendo [LIBRETRO_DESIGN.md](LIBRETRO_DESIGN.md).

Le opzioni selezionate per gli altri parent restano rinviate finché il relativo
formato non è scrivibile con controllo d'integrità dimostrato. `hpyagu98` e
`pltkids` non ripristinano le modifiche eseguite dai rispettivi service menu dopo
il riavvio. Per `hpyagu98`
la stessa perdita è stata riprodotta nello standalone mainstream 0.9.4 e in
MAME 0.289: il menu mostra `FAVORITE=TIGERS`, ma la riapertura mostra `OFF`.
Anche MAME perde `Demo Sound=On` di `pltkids` e riapre il menu su `Off`.
Il confronto esclude quindi il contenitore `.srm` Libretro. Lo stesso script conserva invece
`ADVERTISE SOUND=OFF` di `vf2`, quindi il risultato non dipende dalla procedura
di uscita automatica. MAME e SM2-Emu espongono entrambi la SRAM di backup da
16 KiB all'indirizzo Model 2 `0x01d00000` e la EEPROM 93C46: non emerge una
batteria tampone genericamente assente dall'emulazione. Il tracciamento mostra
che i due programmi non eseguono il commit delle modifiche del menu. La lettura
all'avvio funziona però correttamente: EEPROM con CRC e mirror rigenerati hanno
prodotto `FAVORITE=TIGERS` in `hpyagu98` e `Demo Sound=On` in `pltkids`.
Le Core Options aggirano il limite applicando i valori alla EEPROM prima del
reset, senza patchare la RAM volatile. La causa originaria resta aperta: in un
passaggio futuro va seguito il percorso di salvataggio dei due programmi per
capire perché cambio ed `EXIT` non scrivono né sulla porta EEPROM né nella SRAM
di backup, e correggere l'emulazione se il mancato commit dipende da SM2-Emu o
dalla piattaforma Model 2 condivisa con MAME. `rascot2` resta fuori dalla
campagna corrente.

La prova di gameplay usa input sintetici attraverso RetroArch: non convalida
un controller fisico. CoreAudio e PCM registrato confermano il percorso audio;
qualità percepita e ascolto manuale restano da verificare. Non è una matrice di
compatibilità completa né una misura delle prestazioni su altre piattaforme.

La Nightly installata 1.22.2 ha prodotto un'immagine, ma ha mostrato problemi
nel ciclo di avvio da CLI; la validazione conclusiva usa la Stable indicata.
Un primo replay incompleto sulla Stable terminava subito e faceva fallire la
cattura finale del frontend: il test consegnato include tutti gli stati input.
