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

For a separately supplied device BIOS, the Libretro core searches
`<system directory>/sm2-emu/<device>.zip` or `.7z` first, then beside the game
archive, then inside the game archive. Parent ROM archives continue to be
searched beside the game. No BIOS is included with the core or system files.
The standalone and headless loaders retain their existing sibling search.

On 2026-10-07, an isolated RetroArch 1.21.0 run loaded Virtua Cop from a
user-owned game archive deliberately missing `epr-17181.6`. The matching
`model1io2.zip` in the test system directory contained that device file.
The log recorded the separate device archive and 2300 frames completed with
gameplay screenshot, nonzero PCM and valid SRAM. Test copies and recordings
remain under the ignored `build-libretro/validation/` directory.

La Core Option System `ROM CRC Verification (Restart Required)` è abilitata
per default e confronta ogni chip con il CRC32 dichiarato in `games.xml`.
Disabilitandola, il loader cerca i chip soltanto per nome; l'integrità interna
ZIP/7z resta comunque verificata durante l'estrazione. Questa modalità è
destinata esclusivamente alla diagnostica: ROM errate, modificate o
incompatibili possono produrre errori di emulazione, crash o dati di
salvataggio corrotti.

La Core Option System `i960 Round-to-Nearest-Even Fix (Restart Required)` è
disabilitata per default e conserva quindi il comportamento upstream. Quando
abilitata applica alle istruzioni `roundr`, `roundrl`, `cvtri` e `cvtril` il
round-to-nearest con exact half verso il vicino pari documentato da Intel.
L'opzione viene letta al caricamento del contenuto.

Nell'installazione macOS di sviluppo usata per questo progetto, la copia
autorevole del database è:

`/Volumes/MacBook External Drive/Retro/_Media/_Multi (RetroArch)/system/sm2-emu/games.xml`

Ogni build destinata alle prove locali deve aggiornare quel file insieme al
core installato in RetroArch.
On macOS, replace the installed library using a fresh temporary file in the
cores directory followed by an atomic rename, then verify its SHA-256. Avoid
overwriting a mapped library in place: cached code-signing pages may cause a
CODESIGNING Invalid Page termination even when the disk signature is valid.

Il core usa direttamente la directory save fornita dal frontend, che può già
essere specifica per `SM2-Emu` o per il contenuto. RetroArch gestisce qui
`<gioco>.srm`; eventuali immagini standalone `<gioco>.nv` e
`<gioco>.eeprom` da importare vanno nella stessa directory. Il core non crea
ulteriori sottocartelle `sm2-emu/<gioco>`.

## Profili digitali

Il core riconosce esplicitamente tutti gli 87 set parent/clone attuali.
I profili originali seguono i binding approvati in `Docs/revisione_profili_model2.xlsx`;
il nuovo profilo Power Sled segue l'upstream come documentato in
`Docs/UPSTREAM_ROM_LOADER_PACKAGE_2026-10-08.md`. Firme sconosciute o
cambiate restano nel fallback conservativo. The first Input option, **Controller Profile Type**, selects **Normal** or
**Normal + Test/Service** (default) globally for every input port. Controls
registers only the current game-specific device type plus the frontend's None;
it retains remapping but no longer selects the Test/Service variant. Normal
removes the supported Test/Service descriptors and polling. Changes apply on
return to the game, without machine reset. Saved legacy reduced-device IDs are
accepted, but the Core Option now owns the variant across all ports. Poiché Model 2 dispone soltanto
delle linee `Service A` e `Test A`, L3/R3 del Player 2 sono binding secondari
virtuali delle stesse linee, non comandi B distinti.

Historical per-game reports below describe the previously selectable device
variants; their validation remains historical evidence. Current selection is
global through Controller Profile Type as described above.

I controlli ROM-free verificano tutti i set riconosciuti, le stringhe dei
profili e delle azioni, P1/P2 indipendenti, the global variants, one-type registration and
la traduzione hardware. In particolare Virtua Striker associa South/East/West
a `Short Pass`/`Long Pass`/`Shoot` scrivendo rispettivamente i bit Model 2
`0x04`/`0x01`/`0x02`; Virtual On usa i due stick analogici per le due leve,
L1/R1 per `Left/Right Dash (Turbo)` e L2/R2 per `Left/Right Shot Trigger`.

Le prove RetroArch macOS con VF2, Virtua Striker e Virtual On hanno raggiunto
una partita reale con video, audio e SRAM validi. Questo dimostra caricamento,
registrazione del profilo e input automatizzato; la sensazione e il remapping
di un controller fisico restano una verifica manuale distinta.

I parent Dynamite Baseball e Dynamite Baseball 97 condividono il profilo
`Joystick (Standard): Baseball (Dynamite Baseball)`. Ogni giocatore conserva
joystick e `Button 1`/`Button 2`; `Bat Swing` usa esclusivamente Right Analog Y
spinto verso il basso e pilota separatamente `bat1`/`bat2`. La direzione verso
l'alto resta al valore di riposo. Anche questo profilo espone le varianti con
e senza Test/Service. Entrambi i parent hanno raggiunto una partita in
RetroArch macOS con audio e Save RAM validi.

Sega Water Ski espone `Special: Water Ski`: D-Pad Up/Down selezionano
`Select Up`/`Select Down`, L1/R1 controllano `Pitch Left`/`Pitch Right`, South è
`Set` e Left Analog X è `Slide`. West/East sono binding secondari rispettivamente
per `Pitch Left`/`Pitch Right`. Slide applica direttamente la polarità invertita
dichiarata nei metadata (`X+` produce `00`, `X-` produce `FF`). Start condivide
la linea hardware di `Select Down`, come indica il descrittore `Start / Select
Down`. La ROM parent ha raggiunto una gara in RetroArch macOS con audio e Save
RAM validi.

Sega Ski Super G espone `Special: Ski Super G`. D-Pad Up/Down sono `Zoom In` e
`Zoom Out`, L1/R1 sono i due Foot Sensor attivi alti e South/East/West sono
`Select 2`/`Select 3`/`Select 1`. Right Analog X controlla `Inclining` sul
canale Model 2 0 e Left Analog X controlla `Swing` sul canale Model 2 1.
Independent axes are the approved core-specific layout. For coupled steering,
users can bind both virtual horizontal axes to the same physical axis in
Settings > Input > Port 1 Controls; Swing's metadata already reverses its
polarity. No additional Core Option or Quick Menu selector is provided.
Exact upstream response/rounding equivalence through this binding remains
unverified. Swing
applica la polarità invertita dichiarata nei metadata: X+ produce `00` e X-
produce `FF`. Ski Super G now has a stationary Drive Board acknowledgement
stub in the common machine: PE commands receive the expected PD startup
responses (`00` → `CE`, centre command → `FF`). It reaches the attract sequence
without pressing Test. The old timed Test bypass is removed. ROM, NVRAM,
calibration and bindings are unchanged; the mechanism and full board protocol
are not emulated. See `Docs/SKI_SUPER_G_DRIVE_BOARD_STUB_2026-10-11.md`.

Le quattro revisioni Top Skater condividono `Special: Top Skater`. D-Pad
Left/Right corrispondono a `Select Left`/`Select Right`, South/East a `Jump
Tail`/`Jump Front`, Left Analog X a `Curving` e Right Analog X a `Slide`.
Curving retains the metadata polarity (`X+` reaches `00`, `X-` reaches `FF`).
`Top Skater Curving Response` defaults to `Progressive (Fine Center)`, using
upstream's small deadzone and quadratic travel; `Linear` restores the previous
direct curve. The option is visible for all games and only affects Top Skater.
L2/R2 add Slide Left/Right aliases without removing Right Analog X; the
stronger input wins (stick wins ties). D-Pad Left/Right also steer at full lock.
Direzioni e pulsanti non assegnati nel foglio non vengono esposti. Il parent ha
raggiunto una sessione di gioco in RetroArch macOS con audio e Save RAM validi;
le tre revisioni ereditano la stessa firma verificata dal catalogo.

Wave Runner espone `Special: Wave Runner`: D-Pad Up è `View`, R2 è `Throttle`,
Left Analog X/Y sono `Handle`/`Pitch` e Right Analog X è `Roll`. Il core usa il
valore di riposo dichiarato nei metadata per tutti e quattro i canali e lascia
al frontend l'eventuale conversione di R2 digitale in analogico. Il safety
sensor del cabinet resta un dettaglio hardware interno e non viene esposto come
binding. Il parent ha raggiunto una sessione di gioco in RetroArch macOS con
profilo registrato, audio non silenzioso e Save RAM valida.

Air Walkers espone P1-P4 con `Joystick (Standard): Basketball (Air Walkers)`,
D-Pad e `Button 1`/`Button 2`/`Button 3` nelle posizioni approvate. Il backend
riproduce la matrice del cabinet documentata da MAME: il bit 7 della porta F
seleziona la coppia P1/P2 oppure P3/P4 sulle porte C/D e commuta con essa le
linee Start. Coin 1-4 rimangono sulle rispettive linee comuni. Il default NVRAM
del gioco resta `2P Simultaneous`; scegliendo `4P Simultaneous` il gioco usa la
seconda coppia già pubblicata dal core. Il parent aveva già raggiunto una
partita reale con audio e Save RAM validi; i quattro percorsi input e la matrice
sono coperti dai controlli ROM-free.

Royal Ascot II espone `Joystick (Standard): Horse Racing (Royal Ascot II)` a un
giocatore, usando D-Pad e tre pulsanti generici come consentono i metadata
correnti. La prova reale registra il profilo, visualizza il titolo e salva la
Save RAM; l'audio resta silenzioso e il replay non supera la sequenza iniziale
che conduce all'attesa SegaNet già documentata. Il set è preliminary e il
cabinet fisico non è ancora identificato, quindi il risultato non dimostra la
giocabilità né la semantica dei tre pulsanti.

