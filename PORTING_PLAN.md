# Roadmap del core SM2-Emu Libretro

Il progetto indipendente `Zer0one/sm2-emu-libretro` sviluppa su `main` e usa
`dmanlfc/sm2-emu` come remote `upstream`. La base iniziale è SM2-Emu 0.9.4,
commit `8b3a468c5b51387093811cb16b076e6fd9289d66`. Il clone mainstream rimane
separato, per build e confronti con il codice originale. Il remote è stato
ricontrollato il 19 settembre 2026: `upstream/main` è
`af0be801980e40eb1f7eeff7f72ecc2f8ffe1024`, release 0.9.9. Le modifiche
applicabili al core e approvate fino a questa revisione sono state integrate
selettivamente; il core dichiara ora la versione 0.9.9. Le esclusioni e gli
adattamenti Libretro sono riepilogati nel punto 7.

Obiettivo: un core per RetroArch e Batocera, preservando emulazione, timing,
audio e controlli. Ogni traguardo richiede una prova eseguibile; una build
riuscita da sola non dimostra compatibilità o gameplay corretto.

Menu, profili e opzioni seguono [LIBRETRO_DESIGN.md](LIBRETRO_DESIGN.md),
che adatta le convenzioni del progetto personale Supermodel a Model 2.

## 0. Baseline upstream — completata nei limiti indicati

Standalone macOS arm64 compilato e avviato con Vulkan/MoltenVK su Apple M4.
Catture delle sequenze dimostrative di Daytona USA, Sega Rally e Virtua Fighter 2.
Vedere [MACOS_BUILD.md](MACOS_BUILD.md).

## 1. Macchina senza frontend — completata il 9 settembre 2026

- Separare in CMake librerie di emulazione e programma SDL: il target di prova
  deve configurarsi e compilarsi senza SDL, Vulkan/OpenGL, ImGui o glslc.
- Esporre l'audio di ogni scheda attraverso l'interfaccia comune della macchina,
  usando il sample rate effettivo e svuotando il buffer una volta per frame.
- Aggiungere `sm2-headless`: caricamento ROM, NVRAM nuova, esecuzione di N frame,
  rendering software per ogni frame, registrazione audio e immagine finale.
- Confrontare con `--boot-test` dello standalone originale: immagine finale,
  campioni audio, NVRAM, stato CPU e cicli devono coincidere.
- Ricompilare anche lo standalone, per verificare che la separazione non lo rompa.

Criterio superato: build indipendente e confronti riusciti su Daytona USA
(Model 2), Virtua Fighter 2 (2A), Virtual On (2B) e Sega Touring Car (2C),
1800 frame ciascuno. Vedere [HEADLESS.md](HEADLESS.md) per dettagli e limiti.
Questo target è uno strumento di verifica, non un core Libretro.

## 2. Primo core Libretro software — completata il 9 settembre 2026

- Aggiungere il target dinamico e l'ABI Libretro: init/deinit, load/unload,
  `retro_run`, reset, informazioni di sistema e callback del frontend.
- Usare loader e macchina esistenti; individuare `games.xml`, ROM e salvataggi
  attraverso le directory fornite dal frontend, senza percorsi locali fissi.
- Consegnare un frame per `retro_run()`: conversione esplicita dal formato
  software upstream al pixel format Libretro, geometria 496 × 384, aspect 4:3
  e timing derivato dall'hardware. Verificare colori, orientamento e pitch.
- Consegnare audio stereo al rate della scheda, gestendo correttamente i campioni
  accettati dal frontend e le transizioni tra contenuti.
- Input minimo necessario alla prova: moneta, start, comandi principali e reset.
- Dichiarare i save state non supportati finché la serializzazione non esiste.

Criterio: un titolo realmente avviato in RetroArch macOS con immagine, audio e
controlli; prove ripetute load/unload/reset e confronto con la baseline.
Criterio superato: VF2 avviato in RetroArch macOS con input da replay,
immagine di gameplay e audio stereo registrato; test ABI load/unload/reset e
confronti video/audio/NVRAM identici su tutte le quattro schede. Vedere
[LIBRETRO.md](LIBRETRO.md) per prove ripetibili, confini del supporto e limiti
rispetto a controller fisici e ascolto manuale.

Ordine aggiornato su richiesta dell'utente: dopo il punto 2 è stato anticipato
il punto 5 (GPU), prima dell'implementazione dei profili. I numeri restano stabili
per i riferimenti esistenti: **0 → 1 → 2 → 5 → 3 → 4 → 6**.
Vedere [GPU.md](GPU.md) per implementazione, prove e limiti della prima GPU macOS.

## 3. Controlli, NVRAM e opzioni essenziali

Procedere per sottopunti circoscritti, senza implementarli tutti in blocco.
Il primo definisce il catalogo; le mappature si implementano dopo la sua revisione.

### 3.1 Catalogo dei profili dai metadati — revisionato e implementato

- Inventariare i `GameSpec` dopo l'ereditarietà parent/clone.
- Definire famiglie e varianti con il criterio usato in Supermodel; per giochi
  e seguiti con controlli equivalenti conservare nome, disposizione e convenzioni
  del profilo, documentando differenze reali e lacune dei metadati.
