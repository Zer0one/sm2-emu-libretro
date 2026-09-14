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

Il core usa direttamente la directory save fornita dal frontend, che può già
essere specifica per `SM2-Emu` o per il contenuto. RetroArch gestisce qui
`<gioco>.srm`; eventuali immagini standalone `<gioco>.nv` e
`<gioco>.eeprom` da importare vanno nella stessa directory. Il core non crea
ulteriori sottocartelle `sm2-emu/<gioco>`.

## Profili digitali

Il core riconosce esplicitamente tutti gli 83 set parent/clone revisionati
attraverso i profili completati finora. Le descrizioni e i binding sono
quelli approvati in `Docs/revisione_profili_model2.xlsx`; firme sconosciute o
cambiate restano nel fallback conservativo. Ogni profilo registra due tipi di
dispositivo su entrambe le porte: quello predefinito con slot Test/Service
rimappabili e la variante senza i due slot. Poiché Model 2 dispone soltanto
delle linee `Service A` e `Test A`, L3/R3 del Player 2 sono binding secondari
virtuali delle stesse linee, non comandi B distinti.

I controlli ROM-free verificano tutti i set riconosciuti, le stringhe dei
profili e delle azioni, P1/P2 indipendenti, le due varianti del dispositivo e
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
per `Pitch Left`/`Pitch Right`. `Sega Water Ski Slide Axis Mode` usa
`Inverted` come default (`X+` produce `00`, `X-` produce `FF`); `Normal`
conserva la polarità del frontend. L'opzione è sempre visibile e si applica
immediatamente. Start condivide la linea hardware di `Select Down`, come indica
il descrittore `Start / Select Down`. La ROM parent ha raggiunto una gara in
RetroArch macOS con audio e Save RAM validi.

Sega Ski Super G espone `Special: Ski Super G`. D-Pad Up/Down sono `Zoom In` e
`Zoom Out`, L1/R1 sono i due Foot Sensor attivi alti e South/East/West sono
`Select 2`/`Select 3`/`Select 1`. Right Analog X controlla `Inclining` sul
canale Model 2 0 e Left Analog X controlla `Swing` sul canale Model 2 1 con
`Sega Ski Super G Swing Axis Mode` impostato di default su `Inverted`: X+
produce `00`, X- produce `FF`. `Normal` conserva invece la polarità del
frontend. L'opzione è sempre visibile e si applica immediatamente. Il
profilo e la Save RAM sono validi, ma la partita si
ferma su `DRIVE BOARD TROUBLE CODE: FF`: il set è già marcato preliminary e il
database SM2 non contiene la ROM drive-board presente nella definizione MAME.
Questo limite appartiene all'emulazione comune, non all'adattatore di input.

Le quattro revisioni Top Skater condividono `Special: Top Skater`. D-Pad
Left/Right corrispondono a `Select Left`/`Select Right`, South/East a `Jump
Front`/`Jump Tail`, Left Analog X a `Curving` e Right Analog X a `Slide`.
`Top Skater Curving Axis Mode` usa `Inverted` come default (`X+` produce `00`,
`X-` produce `FF`); `Normal` conserva la polarità del frontend. L'opzione è
sempre visibile, si applica immediatamente e non modifica Slide.
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

Air Walkers espone P1 e P2 con `Joystick (Standard): Basketball (Air Walkers)`,
D-Pad e `Button 1`/`Button 2`/`Button 3` nelle posizioni approvate. Coin 2,
Start 2 e il gameplay P2 raggiungono rispettivamente le linee comuni e `IN2`.
Il parent ha raggiunto una partita reale con audio e Save RAM validi. Il
multiplexing P3/P4 documentato da MAME resta un aggiornamento futuro del backend
I/O.

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
Up come prescritto dal foglio. Accelerator e Brake conservano la polarità
invertita dichiarata dai metadata, senza correzioni aggiuntive nel binding.
MAME segnala per questo gioco problemi analogici ancora aperti: sterzo che non
si centra e pedali che pulsano invece di mantenere un valore stabile. D-Pad
Down non è esposto. Anche questo profilo
ha le varianti con e senza Test/Service. La ROM parent ha raggiunto la selezione
del circuito in RetroArch macOS con audio e Save RAM validi.

I tre set Manx TT espongono `Driving: Sequential (Manx TT Superbike)`. `Bank`
usa lo stick sinistro X con la polarità invertita dichiarata dal gioco; Brake e
Accelerator usano esclusivamente i trigger analogici L2/R2 e il cambio usa
L1/R1. Start conserva l'azione combinata `Start / VR`; il D-Pad non espone
azioni. `Cabinet Type` usa `Twin` come default sia nelle Core Options sia nel
setup NVRAM del primo avvio. Le varianti con e senza Test/Service sono
disponibili. La ROM parent ha
raggiunto il controllo iniziale del motion slider con audio e Save RAM validi.