Desert Tank espone `Joystick (Analog): Desert Tank + VR3`. I metadata condivisi con lo
standalone descrivono ora i tre canali della scheda Model 1 I/O: Left Analog X
è `Steering`, R2 è `Accelerator` e Left/Right Analog Y controllano `Elevation`.
Per default lo scostamento dello stick determina direzione e velocità della
torretta, mentre il rilascio mantiene l'ultima posizione nell'intervallo
`00`–`FF`; la posizione iniziale è `80`. Le Core Options sempre visibili
`Desert Tank Elevation Control`, `Desert Tank Elevation Speed (Relative Only)`
e `Desert Tank Elevation Axis Mode` permettono rispettivamente di ripristinare
la mappatura assoluta, regolare la velocità relativa e invertire l'asse.
D-Pad Down/Left/Up
sono `VR1 (Blue)`/`VR2 (Green)`/`VR3 (Red)`,
South/East sono
`Machine Gun`/`Cannon`, con gli alias secondari RB/LB, e West aziona `Shift`
come toggle. Il profilo ha le
varianti con e senza Test/Service. Il parent ha raggiunto una missione reale in
RetroArch macOS con tutti i percorsi input presenti nel replay, audio non
silenzioso e Save RAM valida.

Hanguk Pro Yagu 98 espone a due giocatori `Joystick (Standard): Baseball
(Hanguk Pro Yagu 98)`. Entrambe le porte usano D-Pad e `Button 1`/`Button 2`/
`Button 3` sul layout elettrico VF2 indicato da MAME, con varianti con e senza
Test/Service. È distinto dal profilo Dynamite Baseball perché non dichiara assi
analogici Bat Swing. Il problema noto di `FAVORITE` non salvato dal Service
Menu riguarda la NVRAM e non modifica profilo o binding. Il parent ha raggiunto
una partita reale in RetroArch macOS con input registrato, audio non silenzioso
e Save RAM valida.

## Profili di guida a quattro marce

I quattro set Daytona espongono `Driving: 4-Speed + VR4`; i cinque set Sega
Rally espongono `Driving: 4-Speed + VR1 + Handbrake`. Tutte le descrizioni e
posizioni corrispondono al foglio revisionato. Sterzo conserva la precisione
analogica dello stick sinistro; Brake e Accelerator leggono esclusivamente i
trigger analogici L2/R2. Sega Rally traduce South negli estremi del freno a
mano analogico IN2: 0x00 rilasciato e 0xFF premuto.

La Core Option `4-Speed Shifter` usa H-Gate per default e può passare alla
modalità Standard; West resta `4-Speed: Neutral` e L1/R1 restano `Shift
Down`/`Shift Up`. Entrambi i profili hanno le varianti con e senza Test/Service.
I controlli ROM-free coprono i nove set, cablaggio VR, assi, pedali e cambio.
Le prove RetroArch macOS hanno raggiunto gare reali con input registrato:
Daytona a 300 km/h e Sega Rally a 127 km/h nei frame finali acquisiti.

### Gear Shift Neutral Time

Input groups **Gear Shift Neutral Time** immediately after **4-Speed Shifter**.
It follows upstream: **100 ms** by default, **0–500 ms** in 10 ms steps. Shift
Up/Down between nonzero gears temporarily sends neutral; direct gear selection
and explicit neutral bypass/cancel the interval. At 100 ms this occupies six
native machine frames. **0 ms** restores immediate shifting and cancels a
pending interval. Changes apply on return to gameplay without a machine reset;
nonzero edits affect subsequent shifts. The option is visible for every game
and is a no-op outside the four-speed gearbox profiles (including their clones).
It does not delay the two-button native sequential shifters used by Indy 500,
Manx TT and similar titles. Save States preserve a pending interval.
See `Docs/GEAR_SHIFT_NEUTRAL_TIME_2026-10-18.md`.

## Profili di guida con cambio sequenziale

I tre parent Indy 500, Over Rev e Sega Touring Car Championship, con le loro
revisioni per un totale di dieci set, espongono il profilo approvato `Driving:
Sequential + VR2`. D-Pad Up/Down corrispondono a `View 1`/`View 2`, L1/R1 a
`Shift Down`/`Shift Up`, L2/R2 ai pedali analogici e lo stick sinistro X a
`Steering`. Il cambio agisce direttamente sui due ingressi momentanei Model 2:
il core non mantiene uno stato della marcia. Le direzioni D-Pad e gli assi non
assegnati nel foglio non vengono esposti.

Anche questo profilo offre il dispositivo predefinito con Test/Service e quello
ridotto senza i due comandi. Il core non converte ingressi digitali in pedali
analogici; l'eventuale rimappatura appartiene al frontend.

Le ROM parent sono state avviate in RetroArch macOS con il replay specifico:
Indy 500 ha raggiunto la griglia di partenza, Over Rev la selezione modalità e
STCC l'inserimento nome. Tutte e tre le esecuzioni hanno registrato il profilo
atteso, audio non silenzioso e Save RAM della dimensione prevista.

Super GT 24h espone separatamente `Driving: Sequential + VR1`: conserva lo
stesso sterzo, pedali e cambio sequenziale, ma assegna soltanto `VR1` a D-Pad
Down come prescritto dal foglio. Accelerator e Brake conservano la polarità
invertita dichiarata dai metadata, senza correzioni aggiuntive nel binding.
MAME segnala per questo gioco problemi analogici ancora aperti: sterzo che non
si centra e pedali che pulsano invece di mantenere un valore stabile. D-Pad
Up non è esposto. Anche questo profilo
ha le varianti con e senza Test/Service. La ROM parent ha raggiunto la selezione
del circuito in RetroArch macOS con audio e Save RAM validi.

I tre set Manx TT espongono `Driving: Sequential (Manx TT Superbike)`. `Bank`
usa lo stick sinistro X con la polarità invertita dichiarata dal gioco; Brake e
Accelerator usano esclusivamente i trigger analogici L2/R2 e il cambio usa
L1/R1. L'azione combinata `Start / VR` è associata a Start e, come binding
secondario coerente con VR1, a D-Pad Down. `Cabinet Type` usa `Twin` come default sia nelle Core Options sia nel
setup NVRAM del primo avvio. Le varianti con e senza Test/Service sono
disponibili. La ROM parent ha
raggiunto il controllo iniziale del motion slider con audio e Save RAM validi.

I due set Motor Raid espongono `Driving: Sequential (Motor Raid)`. Mantengono
Bank, pedali e `Start / VR` nelle posizioni del foglio. Motor Raid non ha un
cambio: `Kick` usa South con L1 come binding secondario, mentre `Punch` usa East
con R1 come binding secondario. Ogni coppia raggiunge lo stesso ingresso arcade.
La ROM parent ha raggiunto una gara in RetroArch macOS;
audio e Save RAM sono validi.

Le quattro regolazioni analogiche riprese da Supermodel sono sempre visibili e
agiscono soltanto sui profili Driving. `Driving Steering Response` offre
`Linear`, predefinito, `Progressive (Fine Center)` e `FBNeo Logarithmic (Fine
Center)`. `Driving Steering Output Range`, `Driving Accelerator Output Range`
e `Driving Brake Output Range` sono indipendenti, usano `100%` come default e
offrono valori comuni dal `50%` al `150%`. Sotto il 100% riducono l'escursione
emulata; sopra il 100% raggiungono il fondo corsa con minore corsa fisica. Le
regolazioni preservano polarità e riposo Model 2; Accelerator si applica anche
al Throttle dei profili motociclistici. Come nel core Supermodel, il preset
sterzo `63% (30-80-D0)` produce l'intervallo ADC esatto `30-80-D0`, mentre il
preset pedali `75.3% (00-C0)` produce l'intervallo esatto `00-C0`.

Sky Target espone `Joystick (Analog): Sky Target`. Lo stick sinistro conserva
gli assi `Analog Joystick X`/`Analog Joystick Y` e le polarità dichiarate dal
gioco: X invertito, Y normale. South/RB ed East/LB corrispondono a
`Machine Gun` e `Missile`; D-Pad Up controlla `View Change`.
Il profilo a un giocatore offre le varianti con e senza Test/Service. La ROM
parent ha raggiunto una partita in RetroArch macOS con punteggio e colpi
registrati; audio e Save RAM sono validi.

I dieci set Gun revisionati sono implementati in due profili esposti. Virtua
Cop, Virtua Cop 2, House of the Dead, Gunblade NY e Rail Chase 2 usano `Gun`;
Behind Enemy Lines usa `Gun: Behind Enemy Lines`. Entrambe le porte offrono le
varianti con e senza Service A/Test A; sulla porta 2 sono alias virtuali delle
stesse linee hardware A.

La Core Option `Gun Input Mode`, derivata da Supermodel, seleziona `Standard`,
`Lightgun Only`, `Mouse + Analog Stick`, `Mouse Only` o `Analog Stick Only`.
Standard unifica le tre sorgenti in un cursore virtuale. Le modalità dedicate
e i descrittori vengono aggiornati immediatamente. South e RB sul RetroPad,
grilletto e clic sinistro sono `Shot`. La Core Option `Analog Stick Aim` sceglie come agisce la levetta analogica: `Cursor` (predefinito) sposta il cursore, che resta fermo al rilascio; `Absolute` usa la posizione della levetta come punto di mira, come una pistola ottica, e riporta il cursore al centro al rilascio. In Standard e Mouse + Analog Stick il mouse muove il cursore mentre la levetta è a riposo. BEL assegna `Missile` a East/LB, clic destro,
Lightgun Aux A e Reload; per BEL Reload è soltanto un secondo binding della stessa azione.

