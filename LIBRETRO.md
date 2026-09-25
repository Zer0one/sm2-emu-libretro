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

La Core Option System `ROM CRC Verification (Restart Required)` è abilitata
per default e confronta ogni chip con il CRC32 dichiarato in `games.xml`.
Disabilitandola, il loader cerca i chip soltanto per nome; l'integrità interna
ZIP/7z resta comunque verificata durante l'estrazione. Questa modalità è
destinata esclusivamente alla diagnostica: ROM errate, modificate o
incompatibili possono produrre errori di emulazione, crash o dati di
salvataggio corrotti.

Nell'installazione macOS di sviluppo usata per questo progetto, la copia
autorevole del database è:

`/Volumes/MacBook External Drive/Retro/_Media/_Multi (RetroArch)/system/sm2-emu/games.xml`

Ogni build destinata alle prove locali deve aggiornare quel file insieme al
core installato in RetroArch.

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
per `Pitch Left`/`Pitch Right`. Slide applica direttamente la polarità invertita
dichiarata nei metadata (`X+` produce `00`, `X-` produce `FF`). Start condivide
la linea hardware di `Select Down`, come indica il descrittore `Start / Select
Down`. La ROM parent ha raggiunto una gara in RetroArch macOS con audio e Save
RAM validi.

Sega Ski Super G espone `Special: Ski Super G`. D-Pad Up/Down sono `Zoom In` e
`Zoom Out`, L1/R1 sono i due Foot Sensor attivi alti e South/East/West sono
`Select 2`/`Select 3`/`Select 1`. Right Analog X controlla `Inclining` sul
canale Model 2 0 e Left Analog X controlla `Swing` sul canale Model 2 1. Swing
applica la polarità invertita dichiarata nei metadata: X+ produce `00` e X-
produce `FF`. L'emulazione comune non risponde al controllo della Drive Board
esterna e il gioco si ferma su `DRIVE BOARD TROUBLE CODE: FF`. La Core Option
specifica `Drive Board Error Bypass (Restart Required)`, disabilitata per
default, riproduce una singola pressione di Test quando compare quel messaggio.
Con il bypass abilitato la ROM parent ha superato l'errore ed è arrivata alla
sequenza sciabile in RetroArch macOS; con il bypass disabilitato conserva il
comportamento upstream. La funzione non emula la Drive Board e non modifica ROM,
NVRAM o binding.

Le quattro revisioni Top Skater condividono `Special: Top Skater`. D-Pad
Left/Right corrispondono a `Select Left`/`Select Right`, South/East a `Jump
Tail`/`Jump Front`, Left Analog X a `Curving` e Right Analog X a `Slide`.
Curving applica direttamente la polarità invertita dichiarata nei metadata
(`X+` produce `00`, `X-` produce `FF`); Slide resta invariato.
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
grilletto e clic sinistro sono `Shot`. BEL assegna `Missile` a East/LB, clic destro,
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

La Core Option System `Linked Cabinets (Restart Required)`, collocata prima di
`NVRAM Settings`, offre `Disabled`, predefinito, e valori da `2 Cabinets` a
`8 Cabinets` per Daytona e Indy 500; STCC arriva a `9 Cabinets` includendo il
Relay e Sega Rally arriva a `5 Cabinets` includendo il Relay. Motor Raid,
Wave Runner, Sega Ski Super G, Super GT 24h e Over Rev si fermano a `4 Cabinets`;
Manx TT e Virtual On offrono `2 Cabinets` e `3 Cabinets`.
`daytona93` è escluso perché il suo menu acquisito non contiene `Link ID` né
`Car Number`. L'opzione è specifica per
contenuto: appare soltanto sui set supportati, così il valore non si propaga a
contenuti incompatibili. Tutte le dimensioni usano
esclusivamente l'interfaccia ufficiale Libretro Netpacket. Il core non apre socket:
RetroArch gestisce host, client e rete, mentre `M2Comm` conserva
il protocollo della communication board Model 2. La struttura di `M2Comm`, il
possesso del trasporto e il loopback sono quelli dello standalone upstream
0.9.7; il core sostituisce il trasporto UDP con l'adattatore Netpacket e attende
che il roster contenga tutti i cabinet configurati. Handshake, conteggio atteso
e roster ordinato seguono la struttura del core Supermodel; i frame Model 2
restano completi e vengono inoltrati al successore dell'anello.

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
  --cabinets 3