I due set Motor Raid espongono `Driving: Sequential (Motor Raid)`. Mantengono
Bank, pedali, cambio e `Start / VR` nelle stesse posizioni del foglio e
aggiungono `Kick` su South e `Punch` su East. Punch/Shift Up e Kick/Shift Down
raggiungono rispettivamente gli stessi due ingressi arcade, come previsto dal
cablaggio Motor Raid. La ROM parent ha raggiunto una gara in RetroArch macOS;
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
grilletto e clic sinistro sono `Shot`. BEL assegna `Missile` a East/LB, clic destro,
Lightgun Aux A e Reload; per BEL Reload è soltanto un secondo binding della stessa azione.

Il core conserva le due interfacce arcade. Virtua Cop 1/2 e House of the Dead
ricevono coordinate seriali RS-422 a 10 bit; East o LB, clic destro o il comando
Lightgun Reload eseguono `Reload Offscreen` portando la mira fuori dall'area
calibrata e premendo Shot. La Core Option `Off-Screen Reload Shortcut`, sempre
visibile e attiva per default, abilita o disabilita insieme i tre binding nei
giochi supportati. Il grilletto di una lightgun fisicamente puntata fuori schermo resta
sempre un gesto nativo di ricarica. Gunblade NY, Rail Chase 2 e BEL ricevono invece
coordinate posizionali a 8 bit nel mux analogico e non espongono Reload
Offscreen. House of the Dead conserva inoltre il diverso bit hardware del
grilletto P2. BEL mantiene Shot/Missile separati per P1/P2 e traduce le linee
Test/Service scambiate dal cabinet in `Service A`/`Test A`.

BEL dipende inoltre da quattro coppie centro/ampiezza e dai corrispondenti
parametri temporanei in work RAM. Il workaround upstream riconosce sia lo stato
non calibrato sia il preset distribuito da `Automatic Initial NVRAM Setup`, così
il preset non impedisce l'inizializzazione dei parametri temporanei quando manca
l'handshake della gun board. Un salvataggio già calibrato è stato provato in
RetroArch macOS: gli assi cambiano nel Service Menu e il mirino del gioco si
muove correttamente sia in orizzontale sia in verticale.

La Core Option `Show Crosshair`, ripresa da Supermodel, seleziona `Disabled`,
`Player 1 Only`, `Player 2 Only` oppure `Players 1 & 2` ed è disabilitata per
default. La crosshair vettoriale rossa di P1 e verde di P2 usa lo stesso cursore
virtuale condiviso da Lightgun, Mouse e Analog Stick, funziona nei renderer
Software, Vulkan e OpenGL e scompare durante la ricarica fuori schermo. Come
nel core Supermodel, si applica sia ai giochi con pistola seriale sia a Gunblade
NY, Rail Chase 2 e Behind Enemy Lines, che usano gli assi posizionali.

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
  parent verificati, quando non esistono un `.srm` valido o file nativi
  `.nv`/`.eeprom` validi, il core carica il campione completo ricavato dal
  Service Menu prima del primo frame. Imposta quindi Country/Nation su USA,
  oppure Export se USA non è disponibile, e i valori offline necessari per
  evitare attese di cabinet collegati. Come scelta specifica del core, la
  famiglia Daytona riceve inoltre Cabinet=Deluxe. Il setup automatico non sostituisce i
  salvataggi esistenti; le normali scritture del gioco continuano a persistere.
  Eliminando i dati di salvataggio del gioco si rigenera il setup.
- Core Option v2 generale `NVRAM Settings`, Disabled per default come nel core
  Supermodel. Quando è Enabled mostra soltanto le opzioni del parent caricato e
  applica tutti i valori scelti all'avvio. Non usa `Keep Current`: disabilitando
  l'opzione generale il core lascia invariati i campi NVRAM.
- Core Option `Gun Input Mode` con gli stessi cinque percorsi di Supermodel:
  Standard, Lightgun, Mouse + Analog Stick, Mouse e Analog Stick.
- Core Option `Off-Screen Reload Shortcut`, per Virtua Cop 1/2 e House of the
  Dead: RetroPad East/LB, Mouse destro e Lightgun Reload; attiva per default.
- Core Option `Show Crosshair`, disabilitata per default, con selezione P1, P2
  o entrambi e composizione coerente nei percorsi Software, Vulkan e OpenGL.
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