Il core conserva le due interfacce arcade. Virtua Cop 1/2 e House of the Dead
ricevono coordinate seriali RS-422 a 10 bit; East o LB, clic destro o il comando
Lightgun Reload eseguono `Reload Offscreen` impostando lo stato seriale
off-screen e premendo Shot. Lo stato nativo della Lightgun arriva separatamente
da `RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN`; le coordinate Mouse al bordo non lo
generano. La Core Option `Off-Screen Reload Shortcut`, sempre
visibile e attiva per default, abilita o disabilita insieme i tre binding nei
giochi supportati. Il grilletto di una lightgun fisicamente puntata fuori schermo resta
sempre un gesto nativo di ricarica. Gunblade NY, Rail Chase 2 e BEL ricevono invece
coordinate posizionali a 8 bit nel mux analogico e non espongono Reload
Offscreen. House of the Dead now follows upstream `fa88f95`: P2 uses IN1 bit 1
on all three revisions. Serial on-screen coordinates map one unit inside the
calibrated endpoints, matching upstream `902fb86`; explicit off-screen status
and reload shortcuts still use the endpoint. BEL mantiene Shot/Missile separati per P1/P2 e traduce le linee
Test/Service scambiate dal cabinet in `Service A`/`Test A`.

BEL dipende inoltre da quattro coppie centro/ampiezza e dai corrispondenti
parametri temporanei in work RAM. Il workaround upstream riconosce sia lo stato
non calibrato sia il preset distribuito da `Automatic Initial NVRAM Setup`, così
il preset non impedisce l'inizializzazione dei parametri temporanei quando manca
l'handshake della gun board. Un salvataggio già calibrato è stato provato in
RetroArch macOS: gli assi cambiano nel Service Menu e il mirino del gioco si
muove correttamente sia in orizzontale sia in verticale.

La Core Option `Show Crosshair` usa `Automatic` come default unico: mostra P1
nei giochi con pistola seriale e conserva il mirino in gioco nei titoli gun
posizionali, seguendo SM2-Emu. `Disabled`, `Player 1 Only`, `Player 2 Only` e
`Players 1 & 2` sono scelte esplicite; queste ultime rendono il mirino esterno
selezionabile anche per Gunblade NY, Rail Chase 2 e Behind Enemy Lines.
`Crosshair Style` usa per default il mirino SM2-Emu, circolare con linee
cardinali, P1 verde e P2 ciano; lo stile Supermodel a quattro cunei, P1 rosso e
P2 verde, resta selezionabile. Entrambi usano lo stesso cursore virtuale
condiviso da Lightgun, Mouse e Analog Stick, funzionano nei renderer Software,
Vulkan e OpenGL e scompaiono durante la ricarica fuori schermo.

I controlli senza ROM coprono tutte le modalità, i descrittori, le calibrazioni,
i bit dei grilletti e la distinzione del reload. Le prove RetroArch macOS con
Gunblade NY e Virtua Cop 2 hanno raggiunto una partita con input analogico
registrato, audio non silenzioso e Save RAM valida. Le periferiche fisiche
Lightgun e Mouse restano da provare manualmente.

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
- Core Option `Automatic Initial NVRAM Setup`, Enabled per default. Per i 35
  parent verificati e i cloni autorizzati, quando non esistono un `.srm` valido o file nativi
  `.nv`/`.eeprom` validi, il core carica il campione completo ricavato dal
  Service Menu prima del primo frame. Imposta quindi Country/Nation su USA,
  oppure Export se USA non è disponibile, e i valori offline necessari per
  evitare attese di cabinet collegati. Come scelta specifica del core, la
  famiglia Daytona riceve inoltre Cabinet=Deluxe e Super GT 24h riceve I/O
  Type=C, coerente con l'I/O emulato. Il setup automatico non sostituisce i
  salvataggi esistenti; le normali scritture del gioco continuano a persistere.
  Eliminando i dati di salvataggio del gioco si rigenera il setup.
  Ventuno cloni con campioni byte-identici possono usare il template parent e
  altri 27 usano un template dedicato. Dodici di questi applicano le opzioni
  del parent; `daytona93`, `daytonas`, `dyndeka2`, `dyndeka2b`,
  `manxttdx`, `motoraiddx`, `stcca`, `stccb`, `stcco`, `vf2a`, `vf2o`, `indy500d`,
  `vstrikero`, `srallycdx`, `srallycdxa` e `hotdp` hanno cataloghi specifici verificati dal
  rispettivo Service Menu. Tutti i 48 cloni dispongono quindi di un template
  dedicato o di un'eredità parent verificata.
- Core Option v2 generale `NVRAM Settings`, Disabled per default come nel core
  Supermodel. Quando è Enabled mostra soltanto le opzioni del gioco caricato e
  applica tutti i valori scelti all'avvio. Non usa `Keep Current`: disabilitando
  l'opzione generale il core lascia invariati i campi NVRAM.
- Core Option `Gun Input Mode` con gli stessi cinque percorsi di Supermodel:
  Standard, Lightgun, Mouse + Analog Stick, Mouse e Analog Stick.
- Core Option `Off-Screen Reload Shortcut`, per Virtua Cop 1/2 e House of the
  Dead: RetroPad East/LB, Mouse destro e Lightgun Reload; attiva per default.
- Core Option `Mouse Edge Off-Screen Reload`, disabilitata per default: nei tre
  giochi seriali interpreta un Mouse Left premuto entro il 5% esterno del
  cursore virtuale come colpo off-screen, ripristinando su richiesta il
  comportamento MAME precedente senza alterare il percorso Lightgun.
- Core Option `Show Crosshair`, con default `Automatic` dipendente dal tipo di
  gun e selezione esplicita di P1, P2 o entrambi anche per i giochi posizionali;
  `Crosshair Style` seleziona SM2-Emu (default) o Supermodel, con composizione
  coerente nei percorsi Software, Vulkan e OpenGL.
- 299 impostazioni operatore verificate per 35 parent e sedici cloni con menu
  specifico. `daytona93` espone il proprio menu ridotto di quattro voci;
  `daytonas` aggiunge Cabinet=Special e Promote Saturn; i tre cloni STCC usano
  la propria codifica Country. `dyndeka2` e `dyndeka2b` omettono la riga
  informativa HP Password; `motoraiddx` omette Engine Volume=Out of Use e
  aggiunge Cabinet Type=Deluxe/Twin; `manxttdx` omette Cabinet Type, Link Type e
  i due Revise Mode; `indy500d` omette Engine Volume e Default
  View; `vstrikero` omette One Match Mode; `srallycdxa` omette Cabinet Type e
  Link Type; `hotdp` usa il proprio formato a banche da 24 byte ed espone
  Difficulty, Blood Color, Advertise Sound e Country. Oltre ai sedici cloni,
  sono coperti `airwlkrs`, `bel`, `dynabb`, `dynabb97`, `dynamcop`, `hotd`, `hpyagu98`, `indy500`,
  `gunblade`, `lastbrnx`, `manxtt`, `motoraid`, `overrev`, `rchase2`, `segawski`,
  `pltkids`, `sgt24h`, `skisuprg`, `skytargt`, `srallyc`, `stcc`, `topskatr`, `von`, `waverunr`,
  `zerogun` e `zeroguna`. I valori e le patch specifiche restano in una
  tabella separata dal motore generico, così gli aggiornamenti upstream non
  richiedono modifiche alle macchine emulate.
- Country/Nation usa Export come default quando disponibile e prevede USA come
  fallback per i giochi che non espongono Export. Daytona usa inoltre
  `SINGLE` come Link ID predefinito, evitando l'attesa di un cabinet collegato.
  In VF2 Country e Drink sono indipendenti, anche se il Service Menu originale
  modifica Drink durante alcune selezioni di Country.
- Ogni formato viene riconosciuto prima della scrittura. Il core rigenera CRC o
  checksum e sincronizza copie speculari ed EEPROM soltanto per i layout
  dimostrati dai campioni reali; un layout non riconosciuto resta intatto.
- I campioni iniziali sono generati in modo riproducibile dagli `.srm` validati
  con `scripts/generate-initial-nvram-templates.py`, compressi e verificati di
  nuovo a runtime tramite CRC. L'infrastruttura consente di sostituire i
  campioni dopo la verifica delle calibrazioni senza cambiare il flusso del core.
- Errori di caricamento segnalati al frontend; dettagli del loader nel log
  stderr. Nessun percorso implicito nella directory corrente.

I profili digitali semplici usano il D-pad e i pulsanti per posizione del pad:

| RetroPad | Azione |
| --- | --- |
| Select / Start | Moneta / Start, indipendenti per P1/P2 |
| South / East / West | Pulsanti originali 1 / 2 / 3 |
| LB | Alias aggiuntivo del pulsante 3 |
| North / RB | Pulsante 4 e alias, soltanto nei profili che non sono a tre pulsanti |
| L3 / R3, porta 1 | Service A / Test A |
| L3 / R3, porta 2 | Alias virtuali di Service A / Test A |

Il bit Start specifico del gioco viene rispettato. Le azioni sono abilitate
soltanto nei profili riconosciuti; quelli ancora non implementati mantengono il
fallback conservativo. Le posizioni analogiche di riposo restano quelle della
macchina.
I descrittori vengono aggiornati al caricamento e rimossi all'unload.
Quando un gioco non offre gameplay sulla porta 2, quella porta conserva lo stesso
nome del profilo del gioco: i descrittori RetroArch espongono soltanto Coin e
Start, senza creare un profilo generico separato.