- Separare ciò che i metadati determinano dalle eccezioni o lacune da verificare.
- Consegnare [CONTROL_PROFILES.md](CONTROL_PROFILES.md): 83 set, 14 firme
  generiche e 23 raggruppamenti di lavoro, con matrice Model 2 / Model 3 e casi da verificare.

Criterio: copertura del catalogo e revisione dei profili proposti. Nessuna
nuova mappatura, opzione o modifica alla persistenza in questo sottopunto.

### 3.2 Riconoscimento e controlli digitali — implementato e verificato localmente

- Implementare il riconoscitore secondo il catalogo revisionato, con fallback
  esplicito; registrare dispositivo e descrittori pertinenti al gioco.
- Completare Joystick (Standard), comprese Fighting/Soccer, e Joystick (Twin),
  preservando azioni e alias e verificando la permutazione Soccer in partita;
  aggiungere le varianti comuni con/senza Test/Service rimappabili.

Criterio: test del riconoscitore sui set catalogati, nessun controllo improprio,
P1/P2 indipendenti e prova dei profili digitali in RetroArch.

Il riconoscitore copre i 34 set parent/clone revisionati per i profili digitali
e rifiuta firme I/O cambiate o non catalogate. Nomi, descrizioni e posizioni
RetroPad sono quelli approvati in `Docs/revisione_profili_model2.xlsx`. Ogni
profilo espone la variante predefinita con Test/Service e quella ridotta senza
i due comandi. I controlli ROM-free verificano inoltre indipendenza P1/P2,
permutazione hardware Soccer e doppia leva Virtual On. VF2, Virtua Striker e
Virtual On sono stati avviati fino a una partita reale in RetroArch macOS;
rimane distinta la prova manuale con un controller fisico.

### 3.3 Profili di guida — implementati

- Procedere per varianti 4-Speed + VR4/VR1, Sequential e Motorcycle.
  Per Sega Rally verificare Handbrake prima di esporre il profilo completo
  condiviso con Sega Rally 2; la lacuna è documentata nel catalogo.
- Rispettare canali, calibrazioni, polarità e comandi del cabinet; aggiungere
  cambio e regolazioni di sterzo/acceleratore/freno con default trasparenti.
- Introdurre insieme alle funzioni le relative Core Options v2 e descrizioni.

Criterio: estremi/riposo degli assi, cambio e azioni coperti per variante.