```

Per STCC o Sega Rally sostituire la ROM e usare rispettivamente `--set-name
stcc` o `--set-name srallyc`.

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
framebuffer emulato resta 496×384.

In `Auto` il core legge il tipo di cabinato dalla EEPROM attiva e aggiorna la
geometria Libretro soltanto quando il valore cambia:

- famiglia Indy 500: `Twin` usa 4:3, `Deluxe` usa 16:9;
- famiglia Sega Touring Car Championship: `Twin` usa 4:3, `Deluxe` usa 16:9;
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

Le Core Options Video includono `A/V Timing`, con `Native (57.524160 Hz)` come
default e `60 Hz Compatibility` come alternativa. La seconda non esegue un
frame macchina per ogni callback a 60 Hz, perché ciò accelererebbe il gioco:
mantiene la cadenza hardware e duplica periodicamente l'ultimo frame. L'audio
resta al sample rate effettivo della scheda e viene distribuito su pacchetti da
60 Hz; eventuali campioni non accettati o non ancora disponibili restano in
coda e vengono recuperati nelle callback successive.

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

## Enhanced Audio Balance

La categoria Audio espone un unico interruttore globale `Enhanced Audio
Balance`, Enabled per default. Quando è attivo, il core applica automaticamente
a ogni gioco supportato i coefficienti SCSP di SM2-Emu 0.9.7; VF2 classifica
inoltre gli slot attivi come musica, effetti, annunciatore o voci. Disabled
ripristina immediatamente unity gain per tutti gli slot e conserva l'output
emulato senza il mastering aggiuntivo.

`Music Volume` è un controllo globale separato, modellato sull'opzione del core
Supermodel. Offre 0–200% a passi di 10 con default 100% e scala esclusivamente
l'audio MPEG prodotto dalle schede DSB/DSB2 prima che venga sommato all'SCSP.
Interessa le famiglie STCC e Top Skater; effetti, voci e giochi privi di una
scheda musicale separata restano invariati. La variazione è immediata.

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
L'implementazione adatta il decoder drive-board upstream 0.9.9 all'interfaccia
rumble Libretro. Conserva tutte le scritture del frame e distingue i protocolli
Daytona, STCC e Sega Rally; questo evita di perdere i byte effetto di Indy 500 e
di trattare le forze continue di STCC o Sega Rally come urti Daytona. Nei giochi
di guida gli effetti `Push` e `Vibrate` non continui producono brevi colpi
sul motore strong, accompagnati a metà intensità dal motore weak; lo
scostamento dello sterzo produce una vibrazione più leggera. Gli altri giochi
restano silenziosi. Il core genera questi rapporti sulla scala normalizzata;
`Input Rumble Gain` di RetroArch è l'unico controllo dell'intensità complessiva.

La variazione delle opzioni è immediata. Disattivazione, reset, unload ed errori
azzerano entrambi i motori, evitando vibrazioni bloccate. L'interfaccia rumble
non rappresenta la forza direzionale di un volante e non gestisce il recoil
fisico delle lightgun; il recoil `evdev` è annotato nella roadmap come possibile
integrazione futura.

## Save State Libretro

Il core implementa `retro_serialize_size`, `retro_serialize` e
`retro_unserialize` usando la serializzazione frontend-neutral introdotta
dall'upstream 0.9.8 per tutte le quattro varianti Model 2. RetroArch possiede
slot e file: il core non importa l'interfaccia standalone né crea una propria
directory degli stati.

La dimensione dichiarata è fissa a 9 MiB ed è disponibile già prima del
caricamento del gioco, requisito pratico di RetroArch 1.22.2. L'immagine contiene
magic, versione, nome del set e tipo di scheda; stati di un altro gioco o board,
intestazioni errate e payload troncati sono rifiutati. Il caricamento troncato è
transazionale e lascia intatta la macchina corrente. Il payload macchina usa il
formato upstream versione 4; una coda Libretro separata e versionata conserva
soltanto lo stato runtime del frontend, senza introdurre dipendenze nella macchina.
Non è promessa compatibilità con altre revisioni del core.

Il core usa inoltre la più recente negoziazione pubblica Libretro
`RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS` per dichiarare che il formato è
dipendente da piattaforma ed endianness. I frontend che non implementano il
comando lo ignorano e continuano a usare gli stessi tre callback standard; non
sono richieste estensioni private di RetroArch.

Dopo un caricamento vengono invalidate le risorse derivate del renderer e
azzerate le code audio e il rumble. Cadenza, cambio, controlli relativi e stato
del puntatore vengono invece ripristinati dalla coda Libretro: il loro precedente
azzeramento causava in Daytona una divergenza di cinque byte dopo tre frame.
I test ABI con ROM reali hanno verificato round-trip byte-identico e
riesecuzione deterministica su Daytona USA, VF2, Indy 500 e STCC, oltre al
rifiuto senza effetti di stati corrotti e di VF2 caricato in Daytona. Venti
cicli per ciascuna board coprono sia il pattern run-ahead, con esecuzione
speculativa e ripristino, sia una sequenza di stati ricaricata in ordine inverso
come rewind. Daytona supera la stessa prova anche con timing frontend a 60 Hz.
RetroArch Nightly 1.22.2 su
macOS ha caricato lo stesso stato VF2 dal proprio replay sia con Software sia
con Vulkan, completando 2300 frame con screenshot, audio e SRAM validi. Il
runner `scripts/test-retroarch-savestate.py` conserva una verifica separata del
percorso file e slot del frontend e ora usa `PAUSE_TOGGLE`; rifiuta confronti
casuali tra frame animati e permette di omettere il replay, incompatibile con il
caricamento di uno stato esterno durante la riproduzione.

La build Linux x86_64 del worktree corrente ha ripetuto su Batocera 43.1 la
prova Daytona con 20 cicli: round-trip, replay deterministico, pattern run-ahead,
ricarica in ordine inverso come rewind e rifiuto degli stati corrotti sono tutti
superati. Questa prova qualifica il formato e la coda Libretro anche sul target
Linux; file e slot grafici restano responsabilità di RetroArch.

Save/load è disabilitato durante `Linked Cabinets`: la communication board
interna è serializzabile, la sessione Netpacket esterna no. Rewind e run-ahead
sono qualificati su macOS per tutte le quattro board e su Batocera per Daytona;
le altre combinazioni di piattaforma e board estendono la matrice senza cambiare
il formato.

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