## Collegamento tra cabinet

The System menu exposes exactly two network controls: `Linked Cabinets` and
`Auto Network Role Settings (ANRS)`. Unsupported games display `Not Supported`.
Linked Cabinets defaults to Disabled (`-1`, with `0`/`1` aliases); supported
totals include Relay. ANRS is independent of NVRAM Settings: `-1` manual,
`0` native Standalone/No Link, positive native IDs, and `r` special role
(`l`/`s` aliases). Power Sled uses colors 1–4 with fixed program roles; dedicated
Relay programs offer only `r`. The communication board remains fitted and uses
local loopback offline, matching upstream, including for the reviewed Dynamite
Baseball, Pilot Kids, Rail Chase 2 and Zero Gunner families.
Topology never silently
converts the selected native role.

Changes are retained even when frontend visibility reads consume the option
update flag. Resume applies effective changes in one machine reset, retaining
the frontend lobby and callbacks. Hidden manual role selectors track applied
values for handover when ANRS is disabled. See
[the current network contract and evidence](Docs/NETWORK_TWO_OPTION_2026-10-17.md)
and [launcher instructions](Docs/LINKED_CABINET_LAUNCHER.md).

The following dated records describe earlier network qualification, not the
current option values or defaults.

The first STCC 3/3 attempt exposed a Netpacket startup overflow on the last
client. Before that client's roster formed, early peer frames and repeated
identical `Hello` packets could fill the pending queue. The adapter now drops
frames that cannot yet enter the local ring and keeps one pending copy of each
identical `Hello`. Unit checks cover 1400 early frames and 1400 repeated
hellos, while preserving the bounded failure for a 1025-frame burst after
the roster is ready. The repeated STCC 3/3 RetroArch run passed; extended
stalls and reconnects were subsequently checked in the
[2026-10-09 network package](Docs/NETWORK_VALIDATION_2026-10-09.md).

The same package extends preset checks to a 20-case matrix, repairs known-peer
identity requests after a local Restart and advances the protocol to NetBoard
v3. All participants must use the updated build. Actual Daytona Core Options
transitions between two cabinets and Disabled, menu return, coordinated Restart,
2/20-second stalls and process reconnection were exercised in isolated
RetroArch. Reconnection and an individual cabinet Restart require coordinated
Restart to recover the native Daytona board. Motor Raid Deluxe's drive-board
precondition is avoided for new automatic setups by the approved Cabinet Type =
Twin default, also used by the NVRAM selector. Existing saves retain precedence;
Deluxe remains selectable. Subsequent Manx TT qualification corrects the counted
Relay's native status slot and verifies two-rider races with LIVE camera,
Time Up/ranks and subsequent progression on the parent and Revision C.
See [Manx TT Gameplay Validation](Docs/MANX_TT_GAMEPLAY_VALIDATION_2026-10-09.md).
Other roster-only checks do not establish synchronized gameplay. Both installed local core
copies were updated and hash-verified. See the package for exact cases, command
flags, before/after evidence and recovery limits.

Tutti i partecipanti devono usare la stessa build del core, lo stesso timing e
lo stesso valore `Linked Cabinets`. Usano inoltre lo stesso ROM set, salvo il
programma Relay `vonr` richiesto dalla configurazione Virtual On a tre istanze.
Con `NVRAM Settings=Enabled`,
configurare e riavviare il contenuto così:

| RetroArch | Link ID | Car Number |
| --- | --- | --- |
| Host | Master | 1 |
| Client 1 | Slave | 2 |
| Client successivi | Slave | 3–8, senza duplicati |

Per STCC e Sega Rally, `Link Type` contiene già ruolo e numero: usare `Car 1`
sull'host e numeri progressivi sui client, senza duplicati. Con il valore massimo,
STCC usa otto istanze `Car 1`…`Car 8` più una `Relay`; Sega Rally usa quattro
istanze `Car 1`…`Car 4` più una `Relay`. `Linked Cabinets` mostra soltanto il
numero totale dei partecipanti; il runner assegna automaticamente il Relay
all'ultima istanza.

Indy 500, Motor Raid, Wave Runner e Sega Ski Super G usano invece due campi
separati. Impostare `Network Type=Master` e `Cabinet ID=1` sull'host;
`Network Type=Slave` e ID progressivi sui client. Il limite è 8 per Indy 500 e
4 per gli altri tre giochi. In Motor Raid, `Live` è il ruolo Relay/live monitor:
è incluso nel totale, può essere usato da una sola istanza e non sostituisce uno
dei cabinet giocabili Master/Slave. Il runner lo assegna all'ultima istanza con
`--include-relay`; la stessa opzione consente un Relay anticipato nelle famiglie
STCC e Sega Rally.