Il primo gruppo copre i quattro set Daytona e i cinque set Sega Rally. Espone
`Driving: 4-Speed + VR4` e `Driving: 4-Speed + VR1 + Handbrake`, ciascuno con
la variante predefinita Test/Service e quella ridotta. L'opzione `4-Speed
Shifter` seleziona H-Gate, predefinito, oppure Standard; Neutral e Shift
Down/Up restano sempre disponibili. I test verificano assi, trigger analogici,
VR, cambio e freno a mano IN2. Le ROM parent hanno raggiunto una gara reale in
RetroArch macOS, a 300 km/h Daytona e 127 km/h Sega Rally nel frame acquisito.
I profili Motorcycle sono coperti nei gruppi Manx TT e Motor Raid descritti sotto.

Il secondo gruppo copre dieci set delle famiglie Indy 500, Over Rev e Sega
Touring Car Championship con il profilo unico `Driving: Sequential + VR2`.
Descrizioni e posizioni derivano direttamente dalle tre righe approvate del
foglio Excel. View 1/2 usano D-Pad Up/Down; Shift Down/Up usano L1/R1 e agiscono
direttamente sui due ingressi momentanei hardware; Brake, Accelerator e Steering
restano esclusivamente analogici. Il profilo espone entrambe le varianti con e
senza Test/Service.

Le tre ROM parent sono state avviate con successo in RetroArch macOS usando un
replay di avvio: Indy 500 ha raggiunto
la griglia di partenza, Over Rev la selezione modalità e STCC l'inserimento
nome. Audio e Save RAM sono risultati validi in tutte le esecuzioni.

Il terzo gruppo aggiunge il solo Super GT 24h come `Driving: Sequential + VR1`.
Rispetto alla variante VR2 espone esclusivamente `VR1` su D-Pad Down; cambio,
sterzo e pedali seguono le posizioni approvate nel foglio, con pedali analogici
e polarità invertita dichiarata dai metadata, senza correzioni aggiuntive nel
binding. MAME segnala ancora sterzo non centrato e pedali pulsanti: il limite va
indagato nell'emulazione analogica comune. La ROM parent ha raggiunto la
selezione del circuito in RetroArch macOS.

Il quarto gruppo aggiunge i tre set Manx TT come `Driving: Sequential (Manx TT
Superbike)`. Il profilo usa Bank sullo stick sinistro X, Brake/Accelerator sui
trigger analogici, Shift Down/Up su L1/R1 e l'azione combinata `Start / VR` su
Start; non espone comandi sul D-Pad. La polarità Bank segue il metadato
invertito. La ROM parent ha raggiunto il controllo iniziale del motion slider
in RetroArch macOS.

Il quinto gruppo aggiunge `motoraid` e `motoraiddx` come `Driving: Sequential
(Motor Raid)`. Rispetto a Manx TT aggiunge Kick su South e Punch su East;
queste azioni raggiungono gli stessi due ingressi arcade usati rispettivamente
da Shift Up e Shift Down, secondo il cablaggio del gioco e le posizioni
approvate nel foglio. La ROM parent ha raggiunto una gara in RetroArch macOS;
audio e Save RAM sono validi.

### 3.4 Puntamento e joystick analogico

Stato: Joystick (Analog) e famiglia Gun implementati.

Criterio: calibrazione, polarità, pulsanti e cambio sorgente coperti.

L'audit preliminare dei sei parent Gun ha confermato coordinate assolute per
tutti. `vcop`, `vcop2` e `hotd` usano la lightgun seriale RS-422 a 10 bit e
supportano il reload sparando fuori dall'area calibrata; il foglio assegna
quindi `Reload Offscreen` a East. `gunblade`, `rchase2` e `bel` usano assi
posizionali a 8 bit e non espongono questa azione; BEL conserva `Missile` su
East. Queste due famiglie hardware richiedono traduzioni interne distinte.

Il primo gruppo del punto 3.4 implementa `skytargt` come `Joystick (Analog):
Sky Target`. Il profilo usa esclusivamente lo stick sinistro per gli assi X/Y,
con X invertito e Y normale secondo i metadata, South/RB ed East/LB per
`Machine Gun`/`Missile`, e D-Pad Up per `View Change`. Espone le
varianti con e senza Test/Service. I controlli ROM-free verificano firma,
descrittori, bit digitali e assi. La ROM parent
ha raggiunto una partita in RetroArch macOS; audio e Save RAM sono validi.

Il secondo gruppo implementa `bel` come `Gun: Behind Enemy Lines` per due
giocatori. Ogni port usa South/East per `Shot`/`Missile` e lo stick sinistro
per `Gun Yaw (Analog Cursor)`/`Gun Pitch (Analog Cursor)`, senza binding non
previsti dal foglio. La traduzione conserva i quattro range analogici distinti,
i bit Shot/Missile di P1/P2 e lo scambio hardware delle linee Test/Service
specifico di BEL. I controlli ROM-free coprono entrambe le varianti service,
descrittori, assi e bit digitali. La ROM ha raggiunto una partita in RetroArch
macOS; audio e Save RAM sono validi.

Il terzo gruppo completa la famiglia Gun: `gunblade`, `rchase2`, `vcop`,
`vcop2` e `hotd`, incluse le revisioni, usano il profilo `Gun`; BEL conserva il
profilo specifico. La Core Option `Gun Input Mode` riprende da Supermodel i
percorsi Standard, Lightgun, Mouse + Analog Stick, Mouse e Analog Stick e
aggiorna subito nomi e descrittori. Il core traduce il cursore virtuale nel
protocollo seriale RS-422 a 10 bit oppure nei quattro canali posizionali a 8 bit
in base ai metadata. `Reload Offscreen` è disponibile solo per Virtua Cop 1/2 e
House of the Dead; BEL espone invece `Missile` su East/LB.

Il difetto del mirino verticale di BEL con un `.srm` già inizializzato è stato
risolto estendendo il workaround hardware esistente al preset di calibrazione
incluso nell'Initial NVRAM. Build, controlli input, avvio automatico di 600 frame
e prova manuale in RetroArch macOS completati: il Service Menu riceve entrambi
gli assi e il mirino del gioco si muove in entrambe le direzioni.

La Core Option `Off-Screen Reload Shortcut`, visibile soltanto sui sei set
seriali, abilita per default RetroPad East/LB, Mouse destro e Lightgun Reload. Se
disabilitata rimuove questi descrittori e ingressi; una lightgun puntata fuori
schermo e azionata col grilletto continua a usare il gesto nativo del cabinet.

I controlli senza ROM verificano tutti i dieci set, P1/P2, le cinque modalità,
i range e le polarità degli assi, il particolare grilletto P2 di House of the Dead e l'assenza
del reload nei cabinet posizionali. Gunblade NY e Virtua Cop 2 hanno raggiunto
una partita in RetroArch macOS con audio e Save RAM validi.

### 3.5 Cabinet speciali

Primo gruppo completato: `dynabb` e `dynabb97` espongono il profilo condiviso
`Joystick (Standard): Baseball (Dynamite Baseball)`, con joystick, due pulsanti
e `Bat Swing` su Right Analog Y verso il basso per ciascun giocatore. I
controlli ROM-free verificano firma, descrizioni, P1/P2, semiasse e varianti
Test/Service; entrambi i parent hanno raggiunto una partita in RetroArch macOS.

Secondo gruppo completato: `segawski` espone `Special: Water Ski` con Set su
South, Pitch Left/Right su L1/R1 e sui binding secondari West/East, Slide su
Left Analog X e l'alias hardware
`Start / Select Down`. Slide applica la polarità invertita dichiarata nei
metadata. `skisuprg` espone `Special: Ski Super G`
con Inclining su Right Analog X/canale 0 e Swing su Left Analog X/canale 1.
Swing applica la polarità invertita dichiarata nei metadata e
Foot Sensor attivi alti su L1/R1. I controlli ROM-free verificano
firme, descrittori, bit, assi e varianti Test/Service. Water Ski ha raggiunto
una gara reale. Ski Super G carica profilo, audio e Save RAM, ma resta bloccato
su `DRIVE BOARD TROUBLE CODE: FF`: il database SM2 non include la ROM
drive-board definita da MAME. La relativa emulazione resta un
sottopunto futuro separato dai profili.

Terzo gruppo completato: `topskatr`, `topskatrj`, `topskatru` e `topskatruo`
espongono `Special: Top Skater`. Curving e Slide restano assi distinti su Left
Analog X e Right Analog X. Curving applica la polarità invertita dichiarata nei
metadata; Slide resta invariato. Select Left/Right e
Jump Front/Tail raggiungono i bit
documentati da MAME. I controlli ROM-free coprono tutte le revisioni, firme,
descrittori, assi e varianti Test/Service. Il parent ha raggiunto una sessione
di gioco in RetroArch macOS con audio e Save RAM validi.

Quarto gruppo completato: `waverunr` espone `Special: Wave Runner` con View,
Handle, Pitch, Roll e Throttle nelle posizioni approvate. I controlli ROM-free
verificano firma, descrittori, intera escursione dei quattro canali, valore di
riposo dichiarato, safety sensor non bindabile e varianti Test/Service. Il
parent ha raggiunto una sessione di gioco in RetroArch macOS con audio e Save
RAM validi.

Quinto gruppo completato: `airwlkrs` espone P1/P2 con
`Joystick (Standard): Basketball (Air Walkers)`; `rascot2` espone
`Joystick (Standard): Horse Racing (Royal Ascot II)` secondo i metadata
disponibili. Entrambi hanno firme, descrittori, bit e varianti Test/Service
coperti dai controlli ROM-free. Air Walkers raggiunge una partita reale con
audio e Save RAM validi. Royal Ascot II mostra il titolo e salva la Save RAM,
ma resta silenzioso e non supera la sequenza iniziale verso l'attesa SegaNet
nel replay standard.

Sesto gruppo completato: `desert` espone `Joystick (Analog): Desert Tank + VR3`. Il backend
Model 1 I/O esistente riceve ora dai metadata condivisi Steering, Accelerator
ed Elevation sui canali 0-2; il profilo aggiunge VR1-VR3, Machine Gun, Cannon e
Shift toggle con le posizioni approvate. I controlli ROM-free verificano firma,
descrittori, riposo e controllo relativo/assoluto di Elevation, velocità e
inversione dell'asse, toggle, alias RB/LB per Machine Gun/Cannon e varianti
Test/Service.
La build standalone compila con lo
stesso metadato e il parent ha raggiunto una missione reale in RetroArch macOS
con audio e Save RAM validi.

Settimo gruppo completato: `hpyagu98` espone `Joystick (Standard): Baseball
(Hanguk Pro Yagu 98)` a due giocatori, con D-Pad e Button 1/2/3 sul layout
elettrico VF2 documentato da MAME. I controlli ROM-free verificano entrambe le
porte, descrittori, bit e varianti Test/Service. Il parent ha raggiunto una
partita reale in RetroArch macOS con audio e Save RAM validi. Il problema di
persistenza di `FAVORITE` resta confinato alla NVRAM e non blocca il profilo
input.

- Integrare in seguito P3/P4 di Air Walkers mediante il multiplexing della
  matrice. P1/P2, incluse le rispettive linee Start/Coin, sono implementati.
- Integrare ciascun profilo con i suoi test senza estendere implicitamente le
  conclusioni agli altri cabinet o confondere input supportati e gioco funzionante.

Criterio: chiusura documentata per ogni profilo; i casi non verificati restano
indicati nel catalogo e nel frontend.

La crosshair è implementata nei backend Software, Vulkan e OpenGL. Il valore
`Automatic` segue la convenzione upstream: P1 esterno per i giochi con pistola
seriale e mirino in gioco per quelli posizionali; una selezione esplicita rende
il mirino esterno disponibile anche per questi ultimi. `Crosshair Style` offre
lo stile SM2-Emu come default e Supermodel come alternativa. Le prove sui sei giochi hanno confermato
che i valori di calibrazione nativi sono già funzionanti; per decisione
dell'utente non serve una campagna dedicata né l'iniezione di ulteriori dati nei
campioni iniziali.

### 3.6 Persistenza NVRAM/EEPROM

Stato: persistenza e matrice dei parent implementate. `RETRO_MEMORY_SAVE_RAM`
contiene backup RAM ed EEPROM in formato versionato; import frontend, fallback
nativo e 289 impostazioni operatore per 35 parent più quattordici cloni con menu
specifico sono coperti da test senza
ROM e da avvii reali consecutivi in RetroArch. `Automatic Initial NVRAM Setup`
carica per questi parent un campione completo validato prima del primo frame,
soltanto in assenza di `.srm` e NVRAM native valide, quindi applica Country/Nation
USA o Export, i valori offline necessari e I/O Type C per Super GT 24h.

- Valutare SRAM gestita dal frontend definendo prima formato, precedenza e
  import conservativo dei file nativi. Non confonderla con i save state.
- Non inizializzare o modificare automaticamente i salvataggi esistenti; il
  setup automatico interviene soltanto alla creazione di una nuova Save RAM.
- Prima fase della campagna clone completata il 15 settembre 2026: tutti i 48
  cloni presenti in `games.xml` sono stati avviati senza input da una directory
  vuota e hanno prodotto backup RAM ed EEPROM native. Ventuno acquisizioni sono
  identiche al parent e 27 presentano differenze stabili; nessun avvio è fallito
  e una seconda acquisizione dei 22 parent ha escluso variabilità tra esecuzioni
  equivalenti. Il dettaglio è in `CLONE_NVRAM.md`; i dati grezzi restano esclusi
  da Git.
- Per i 21 cloni identici, l'eredità del template parent è autorizzata
  esplicitamente e coperta dalla campagna conclusiva.
- Ventuno cloni usano ora template dedicati. Tredici applicano senza modifiche
  le opzioni del parent; `daytona93`, `daytonas`, `dyndeka2`, `dyndeka2b`,
  `motoraiddx`, `stcca`, `stccb` e `stcco`
  hanno invece cataloghi specifici ricavati dal proprio Service Menu.
  `daytona93` espone solo quattro
  voci, `daytonas` aggiunge Cabinet=Special e Promote Saturn, mentre `stcca`,
  `stccb` e `stcco` usano la propria codifica Country; soltanto `stccb` conserva
  Default View. Complessivamente 494
  valori e 42 avvii RetroArch hanno verificato layout, integrità, setup pulito e
  persistenza. Con i 21 cloni byte-identici al parent, il setup automatico copre
  42/48 cloni. Il dettaglio e gli aggiornamenti successivi sono in
  `CLONE_NVRAM.md`.
- `daytonam` conserva l'identificatore upstream `protection="daytona-maxx"` e
  la relativa PIC è emulata nel solo percorso Model 2 Original. Con ROM reale
  il clone supera la schermata operatore e raggiunge Circuit Select con lo
  stesso test Coin/Start del parent; il gameplay è stato poi confermato
  manualmente in RetroArch macOS.

Criterio: persistenza e import provati con casi positivi e negativi; eventuali
campi NVRAM esposti devono avere valori e precedenza espliciti.

Aggiornamento 18 settembre: `vf2a` e `vf2o` dispongono ora di template e
cataloghi propri, riutilizzando gli otto parametri e le codifiche VF2. La
revisione del blocco è 0x13/0x12 anziché 0x18; il campo a 0x3318 è aggiornato
dal gioco e non viene trattato come firma fissa. Il setup copre 44/48 cloni.
`hotdp`, prototipo, è escluso da NVRAM Settings e setup dedicato per decisione
dell'utente. `indy500d` è ora integrato con banca da 44 byte e sette opzioni specifiche;
la copertura aggiornata è 45/48. `vstrikero` è ora integrato con il proprio
menu ridotto di dieci voci, senza One Match Mode, e con Advertise Sound al suo
offset specifico. La copertura è quindi **46/48**: `hotdp` resta escluso perché
prototipo e soltanto `srallycdxa` richiede ancora analisi.

Aggiornamento 19 settembre: anche `srallycdxa` è integrato con il proprio
layout EEPROM da 44 byte e il menu ridotto a Advertise Sound, Country, Game
Difficulty e Game Mode. Cabinet Type e Link Type non sono presenti e i relativi
byte restano invariati. Il setup automatico copre quindi **47/48 cloni**; il
solo `hotdp` rimane intenzionalmente escluso perché prototipo.

Aggiornamento successivo del 19 settembre: su richiesta è stato integrato anche
`hotdp`. Il prototipo usa due banche EEPROM speculari da 24 byte, distinte dal
layout retail, e dispone di quattro Core Options verificate: Game Difficulty,
Blood Color, Advertise Sound e Country. Blood Color espone soltanto Red/Green;
Life Setting, Gun Blowback e Cabinet Type restano documentate ma escluse secondo
la revisione concordata. Template dedicato, checksum, primo avvio e persistenza
portano la copertura della campagna a **48/48 cloni**, con 27 template dedicati
e 21 ereditati.

### 3.7 Revisione dei menu — completata

- Consolidare le opzioni introdotte nei sottopunti precedenti secondo
  LIBRETRO_DESIGN.md: categorie pertinenti, visibilità, default e riavvio richiesto.
Criterio: categorie, visibilità, default e indicazioni di riavvio coerenti con
`LIBRETRO_DESIGN.md`; le prove fisiche residue sono raccolte nella sezione finale.

### 3.8 Core Options derivate da Supermodel

Stato: `A/V Timing` e `Timing / FPS Overlay` sono implementate. Il timing nativo
57,524160 Hz resta il default; la modalità di compatibilità presenta a 60 Hz senza
accelerare la macchina, distribuendo i frame duplicati e l'audio sulla cadenza
del frontend. L'overlay usa lo status OSD Libretro e riporta medie su 61 callback
per macchina, video, audio, `retro_run`, frame peggiore, cadenza effettiva e
capacità stimata. `Frame Skip` è stato escluso per scelta progettuale.

Il cambio a quattro marce `H-Gate Mode`/`Standard`, derivato da Supermodel, è
già implementato con H-Gate come default.

Opzioni legate ai controlli introdotte direttamente dal core Supermodel:

- `Driving Steering Response`: `Linear` come default, `Progressive (Fine
  Center)` e `FBNeo Logarithmic (Fine Center)`; applicazione limitata ai
  profili Driving;
- `Driving Steering Output Range`: default `100%`, con regolazione comune
  `50%`–`150%`; sotto il 100% limita l'escursione emulata e sopra il 100%
  anticipa il fondo corsa. Include il preset Supermodel `63% (30-80-D0)`, che
  produce esattamente tale intervallo ADC;
- `Driving Accelerator Output Range` e `Driving Brake Output Range`: opzioni
  indipendenti, default `100%`, con regolazione comune `50%`–`150%`. Il preset
  Supermodel `75.3% (00-C0)` produce esattamente tale intervallo ADC.

Le quattro regolazioni precedenti sono implementate, restano sempre visibili,
agiscono soltanto sui profili Driving e sono verificate con curve, saturazione,
pedali normali/invertiti e Throttle motociclistico.

`Gamepad Rumble` è implementata seguendo il decoder upstream 0.9.9.
L'interfaccia rumble Libretro pilota i motori strong e
weak del gamepad P1: i comandi della drive board producono colpi brevi e lo
scostamento dello sterzo una vibrazione più leggera. Il default è Enabled al
massimo della scala normalizzata; RetroArch applica il proprio `Input Rumble
Gain` come unico controllo dell'intensità complessiva. I giochi senza controllo
di guida restano silenziosi. Restano da
introdurre o valutare separatamente:

- force feedback direzionale per volanti, che non può essere rappresentato
  integralmente dai due motori della normale interfaccia rumble Libretro;
- recoil lightgun tramite `evdev`, da conservare in roadmap per un'eventuale
  implementazione futura. Richiederà un percorso specifico della piattaforma,
  separato dai binding e dalla vibrazione gamepad.

Come in Supermodel, le quattro regolazioni di guida devono restare sempre
visibili e non produrre effetti fuori dai profili Driving.

`Enhanced Audio Balance` è implementata come unico interruttore globale nella
categoria Audio. Enabled applica automaticamente i profili SCSP upstream 0.9.7
a tutti i set supportati; Disabled ripristina unity gain in tempo reale. Il fix
INTENA di `overrevb`/`overrevba` resta sempre attivo perché corregge il timer
audio della macchina e non appartiene al mastering opzionale.

`Aspect Ratio` è implementata con `Auto` come default globale e override `4:3`
o `16:9`. In Auto legge la configurazione attiva dalla NVRAM: Indy 500 e STCC
usano 16:9 nei rispettivi cabinati Deluxe e 4:3 in Twin; gli altri giochi usano
4:3. Il framebuffer resta 496×384 e il frontend riceve soltanto la geometria.

Altre opzioni da valutare separatamente:

- abilitazione dell'emulazione audio, solo se produce un risparmio reale;
- volume separato della musica DSB/MPEG, se il mixer conserva flussi distinti;
- supersampling aggiuntivo oltre alle scale e ai filtri 2D già disponibili;
- adattamento colore CRT specifico Model 2, solo con una necessità misurata.

Queste voci sono una roadmap, non funzionalità dichiarate. Renderer Model 3,
PowerPC/JIT e DSP specifici di Supermodel restano esclusi perché non applicabili.

## 4. Compatibilità e build di distribuzione

Prima matrice CI implementata: Linux x86_64, macOS arm64/Intel e Windows
x86_64, con software + Vulkan, controlli ABI senza ROM e pacchetti corredati
dal database giochi. Evidenze e limiti in [CI.md](CI.md). Linux arm64 resta
da aggiungere e la milestone completa richiede ancora la matrice estesa sotto.

- Automatizzare build macOS arm64 e Linux x86_64/arm64, controlli ABI e test
  eseguibili senza ROM. Le ROM restano esterne al repository e alla CI.
- Preparare core info e istruzioni di installazione, mantenendo lo standalone
  disponibile come riferimento separato.

Criterio: artefatti installabili sulle piattaforme dichiarate, con matrice di
compatibilità e limiti documentati.

## 5. Rendering GPU integrato nel frontend — Vulkan e OpenGL implementati

- Scegliere il backend dopo aver verificato i contesti realmente disponibili
  nelle build RetroArch target. Non assumere OpenGL 4.3 su macOS.
- Separare rendering da finestra, swapchain e presentazione possedute dall'OSD;
  usare il contesto fornito dal frontend e gestirne perdita e ricreazione.
- Confrontare la resa con la baseline software, compresi scaling e trasparenze.

Criterio: rendering hardware in RetroArch con recupero del contesto e
confronti visivi documentati, mantenendo il percorso software funzionante.

Stato: adattatori Vulkan e OpenGL implementati con passaggi/shader upstream condivisi,
contesto del frontend, Core Options Video e risoluzioni 1×–4×. Sono inoltre
integrati `3D Texture Filtering` (Faithful, Anisotropic 2x–16x) e
`2D Layer Upscaling Filter` (Faithful, xBR, ScaleFX): i filtri 2D operano sulle
tilemap prima della composizione col 3D e restano distinti dagli shader del
frontend. VF2 provato in
RetroArch macOS; quattro schede provate nel frontend GPU di verifica; confronto
pixel esatto con lo standalone Vulkan a 1×/4× e prove di ricreazione del dispositivo.
OpenGL 4.3/OpenGL ES 3.1 riusa i pass upstream ed è stato verificato tramite
EGL/Mesa con VF2 e Last Bronx, scale 1×/4× e perdita/ricreazione del contesto;
immagine, audio e NVRAM coincidono con la baseline software nei casi confrontati.
La modalità 60 Hz ora forza una nuova immagine dopo ogni ricreazione del contesto,
anche quando il callback avrebbe duplicato il frame precedente; il percorso
framebuffer speciale è stato verificato sul title screen di Last Bronx.
La verifica degli altri sistemi e della compatibilità estesa continua al punto 4.
Dettagli, limiti e avvio con MoltenVK aggiornato: [GPU.md](GPU.md).

## 6. Networking, save state e funzioni avanzate

### 6.1 Collegamento tra cabinet — prototipo Daytona conservato

- Usare direttamente la struttura upstream 0.9.7: `M2Comm` possiede il
  `CommTransport` e conserva il protocollo della communication board Model 2.
- Adattare a RetroArch il solo trasporto, sostituendo UDP con Netpacket; la
  piccola estensione `ready()` attende il peer prima di avviare il timer del link.
- Usare l'interfaccia ufficiale Libretro Netpacket: nessun socket, discovery o
  configurazione di rete appartiene al core.
- Conservare il comportamento standalone a cabinet singolo quando la Core
  Option `Linked Cabinets` è lasciata su `Disabled`, valore predefinito.
- Mantenere `Linked Cabinets` sempre visibile come le altre opzioni non NVRAM,
  dichiarando che il trasporto Netpacket del core si applica attualmente alla
  sola famiglia Daytona USA.

Il prototipo attuale resta circoscritto alla famiglia Daytona USA e a due
cabinet. Host e
client usano lo stesso ROM set e core; `NVRAM Settings` imposta rispettivamente
`Link ID=Master`/`Car Number=1` e `Link ID=Slave`/`Car Number=2`. Un test reale
con due istanze RetroArch 1.22.2 su macOS ha formato l'anello con ID `01/02` e
`02/02`, eseguito 3600 frame per istanza e avviato una gara condivisa. Le
catture finali mostrano l'host rosso `2nd/2`, con l'auto blu `2P` davanti, e il
client blu `1st/2`, entrambi in movimento nella stessa sessione.
I test senza ROM verificano inoltre assegnazione degli ID, conteggio dei nodi,
trasporto integro dei frame, rifiuto di un terzo client e disconnessione.

La Core Option è già predisposta con valori da 2 a 8 cabinet, massimo verificato
nei menu operatore Model 2; le sessioni superiori a due restano da implementare
nel trasporto Netpacket.

Limiti attuali: due cabinet soltanto e famiglia Daytona soltanto. L'estensione
agli altri giochi Model 2 con communication board resta sospesa finché il
porting generale non rende opportuno riprendere questa attività.

### 6.2 Save state — upstream 0.9.8 disponibile, adattamento Libretro da implementare

L'upstream 0.9.8 ha già introdotto `Archive`, la serializzazione delle quattro
varianti di scheda e il ripristino transazionale: magic, versione, gioco e board
sono verificati prima di modificare la macchina e un payload troncato ripristina
lo snapshot precedente. Il frontend standalone aggiunge file e slot propri;
questa parte non va portata nel core, perché RetroArch possiede slot e file.

- Importare la serializzazione frontend-neutral e i fix-up post-caricamento,
  mantenendo il layout upstream per facilitare gli aggiornamenti successivi.
- Collegarla a `retro_serialize_size`, `retro_serialize` e
  `retro_unserialize`, definendo una dimensione Libretro stabile e un contenitore
  versionato con gioco e board; non introdurre un secondo sistema di slot.
- Dopo il caricamento invalidare e ricostruire correttamente risorse GPU,
  generazioni video, code audio e stato dell'adattatore input.
- Rifiutare o gestire esplicitamente il caricamento durante un collegamento tra
  cabinet: lo stato della communication board è serializzabile, la sessione
  Netpacket esterna non lo è.
- Dichiarare rewind e run-ahead utilizzabili soltanto dopo la matrice di verifica
  raccolta nella sezione finale.

## 7. Allineamento selettivo upstream 0.9.8/0.9.9 — completato

L'analisi è allineata al commit upstream
`af0be801980e40eb1f7eeff7f72ecc2f8ffe1024` del 19 settembre 2026. Il core
dichiara 0.9.9 e conserva la provenienza della base iniziale 0.9.4.

| Gruppo upstream | Stato nel core |
| --- | --- |
| Save state 0.9.8 | Serializzazione frontend-neutral disponibile upstream; adattamento all'API Libretro ancora da implementare nel punto 6.2. |
| Daytona `start_gear=4` | Integrato con la Core Option globale `Automatic Start Gear`, abilitata per default. |
| FPS toggle standalone | Coperto da `Timing / FPS Overlay`; tasto e UI SDL esclusi. |
| Configurazione SDL dei volanti | Esclusa: dispositivi, mapping e conversione degli assi appartengono a RetroArch. |
| Drive command 0.9.9 | Integrati coda per frame, protocolli Daytona/STCC/Rally e adattamento Gamepad Rumble. |
| Metadata `games.xml` | Integrati `start_gear`, `drive_protocol`, correzioni `drive_board`, clone Daytona MAXX e aggiornamenti approvati. |
| Bilanciamento audio e timer INTENA | Integrati tramite `Enhanced Audio Balance` e fix macchina permanente. |
| Filtri xBR/ScaleFX | Integrati come filtri dei livelli 2D prima della composizione 3D. |
| Texture 3D personalizzate e dump | Non inclusi nello scope corrente; restano un miglioramento renderer opzionale. |
| Traslucenza blended | Non inclusa nello scope corrente; lo stipple hardware resta il comportamento fedele. |
| Funzioni GUI, file, slot e backend SDL/evdev | Escluse quando il frontend o la piattaforma possiedono già la funzione. |

Le future revisioni upstream seguiranno la stessa procedura: confronto con il
clone mainstream pulito, importazione selettiva delle parti applicabili e
adattamento confinato in `src/libretro/` quando la funzione appartiene al
frontend.

## Modalità di lavoro

Facilitare gli aggiornamenti upstream è un requisito trasversale: adattatore
isolato in `src/libretro/`, accessi neutrali minimi alla macchina, layout
originale conservato e nessuna duplicazione del motore. Seguire la sezione
architetturale di [LIBRETRO_DESIGN.md](LIBRETRO_DESIGN.md), mantenendo traccia
dei punti modificati e verificando ogni import rispetto alla stessa revisione
originale, oltre al confronto tra baseline precedente e nuova.

Un punto alla volta, con modifiche circoscritte e verifiche ripetibili. Non
estendere automaticamente il lavoro alle milestone successive. Segnalare
quando un passaggio beneficia di un modello più capace, spiegando il motivo;
non cambiare modello o delegare ad altri agenti senza autorizzazione.

`git fetch upstream` recupera riferimenti senza modificare i sorgenti. Gli
aggiornamenti si valutano e integrano su `main` con patch, cherry-pick o merge,
secondo il caso e le istruzioni correnti. Il mainstream si aggiorna separatamente.

## Verifiche ancora aperte

Questa sezione raccoglie soltanto attività di verifica. Le implementazioni
mancanti restano nei rispettivi punti della roadmap.

| Ambito | Verifica residua |
| --- | --- |
| Standalone baseline | Ascolto audio e prova dei controlli fisici sulla build macOS upstream. |
| Profili digitali | Prova manuale con un gamepad fisico rappresentativo. |
| Guida | Prova con volante e pedali reali, comprese calibrazione, polarità, cambio e Handbrake. |
| Gun | Prova con Lightgun e Mouse fisici sui percorsi dichiarati. |
| NVRAM/SRAM | Riavvio e separazione per gioco con file mancanti, invalidi e directory non scrivibili; conservazione dei salvataggi esistenti. |
| Menu e opzioni | Cambio contenuto, profilo, remapping e visibilità delle opzioni per gioco in RetroArch. |
| Regressione generale | Video, audio, input e NVRAM sulle quattro board con una matrice parent/clone, ZIP/7z e caricamenti falliti. |
| GPU Windows | RetroArch su Windows 11 con GPU dedicata: Vulkan/OpenGL, scale 1×/4×, lifecycle, SRAM, log e screenshot. |
| Piattaforme | Artefatto Linux arm64 e ulteriori prove RetroArch/Batocera sugli hardware dichiarati. |
| Networking | Due controller reali, due host fisici, variante Daytona MAXX e future sessioni superiori a due cabinet dopo la relativa implementazione. |
| Save state | Dopo l'implementazione: determinismo video/audio/macchina sulle quattro board, dati troncati e incompatibili, cicli ripetuti e interazione col networking. |
| Prestazioni e distribuzione | Misure di memoria e velocità sugli hardware target e controllo finale degli avvisi/licenze prima di ampliare la distribuzione. |