Super GT 24h usa `Link Type=Car No.1 Master` sull'host e `Car No.2`…`Car No.4
Slave` sui client; impostare `Link Max` allo stesso totale selezionato in
`Linked Cabinets`. Over Rev usa `Link Max=2/3/4 Links`, uguale su ogni istanza,
e `Link Type=Master CarNo.1` sull'host oppure `Slave CarNo.2`…`CarNo.4` sui
client. Il runner applica automaticamente queste combinazioni.

Manx TT usa `Master` e `Slave` con due cabinet; selezionando tre cabinet aggiunge
una terza istanza `Relay`. `manxtt` e `manxttc` sono supportati, mentre
`manxttdx` è escluso perché il relativo menu acquisito non espone `Link Type`.

For the bounded local gameplay campaign, reuse `scripts/test-manxtt-gameplay.py`;
see [the campaign and evidence](Docs/MANX_TT_GAMEPLAY_VALIDATION_2026-10-09.md).
The separate DX single-program check reaches a race after the operation tutorial
using normal bank, brake, Start and throttle inputs; it adds no link support,
binding or calibration change.

Virtual On usa due programmi Twin come cabinet giocabili: il primo imposta
`Network Link Attribute=Master`, il secondo `Slave`. Se `Linked Cabinets=3`,
la terza istanza deve caricare il programma dedicato `vonr`, che svolge il ruolo
Relay/live monitor e conserva `Network Link Attribute=No Link`. Il trasporto
confronta la famiglia di rete `von`, non il nome esatto del set, così Twin e
Relay possono condividere la sessione. `von`, `vonj`, `vonu` e `vonr` espongono
la Core Option; è ammesso un solo Relay e non è un cabinet giocabile.

Avviare prima l'host RetroArch e poi collegare i client. Le istanze devono
usare directory di salvataggio separate, come accade naturalmente su più
macchine. `Automatic Initial NVRAM Setup` può restare abilitato: le opzioni
NVRAM selezionate vengono applicate prima del primo frame.

La verifica macOS ha usato due istanze isolate di RetroArch Nightly 1.22.2 e
la ROM parent `daytona`, 3600 frame per istanza. RetroArch ha collegato il
secondo partecipante; le communication board hanno completato l'anello come
`01/02` e `02/02`. Gli input registrati hanno portato entrambi i cabinet nella
stessa gara: l'host rosso mostra `2nd/2` e l'auto blu `2P` davanti; il client
blu mostra `1st/2`. Entrambi avanzano a circa 143–145 mph. I rispettivi `.srm`
conservano Master/Car 1 e Slave/Car 2 e i processi terminano correttamente.

Questa prova dimostra il collegamento tra le due macchine emulate, lo scambio
dei frame della communication board e una gara a due auto con input automatici.
Una seconda prova automatica ha avviato tre istanze isolate di RetroArch Nightly
1.22.2: l'host ha registrato i collegamenti `2/3` e `3/3`, mentre i partecipanti
0, 1 e 2 hanno tutti formato lo stesso roster da tre cabinet. Le tre istanze si
sono chiuse con codice 0, senza terminazione forzata né processi residui. Questa
prova copre il roster reale oltre due cabinet, ma non equivale ancora a una gara
sincronizzata a tre auto. Una successiva prova distribuita ha collegato una
istanza macOS e una Batocera x86_64 fisica: entrambe hanno formato il roster 2/2
e mostrato `UP TO 2 RACERS WANTED`. La stessa topologia è rimasta connessa per
due minuti con entrambi gli host cablati, senza errori di collegamento. Restano
una prova manuale con due controller e la gara a tre. Una prova successiva ha
applicato lo stesso replay di guida validato a entrambi gli host: il cabinet
rosso macOS e quello blu Batocera hanno raggiunto la stessa gara, giro 1/8,
nelle posizioni reciproche 2/2 e 1/2 e in movimento a circa 177–185 mph. Questo
conferma a campione una gara sincronizzata tra le due piattaforme.

La stessa topologia è stata qualificata anche tra macOS arm64 e Windows x86_64
con gli artifact CI della medesima revisione `1d8a1ab`. RetroArch 1.22.2 su
macOS ha ospitato la sessione e il client Windows si è collegato via LAN: le due
communication board hanno formato il roster 2/2 e il replay ha portato entrambe
le postazioni nella stessa gara. La cattura macOS mostra il cabinet rosso in
posizione 2/2 con l'auto `2P` davanti; quella Windows mostra il cabinet blu in
posizione 1/2. Entrambi i processi sono usciti con codice 0, senza forzature o
residui, e hanno scritto screenshot e SRAM isolate valide da 16.576 byte.

La prima estensione non-Daytona ha usato il parent `stcc`. La prova massima ha
avviato nove istanze isolate con `Car 1`…`Car 8` e `Relay`, codifiche 1…9
verificate nei rispettivi `.srm`. Tutti i partecipanti hanno formato il roster
9/9 e tutti i processi sono terminati con codice 0, senza kill forzato né
residui. La prova conferma configurazione NVRAM, avvio del core e handshake
Netpacket al limite del gioco; non dimostra ancora una gara STCC sincronizzata.
Una prova distribuita a tre partecipanti ha mantenuto il roster con due cabinet
giocabili su macOS e il Relay su Batocera; il replay esistente si è però fermato
all'inserimento del nome sui cabinet giocabili, quindi non viene contato come
prova di gara sincronizzata.
Una seconda prova distribuita ha usato `Car 1` sul Mac e `Car 2` più `Relay` su
Windows, sempre con gli artifact CI `1d8a1ab`: l'host ha registrato prima 2/3 e
poi 3/3, e tutti i partecipanti hanno formato il roster completo. Le tre
catture, le SRAM separate e le uscite con codice 0 sono valide; nessuna istanza
è stata terminata forzatamente e non sono rimasti processi. Anche questa prova
qualifica interoperabilità e ruoli, non ancora una gara STCC sincronizzata.

La famiglia Sega Rally usa lo stesso schema combinato. La prova massima sul
parent ha avviato cinque istanze con `Car 1`…`Car 4` e `Relay`, codifiche 1…5
verificate nei rispettivi `.srm`; tutti i partecipanti hanno formato il roster
5/5 e tutti i processi sono terminati con codice 0, senza chiusure forzate o
residui. Le revisioni B e C erano già state verificate a due istanze.
`srallycdx` e `srallycdxa` sono esclusi perché i rispettivi
Service Menu acquisiti non espongono né `Cabinet Type` né `Link Type`. La
tabella dei set supportati non dichiara quindi `Linked Cabinets` per queste due
revisioni. Queste prove confermano configurazione NVRAM e handshake, non ancora
una gara Sega Rally sincronizzata.

Motor Raid è stato inoltre verificato con tre istanze `Master`, `Slave` e
`Live`. Tutti i partecipanti hanno formato il roster 3/3; gli `.srm` conservano
le codifiche native 1, 2 e 3 e gli ID 1, 2 e 3 in entrambe le copie EEPROM. I
tre processi sono terminati con codice 0, senza chiusure forzate né residui. Una
prova distribuita successiva ha usato Master e Slave su macOS e Live su Batocera:
il nodo remoto ha formato il roster 3/3 e mostrato la telecamera esterna della
gara. La topologia è stata ripetuta con entrambi gli host cablati e ha mantenuto
il roster 3/3 per due minuti senza errori di collegamento. Il gioco presenta il
ruolo speciale come Relay ID16, mentre il Service Menu conserva i valori NVRAM
documentati `Live` e `Cabinet ID=3`.

Super GT 24h e i tre set Over Rev sono stati verificati con due istanze isolate.
Tutti hanno formato il roster 2/2 sulle communication board, chiuso entrambi i
processi con codice 0 e lasciato zero processi residui. Gli SRM confermano i
ruoli e i conteggi descritti sopra, comprese le copie EEPROM speculari dei tre
Over Rev. Sono prove di avvio, configurazione NVRAM, polling della communication
board e handshake Netpacket; una gara sincronizzata resta nella sezione delle
verifiche.

Le prove a tre istanze su `manxtt` e `manxttc` hanno formato roster 3/3 su tutte
le communication board. I rispettivi SRM conservano `Master=1`, `Slave=2` e
`Relay=3`; tutti i processi sono terminati con codice 0, senza chiusure forzate
o residui. Nella prova distribuita con Master e Slave su macOS e Relay su
Batocera, il nodo remoto ha formato il roster 3/3 e mostrato esplicitamente
`THIS IS RELAY MACHINE`. Resta da verificare una gara con input reali.

The 2026-10-09 audit distinguishes this historical transport roster from a
completed native Relay handshake. Both `manxtt` and `manxttc` Relay remain
unqualified: shared role 3 differs from the simulation's role-0 predicate,
and a tested role-3 candidate still caused the game to reset the board. That
candidate was reverted. See the network package for the retained failure traces.

Virtual On è stato provato prima con due istanze `von` Master/Slave e poi con
tre istanze `von`, `von` e `vonr`. Entrambe le sessioni hanno formato il roster
completo su ogni partecipante; la seconda ha conservato negli `.srm` i valori
NVRAM `Master=1`, `Slave=0` e `No Link=2` del programma Relay. Tutti i processi
sono terminati con codice 0, senza chiusure forzate o residui. Questa prova
conferma avvio, persistenza e compatibilità Netpacket tra i due programmi. Una
prova distribuita ha poi eseguito i due programmi Twin su macOS e `vonr` su
Batocera: il Relay ha formato il roster 3/3 e mostrato il live monitor. La
topologia cablata ha mantenuto il roster completo per due minuti senza errori
di collegamento. Resta da verificare una partita con input reali.

Controllo dedicato senza ROM:

```sh
build-libretro-gpu/bin/sm2-libretro-netpacket-checks
```

Verifica anelli da 2, 3, 4, 8 e 9 nodi, ID e conteggio cabinet, ordine del roster,
consegna del payload al predecessore/successore, registrazione Netpacket,
affidabilità, configurazioni discordanti, disconnessione e rifiuto del decimo
partecipante.

Runner RetroArch isolato per tutti i set supportati da 2 a 9 partecipanti:

```sh
python3 scripts/test-retroarch-netpacket.py \
  --retroarch /percorso/RetroArch.app/Contents/MacOS/RetroArch \
  --core /percorso/sm2_libretro.dylib \
  --rom /percorso/roms/daytona.zip \
  --system-assets /percorso/system/sm2-emu \
  --automatic-network \
  --cabinets 3
```

Per STCC o Sega Rally sostituire la ROM e usare rispettivamente `--set-name
stcc` o `--set-name srallyc`.
`--automatic-network` selects each instance's reviewed native preset. Without
it, the runner explicitly disables the preset and writes the manual NVRAM
selectors, preserving the earlier test mode.

Il runner deriva ogni istanza da una copia della configurazione RetroArch
esistente, applica directory e porte isolate, conserva log e risultato JSON e
chiude soltanto i PID che ha creato. Se RetroArch è già aperto, non avvia il test.
Su macOS il runner dispone esplicitamente le sole finestre appartenenti ai PID
che ha avviato: per default sono affiancate su due colonne. Usare
`--window-columns N` per cambiare il numero di colonne oppure
`--window-layout cascade` per ripristinare la disposizione sovrapposta.

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
Le modalità di guida codificano stick e trigger come `RETRO_DEVICE_ANALOG` (5):
una sonda sul callback ha verificato R2 a 0 rilasciato e 32767 premuto; Motor
Raid ha quindi raggiunto 205 nella cattura di controllo.
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

Il 13 settembre 2026 RetroArch Nightly 1.22.2 ha verificato
`Automatic Initial NVRAM Setup` con VF2. Senza salvataggi, il log registra il
caricamento del campione prima del primo frame; il gioco raggiunge una partita,
produce audio stereo non silenzioso e salva Country=USA in un contenitore
valido. Una seconda esecuzione, avviata con un `.srm` Country=Japan già valido,
registra invece l'import del salvataggio e mantiene Japan. Entrambe le sessioni
terminano con exit code 0.

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

Il 15 settembre 2026 un avvio pulito di Super GT 24h in RetroArch Nightly
1.22.2 ha verificato il nuovo default specifico. Con `Automatic Initial NVRAM
Setup=Enabled` e `NVRAM Settings=Disabled`, il core ha applicato il campione
prima del primo frame e ha salvato I/O Type C (`0x00` all'offset backup RAM
`0x0a`) in un contenitore `.srm` con checksum valido. La sessione è terminata
con exit code 0 e ha prodotto audio non silenzioso e una schermata di gioco.

La verifica visiva successiva ha confrontato la sequenza attract dello
standalone con il core. Il solo `I/O Type C` e il solo `Country=USA` conservano
il logo Jaleco rotante, il titolo e la demo 3D. La scrittura di `Link Max=2`
insieme a `Link Type=NOT LINK` lasciava invece il gioco nella schermata di
controllo rete su fondo nero. `Link Max` viene ora applicato soltanto a `CAR NO1
Master`; tornando a `NOT LINK`, il byte interno viene riportato al valore
offline `1`. Un avvio pulito con `Automatic Initial NVRAM Setup=Enabled` e
`NVRAM Settings=Enabled` ha quindi mostrato l'intera sequenza Jaleco e la demo
3D mantenendo I/O Type C, Country USA, checksum e mirror EEPROM validi.

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

Virtua Striker (older) è stato verificato con il proprio menu ridotto: nove
Core Options, senza One Match Mode, e Advertise Sound agli offset 0x17/0x97.
Il primo avvio ha applicato valori non predefiniti a tutte le nove opzioni,
lasciando Time Set al default nativo 2:00; il gioco ha raggiunto una partita.
Il secondo avvio ha caricato lo stesso `.srm` con `NVRAM Settings=Disabled` e
ha conservato tutti i campi senza riapplicarli. Entrambe le sessioni sono
terminate con exit code 0; il marcatore fisso `0a 00` e le due banche da 128
byte identiche sono rimasti validi. Evidenze in
`build-libretro-gpu/validation/vstrikero-layout-20260919/summary.json`.

Sega Rally Championship Deluxe revision A è stato verificato su tutti i 14
campioni del menu ridotto. Il clone usa una EEPROM dichiarata da 44 byte e un
CRC-16/CCITT invertito sui successivi 42 byte; espone soltanto Advertise Sound,
Country, Game Difficulty e Game Mode. Il primo avvio ha applicato OFF, USA,
HARDEST e LONGEST ed è arrivato in gara, conservando invariati i byte delle
voci Cabinet Type e Link Type assenti. Il secondo avvio con `NVRAM
Settings=Disabled` ha caricato lo stesso `.srm`, mantenuto i quattro valori e
un CRC valido senza riapplicarli. Evidenze in
`build-libretro-gpu/validation/srallycdxa-layout-20260919/summary.json`.

## Rapporto d'aspetto

La Core Option Video `Aspect Ratio` offre `Auto` (default), `4:3` e `16:9`.
Le due scelte esplicite forzano soltanto il rapporto comunicato al frontend; il
framebuffer emulato resta 496×384 quando `Widescreen Mode` è Disabled.

In `Auto` il core legge il tipo di cabinato dalla EEPROM attiva e aggiorna la
geometria Libretro soltanto quando il valore cambia:

- famiglia Indy 500: `Twin` usa 4:3, `Deluxe` usa 16:9;
- famiglia Sega Touring Car Championship: `Twin` usa 4:3, `Deluxe` usa 16:9;
- Super GT 24h: I/O Type B usa 16:9, A/C usano 4:3;
- gli altri giochi usano 4:3.

La EEPROM, inclusi i cambiamenti salvati dal Service Menu, resta la fonte
autorevole. Le Core Options NVRAM e `Automatic Initial NVRAM Setup` modificano
la stessa EEPROM e `Auto` segue il valore risultante. La selezione deriva dal
layout widescreen documentato da MAME per Indy 500/STCC; l'upstream standalone
0.9.9 offre invece soltanto una regolazione generale 4:3/pixel
quadrati/riempimento e non riconosce il cabinato.

I controlli ABI con ROM reali hanno verificato Indy 500 Twin in 4:3, Indy 500
Deluxe in 16:9, entrambe le forzature manuali e STCC `Deluxe` in 16:9. In tutti i
casi risoluzione, frame e NVRAM restano invariati.

## A/V Timing e overlay diagnostico

Le Core Options Video includono `A/V Timing`, con le sole voci
`60 Hz (Default)` e `57.524 Hz (Native Model 2)`. Il valore `60hz`, anche
se già salvato in un file di opzioni, seleziona 60 Hz e rimane `60hz` al
successivo salvataggio. La modalità predefinita esegue un nuovo
frame macchina per ogni callback a 60 Hz, accelerando gioco e suono di circa
il 4,3%; ricampiona i campioni generati al rate audio dichiarato dalla scheda.
La frazione di campioni per callback è serializzata nei Save State. I campioni
non accettati dal frontend restano in coda e vengono recuperati nelle callback
successive. Native conserva la cadenza e l'audio della scheda senza ricampionare.
La precedente modalità a 60 Hz con frame duplicati non è più esposta.

Core Options V2 registers exactly those two labels with `60hz` as default;
the legacy options fallback orders the raw values `60hz|native`. A bounded VF2
direct-ABI run with no saved timing value completed 180 callbacks at 60 FPS,
zero duplicated video callbacks, 132,300 stereo audio frames at 44.1 kHz and a
deterministic Save State replay. An explicit saved `native` completed 180
callbacks at 57.524160 Hz with no video duplicates. A separate visual menu and
subjective A/V check in RetroArch remains open.

`Timing / FPS Overlay` riutilizza il pannello Dear ImGui del core Supermodel.
`Auto`, collocato subito dopo `Off`, seleziona il font base da 13 pixel; seguono
le dimensioni native da 11, 12, 13 o 14 pixel in ordine crescente.
La dimensione selezionata viene usata direttamente a 1× e adattata alle
risoluzioni interne superiori mantenendo le stesse proporzioni visive. Il pannello è
adattato alle fasi disponibili in SM2. Ogni 61 callback aggiorna una finestra
compatta disegnata direttamente nel frame con tempi medi di macchina, video e
audio/pacing, durata media e peggiore di `retro_run`, FPS effettivi e capacità
stimate di engine e callback. La stessa draw list viene composta nei percorsi
Software, Vulkan e OpenGL; il frontend non crea un proprio messaggio OSD.
L'opzione si aggiorna immediatamente; il cambio di timing richiede il
riavvio del contenuto perché modifica le informazioni A/V dichiarate al frontend.

On 2026-10-07, isolated VF2 replays in RetroArch completed 2300 callbacks
with nonzero PCM, valid SRAM and a gameplay screenshot in Software 60 Hz,
Vulkan 60 Hz, and Native Software modes. Vulkan used RetroArch Nightly 1.22.2
with the existing MoltenVK runtime. These runs establish frontend operation;
they do not measure subjective sound continuity or display pacing.
The current Linux x86_64 build also completed 2300 frames in an isolated
Batocera 43.1 RetroArch run at 60 Hz with OpenGL Core 4.6, a gameplay
screenshot, nonzero 44.1 kHz PCM, valid SRAM and exit code 0. The test used
the marked `/userdata/system/codex-core-tests/` area and left the installed
core and global configuration untouched. macOS OpenGL cannot supply the
required 4.3 functions, as documented in `GPU.md`.

## Enhancement dei renderer GPU

La categoria Video espone `3D Texture Filtering` con Faithful come default e
Anisotropic 2x, 4x, 8x o 16x, oltre a `2D Layer Upscaling Filter` con Faithful,
xBR e ScaleFX. Entrambe le opzioni si applicano immediatamente ai renderer
Vulkan e OpenGL; il percorso Software conserva la resa nativa.

Il filtro 3D campiona in modo aggiuntivo le texture sulle superfici oblique.
xBR e ScaleFX lavorano invece sulle tilemap 2D prima che siano composte con i
poligoni 3D. Sono quindi distinti dagli shader RetroArch, che ricevono e
filtrano il frame finale già composto. Faithful conserva il comportamento
precedente ed è il valore iniziale di entrambe le opzioni.

## Software Renderer Threading

Il renderer Software segue SM2-Emu 0.9.11 e usa per default `Asynchronous`:
il rasterizzatore lavora su un thread separato mentre la macchina prepara il
frame successivo. Questo sovrappone il lavoro CPU al costo dichiarato di un
frame di latenza. Su Linux big.LITTLE il rilevamento upstream assegna
automaticamente il rasterizzatore ai core efficienti quando ne trova almeno
due, lasciando i core più veloci all'emulazione.

La Core Option `Software Renderer Threading (Restart Required)` permette di
selezionare `Synchronous`, che restituisce il frame corrente senza la latenza
aggiuntiva. Il percorso Libretro usa buffer distinti per il frame in
elaborazione e quello consegnato al frontend; reset, caricamento degli stati e
unload attendono sempre il thread prima di modificare o distruggere la macchina.
Vulkan e OpenGL non sono interessati dall'opzione.

## Enhanced Audio Balance

The Audio category exposes **Enhanced Audio Balance**, **Enabled by default**.
With each source Gain set to **Auto** (also the default), the core applies the
current upstream game profile. Disabled makes Auto use unity gain. Manual Gain
percentages replace the profile for that source, while Mute silences its output.
Fixed hardware mixer routes, the SCSP master register and Music Volume remain
applicable. VF2 now uses upstream's flat profile; the previous slot-based
music/effects/announcer/voice rebalance is removed. See
`Docs/UPSTREAM_AUDIO_BALANCE_PACKAGE_2026-10-08.md` for exact imports and tests.

`Music Volume` è un controllo globale separato, modellato sull'opzione del core
Supermodel. Offre 0–200% a passi di 10 con default 100% e scala esclusivamente
l'audio MPEG prodotto dalle schede DSB/DSB2 prima che venga sommato all'SCSP.
Interessa le famiglie STCC e Top Skater; effetti, voci e giochi privi di una
scheda musicale separata restano invariati. La variazione è immediata.

The Audio menu uses the TGPulse source Gain convention: Auto (default), Mute,
then 10% through 100% in steps of 10. Auto is stored as `auto`, Mute as `0`,
and percentages as bare numbers. Source labels follow the shared convention: `MultiPCM 1
Gain`, `MultiPCM 2 Gain`, `FM (YM3438) Gain`
and `DSB (MPEG) Gain`; Model 2 also has `SCSP Gain`.
Auto and manual 100% are distinct: Auto follows the enabled upstream profile,
while manual 100% uses the source's unprofiled reference level. Manual numeric
values remain readable, including saved 110–200% and textual mute values; only
an active legacy value is temporarily added to the selector. New configurations
show the canonical choices. Configuration files remain frontend-owned.
These controls scale output while chips, timers and voices keep advancing.
Music Volume and DSB Gain both affect the separate MPEG board. The optional
SCSP output low-pass filter remains independent of these level controls.
Subjective listening checks remain open.

On 2026-10-07, a separate RetroArch 1.21.0 replay of Virtua Cop with
`sm2_multipcm1_gain = "0"` completed 2300 frames using a split device BIOS.
The 0–10 second PCM segment fell from 1270.8 RMS at the default 100% to zero;
the 10–20 second segment fell from 2213.5 to 1030.4 RMS and remained nonzero.
This verifies the output control on the original Model 2 sound board without
claiming a subjective listening result. Setting MultiPCM 1 on VF2 made no
change, as expected because VF2 uses the SCSP board; its menu must hide that
source. The isolated runner accepts repeatable `--source-gain SOURCE=PERCENT`.

A separate isolated RetroArch Nightly 1.22.2 menu inspection loaded Virtua
Cop with MultiPCM 1 initially at `0`. The Audio category showed only the
three present sources (MultiPCM 1, MultiPCM 2 and YM3438), displayed `Mute`
for MultiPCM 1, and ordered its choices as Mute, 10%, 20%, and onward. A
live menu selection of 100% was shown immediately and persisted as `100` in
the isolated per-core `.opt` file; the game resumed and the runner exited
normally. This verifies frontend selection and persistence, while listening
quality remains for a manual check.

Due replay isolati in RetroArch macOS hanno verificato entrambe le schede. Su
STCC il picco PCM è passato da 21073 a 100% a 10087 a 0%; su Top Skater da
32768 a 10243. In entrambi i casi l'audio SCSP è rimasto non silenzioso a 0%,
confermando che il controllo agisce sul solo contributo MPEG. La saturazione
osservata in Top Skater a 100% appartiene al mix preesistente e non è introdotta
dal nuovo guadagno.

Il fix hardware dello stesso aggiornamento upstream è indipendente dall'opzione:
`overrevb` e `overrevba` completano la precedente scrittura INTENA differita
prima del successivo acknowledge del timer audio, evitando che il suono si
blocchi. Il parent `overrev`, basato su Model 2C, non usa questa eccezione.

Con input identici, 1800 frame di VF2 hanno prodotto video e NVRAM identici ma
PCM differenti tra Enabled e Disabled. La traccia Enhanced ha raggiunto il
limite su 16 campioni di circa 2,76 milioni; Disabled ha raggiunto 23332 senza
saturazione. Le tre versioni di Over Rev hanno mantenuto audio non silenzioso
fino agli ultimi dieci secondi di 2300 frame, senza campioni saturati.

## Gamepad Rumble

La categoria Input espone `Gamepad Rumble`, Enabled per default.
The shared decoder now matches upstream `868dce7`, including its prerequisite
Daytona and Sega Rally corrections. It preserves every drive-board write in
frame order and distinguishes Daytona, Indy, STCC and Sega Rally protocols.
Indy 500 and Over Rev parents/clones use the later Indy board; parameter bytes
retain the preceding effect, while step-zero pushes release it. Daytona's
step-zero push remains a real force, motor-off uses `0x1x`, and `0x4x` is a
center-relative Kick. Noncontinuous Push/Kick effects produce brief strong
motor jolts with the weak motor at half intensity; steering deflection adds
lighter synthesized cornering vibration. The existing 12-frame rumble tail
remains, so release stops renewing an impact rather than cutting its tail.
Over Rev drive-board effects require **HARDWARE TYPE = SEGA STCC** in its
operator menu or NVRAM Settings. Since 2026-10-09, SEGA STCC is the core's
Hardware Type default and Automatic Initial NVRAM Setup override for Over Rev
and its two clones. Automatic setup changes only newly seeded NVRAM; existing
frontend/native saves retain precedence, and an explicitly saved Hardware Type
remains selectable and respected when NVRAM Settings is enabled. The acquired
native template still records NORMAL (2in1). See
`Docs/OVER_REV_HARDWARE_DEFAULT_2026-10-09.md` for verification.
Synthesized cornering vibration is independent of drive-board effects. Gli altri giochi
restano silenziosi. Il core genera questi rapporti sulla scala normalizzata;
`Input Rumble Gain` di RetroArch è l'unico controllo dell'intensità complessiva.

La variazione delle opzioni è immediata. Disattivazione, reset, unload ed errori
azzerano entrambi i motori, evitando vibrazioni bloccate. L'interfaccia rumble
non rappresenta la forza direzionale di un volante e non gestisce il recoil
fisico delle lightgun; il recoil `evdev` è annotato nella roadmap come possibile
integrazione futura.

## Save State Libretro

The core implements `retro_serialize_size`, `retro_serialize` and
`retro_unserialize` using frontend-neutral machine serialization. RetroArch owns
slots and state files; the core does not import the standalone state UI.

The declared buffer is fixed at **9 MiB (9,437,184 bytes)** and available before
content load. It includes a separate **96-byte frontend tail**. The machine
currently writes **format 10** and reads **formats 4 through 10**. Loading a
legacy file does not rewrite it; the next successful save uses format 10.
Older cores reject formats they do not support. Actual RetroArch files may be
larger because the frontend adds its own wrapper and optional replay data.

Magic, supported version, game identity and board are checked before applying
the machine image. Truncated/invalid machine payloads trigger rollback to the
previous snapshot. Compatibility preserves these version-specific rules:

- Format 4: convert the published sound scheduler fields when reading.
- Format 6: preserve SCSP filter history and DSB2 playback/loop state; old
  missing history uses the documented fallback. The frontend tail also keeps
  output DC-filter history.
- Format 7: preserve persistent geometry registers and RAM. Older snapshots
  cannot reconstruct geometry data absent from their archives.
- Format 8: preserve pending local communication frames and overflow. Loading
  older snapshots clears future queued frames; their original queue is absent.
- Format 9: preserve the selected sound ROM bank, optional Deluxe Model 1 audio
  board and resampler history. Missing ROMs required by a snapshot cause a
  rejected load and rollback. Older Manx TT states restore their original set
  sound bank; the new cabinet-audio policy is reapplied on Resume, resetting
  audio if it requires a different bank. They contain no supplemental-board
  history, so sample-exact legacy Manx TT audio continuation is not promised.

The core declares platform and endianness dependence through
`RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS`. Older frontends may ignore that
negotiation and retain the standard callbacks. Cross-platform state-file
portability is not promised. Musashi host pointers are rebound on loading;
raw state-file equality across processes is not a semantic comparison.

After a load, derived renderer resources and queued audio are invalidated and
rumble is stopped. Cadence, audio delivery remainder, input runtime,
gun-flash state and output DC history are restored from the frontend tail.
Save/load is refused while an external **Linked Cabinets** session is active:
the local machine archive cannot restore the peer Netpacket session.

The retained package checks cover all four boards plus DSB2, format-8
round-trip/replay, corrupt input rejection, run-ahead/rewind ABI patterns and
fresh-process reads of formats 4–7. The 2026-10-09 closure adds an actual
Virtua Cop slot round-trip in RetroArch Nightly 1.22.2 on the current installed
macOS ARM64 core: exact restored game region, visible advancement/resumption
and clean exit. The established `scripts/test-retroarch-savestate.py` runner
verifies pause state, waits for screenshot task completion, and uses three
comparison steps for geometry cadence and asynchronous presentation latency.

Historical Batocera/other-platform results retain their own artifact/date
scope; they are not a new frontend qualification of the latest binary.
See [Save State Compatibility Review](Docs/SAVE_STATE_COMPATIBILITY_REVIEW_2026-10-09.md)
for the evidence matrix, format transitions and qualification limits.

### Cross-Session 68000 Save States (2026-10-08)

Upstream fix `1a60aff` rebinds Musashi's host pointers when loading a state from
another process. The layout is unchanged. Old and new state files pass fresh
process loads on all four boards plus DSB2, with deterministic replay,
run-ahead and rewind. Use `scripts/smoke-libretro.py --state-output PATH` in a
producer process and `--state-input PATH` in a later process. See
[the package report](Docs/UPSTREAM_SAVE_STATE_PACKAGE_2026-10-08.md) for exact
revisions, comparisons and platform limits. The VF2 frontend slot round-trip also passes on the installed macOS core:
exact cropped-frame restoration after two controlled frame steps, subsequent
visible advancement, and clean exit. The runner waits for the screenshot task
to drain before requesting loading; the report documents the diagnosed race.

## Limiti

I cheat non sono implementati. Save State, rewind e run-ahead sono qualificati
localmente sulle quattro board; le prove multipiattaforma restano separate.
Renderer Vulkan/OpenGL e Core Options Video sono
disponibili nella build descritta in [GPU.md](GPU.md); la geometria nativa
software resta fissa. Il collegamento Netpacket è disponibile per tutte le
famiglie censite: Daytona, STCC, Sega Rally, Indy 500, Motor Raid, Wave Runner,
Sega Ski Super G, Super GT 24h, Over Rev, Manx TT e Virtual On. Daytona a due
cabinet è stato provato in una gara con input registrati; le altre prove reali
coprono roster da 2 a 9 partecipanti, compresi i ruoli Relay/Live di STCC, Sega
Rally, Motor Raid, Manx TT e Virtual On. Motor Raid DX salva i valori corretti e
stabilisce la connessione frontend 2/2, ma non elabora la communication board,
coerentemente con il suo stato upstream non funzionante. Le sessioni distribuite
macOS/Batocera hanno verificato il roster Daytona 2/2 e i ruoli Relay/Live 3/3
di STCC, Sega Rally, Motor Raid, Manx TT e Virtual On. Una campagna aggiuntiva
con entrambi gli host cablati ha mantenuto per due minuti i roster di Daytona,
STCC, Motor Raid e Virtual On, senza errori di collegamento. Il controllo visivo
dell'utente ha inoltre confermato la corretta uscita delle postazioni Relay/Live.
Gli artifact CI macOS arm64 e Windows x86_64 della revisione `1d8a1ab` hanno
inoltre completato una gara Daytona 2/2 e un roster STCC 3/3 distribuiti fra i
due sistemi. Restano da provare le altre gare sincronizzate indicate nella
roadmap e i controller fisici.

Una verifica Daytona in gara su Windows ha confermato lo stipple a scacchiera
previsto dalla traslucenza Model 2. La dominante bluastra osservata sul
televisore proveniva dalla modalità Luce notturna di Windows: gli screenshot
digitali conservano il retino nero e non è emersa alcuna anomalia del core o dei
backend grafici.

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
dalla piattaforma Model 2 condivisa con MAME. `rascot2` resta fuori dalle Core
Options NVRAM perché la procedura acquisita consente l’avvio locale ma non
espone un normale menu Game Settings.

La prova di gameplay usa input sintetici attraverso RetroArch: non convalida
un controller fisico. CoreAudio e PCM registrato confermano il percorso audio;
qualità percepita e ascolto manuale restano da verificare. Non è una matrice di
compatibilità completa né una misura delle prestazioni su altre piattaforme.

La Nightly installata 1.22.2 ha prodotto un'immagine, ma ha mostrato problemi
nel ciclo di avvio da CLI; la validazione conclusiva usa la Stable indicata.
Un primo replay incompleto sulla Stable terminava subito e faceva fallire la
cattura finale del frontend: il test consegnato include tutti gli stati input.


## Power Sled Linked Communication — 2026-10-08

For a current three-instance launch:

```sh
python3 scripts/launch-linked-cabinets.py \
  --game powsledm:1,powsled:2,powsledr:r
```

All instances use Linked Cabinets = 3. Master and Slave use Silver and Red;
Relay retains its fixed role. ANRS works independently of NVRAM Settings.
The launcher supports `name[:ANRS[:GAMEPAD]]`, reuses independent saves and
options, and reapplies resolved role/color IDs. See
[the launcher guide](Docs/LINKED_CABINET_LAUNCHER.md) for current behavior.

The existing isolated runner accepts:

```sh
python3 scripts/test-retroarch-netpacket.py \
  --retroarch /path/to/RetroArch --core /path/to/sm2_libretro.dylib \
  --rom /path/to/powsledm.zip --relay-rom /path/to/powsledr.zip \
  --slave-rom /path/to/powsled.zip --set-name powsledm --cabinets 3 \
  --system-assets /path/to/system/sm2-emu --settle 20 \
  --service-menu --capture-screenshots --output /path/to/new-result-directory
```

The example above explicitly requests an operator-menu inspection; it is not
the normal gameplay launcher. `--automatic-network` can now seed the reviewed
Main Silver / Slave Red colors. `--service-menu` visits Test Menu and Game
Assignments without changing values. Power Sled success requires both the
frontend roster and native board link IDs Main 1/3, Relay program 3/3, Slave 2/3.
Screenshots still require visual review; process success alone is not menu or
gameplay proof. See `Docs/POWER_SLED_SERVICE_MENU_CAMPAIGN.md` for the evidence.


## Manx TT Cabinet Audio — 2026-10-10

Audio exposes **Manx TT Cabinet Audio**, Enabled / Disabled (Enabled default),
always visible and a no-op outside Manx TT. Revision C/D uses validated current
Cabinet Type from NVRAM to select Twin or Deluxe SCSP ROMs. Required sound chips
are CRC checked in the loaded archive and adjacent Manx TT archives; only audio
regions are imported. Missing alternate ROMs retain the selected set audio with
an explanatory notification. Reload content after adding donor archives.

The additional existing M1 sound-board emulation follows Deluxe on Revision
C/D and is always connected for `manxttdx` when its ROMs are present. It reuses
the host UART, native clocks and chip Gain controls, with exact-rate output
resampling and Save State format-9 persistence. Disabling cabinet audio keeps
the original SCSP ROMs; the additional board still follows the cabinet.
Production Deluxe now includes an AMS acknowledgement stub on parallel D/E:
Revision C/D activates it when native NVRAM Cabinet Type is Deluxe; Twin retains
its original P2 input. The Deluxe-only set always activates it. Reset and cabinet
transitions clear its response; format 10 preserves an incomplete handshake.
Formats 4–9 remain readable, with absent AMS state initialized to idle.
This passes the native boot gate; motors and position feedback are not simulated.
Both local installed core copies are updated. See
[implementation and validation](Docs/MANX_TT_CABINET_AUDIO_2026-10-10.md).


## Upstream CPU And SCSP Package — 2026-10-10

Imported `760a556` direct instruction/program fetch and coprocessor wait skipping,
plus `ec9c3ac` SCSP pitch depth-index-3 correction. No new menu values or defaults;
Gain, EAB, filters, input and NVRAM policies keep their existing conventions.
State format remains 10 and previous format-10 states continue on all four boards.

Pristine before/after comparisons pass for Daytona, VF2, Sega Rally and HOTD.
The SCSP change affects only depth 3 in the isolated synthesis reference fixture;
Sega Rally's changed boot PCM matches pristine upstream. VF2 reaches gameplay in
RetroArch Nightly 1.22.2. Both local installed core copies are updated and verified.
See `Docs/UPSTREAM_CPU_SCSP_PACKAGE_2026-10-10.md` for exact revisions, measured
process-time improvement, reproducible evidence and local qualification limits.

## Daytona GTX Support — 2026-10-12

The catalog now includes `daytonagtx` (Daytona USA, GTX 2004 Edition), with
MAME-derived banked protection and Daytona's established driving profile.
GTX-specific Service Menu/NVRAM documentation is pending; no unverified parent
operator overrides or initial template are supplied. Save RAM still persists
normally. The native code-entry screen permits Gas to cancel. An isolated
RetroArch Vulkan 1x run reaches a race with audio/SRAM and clean exit; full-course
completion, networking and physical feedback remain unqualified. See
[GTX Support](Docs/DAYTONA_GTX_SUPPORT_2026-10-12.md).

### Controller Profile Menu Refresh Correction — 2026-10-12

The first test covered republishing after Resume. A user report clarified that
the Controls name stayed stale while remaining in the Quick Menu. The modern
Core Options display callback now republishes the controller type/descriptors
when the global selector changes, even while the machine is paused. A separate
isolated RetroArch 1.22.2 VF2 run verifies Normal in Port 1 Controls without
Resume or restart; the process exits 0. Legacy frontends without this callback
retain the return-to-game refresh. Gameplay polling still follows the selected
option when emulation runs; no machine reset or state change is introduced.
Private evidence: `build-libretro-gpu/validation/controller-profile-type-20261012/menu-refresh/`.


## Widescreen Mode

Video → Widescreen Mode offers Disabled (default) and Expand 3D View Only.
Disabled preserves Aspect Ratio/Auto; selecting 16:9 stretches the original
image. Expand directly selects a 16:9 presentation and renders more 3D scenery
on Vulkan/OpenGL/GLES, with Aspect Ratio set to Auto or 16:9; forced 4:3 suppresses expansion. HUD proportions
are preserved and background side fill is automatic. Disabling it restores the
saved Aspect Ratio selection. Native-capable Indy 500/STCC/Super GT 24h families
and Software keep their existing behavior. Changes apply on return to game
without resetting. Expansion is experimental; individual games can expose
scenery missing from the original picture. See
`Docs/WIDESCREEN_MODE_INTEGRATION_2026-10-18.md`.

## Custom Crosshairs And Inactivity — 2026-10-19

Input → **Crosshair Style** now offers Auto, SM2-Emu, Supermodel and Custom.
The saved `sm2` / `supermodel` values and SM2-Emu default remain compatible.
Auto uses custom artwork per player when available, falling back to SM2-Emu.
Custom draws only applicable PNGs; unavailable or invalid artwork is reported
once for each selected player. **Show Crosshair** still controls visibility,
including Automatic's native-reticle policy for positional-gun games.

Store user PNGs under `<frontend system directory>/sm2-emu/crosshairs/`.
Search order for each player is game-player, game, parent-player, parent,
default-player, default: for example `vcop-p1.png`, `vcop.png`, then the
corresponding parent names, `default-p1.png` and `default.png`. PNGs may contain
transparency and must be at most 1024×1024. Like upstream, their dimensions are
relative to a 1080-line image and scale with the internal raster. Reload content
after editing files, or return to game with a built-in style before selecting
Auto/Custom again. Files are cached, with no repeated disk lookup every frame.
No crosshair artwork is bundled.

**Crosshair Inactivity Timeout** offers Disabled (default), 1–8, 10, 15, 30 or
60 seconds. With a timeout enabled, unused players' external reticles remain
hidden; aim movement, firing or another gun action shows the corresponding
reticle and restarts its timer. The timer uses emulated time, so pausing does
not consume it and fast-forward advances it with the game. It never changes
the game's coordinates, firing/reload behavior or built-in reticles. Reset
clears activity; presentation timers and user PNGs are not Save State payload.
Software and GPU paths share clipped geometry and PNG alpha composition.
See [verification and limits](Docs/CROSSHAIR_CUSTOM_AND_TIMEOUT_2026-10-19.md).

## MAME Audio Corrections And Save States — 2026-10-19

The local development core includes SCSP MIDI FIFO/status and byte-access fixes,
MultiPCM envelope/AM register corrections and an exponential note attack.
These are chip-emulation corrections, with no new option or change to frontend
volume/gain conventions. YM3413 external effects are excluded following the
review of supported hardware. Dead or Alive's real protection is integrated below.

New Save States use format 14. Formats 4–13 remain readable for other games;
legacy Dead or Alive states are rejected because their uploaded protection
tables are missing. The frontend container stays 9 MiB; older builds cannot
read newly created states. Existing
NVRAM is unchanged. Corrected synthesis can change audio after loading an old
state. See [scope and verification](Docs/MAME_AUDIO_INTEGRATION_2026-10-19.md).

## Dead Or Alive Protection — 2026-10-19

All five Model 2A/2B revisions use the MAME-derived 317-0229 transformation and
programmable decompression path. Existing ROMs supply the data and tables;
NVRAM and input profiles are unchanged. No extra option is required.

Save States created before this integration cannot restore the missing decoder
tables and are rejected before changing the machine. RetroArch displays:
“Dead or Alive: this Save State predates the protection update and cannot be
loaded. Start a new game.” Current states preserve uploads, buffered bits and
the initial discarded read. See [implementation and evidence](Docs/DOA_PROTECTION_REVIEW_2026-10-19.md).
