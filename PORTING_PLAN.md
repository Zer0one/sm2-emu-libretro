# Roadmap del core SM2-Emu Libretro

Il progetto indipendente `Zer0one/sm2-emu-libretro` sviluppa su `main` e usa
`dmanlfc/sm2-emu` come remote `upstream`. La base iniziale è SM2-Emu 0.9.4,
commit `8b3a468c5b51387093811cb16b076e6fd9289d66`. Il clone mainstream rimane
separato, per build e confronti con il codice originale.

Obiettivo: un core per RetroArch e Batocera, preservando emulazione, timing,
audio e controlli. Ogni traguardo richiede una prova eseguibile; una build
riuscita da sola non dimostra compatibilità o gameplay corretto.

Menu, profili e opzioni seguono [LIBRETRO_DESIGN.md](LIBRETRO_DESIGN.md),
che adatta le convenzioni del progetto personale Supermodel a Model 2.

## 0. Baseline upstream — completata nei limiti indicati

Standalone macOS arm64 compilato e avviato con Vulkan/MoltenVK su Apple M4.
Catture delle sequenze dimostrative di Daytona USA, Sega Rally e Virtua Fighter 2.
Le prove di audio udibile e controlli fisici restano aperte.
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

### 3.1 Catalogo dei profili dai metadati — proposta pronta per revisione

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

### 3.3 Profili di guida — implementati e verificati localmente

- Procedere per varianti 4-Speed + VR4/VR1, Sequential e Motorcycle.
  Per Sega Rally verificare Handbrake prima di esporre il profilo completo
  condiviso con Sega Rally 2; la lacuna è documentata nel catalogo.
- Rispettare canali, calibrazioni, polarità e comandi del cabinet; aggiungere
  cambio e regolazioni di sterzo/acceleratore/freno con default trasparenti.
- Introdurre insieme alle funzioni le relative Core Options v2 e descrizioni.

Criterio: estremi/riposo degli assi, cambio e azioni verificati per variante,
più prova con un dispositivo di guida appropriato per il supporto dichiarato.

Il primo gruppo copre i quattro set Daytona e i cinque set Sega Rally. Espone
`Driving: 4-Speed + VR4` e `Driving: 4-Speed + VR1 + Handbrake`, ciascuno con
la variante predefinita Test/Service e quella ridotta. L'opzione `4-Speed
Shifter` seleziona H-Gate, predefinito, oppure Standard; Neutral e Shift
Down/Up restano sempre disponibili. I test verificano assi, trigger analogici,
VR, cambio e freno a mano IN2. Le ROM parent hanno raggiunto una gara reale in
RetroArch macOS, a 300 km/h Daytona e 127 km/h Sega Rally nel frame acquisito.
La prova con volante fisico resta distinta. I profili Motorcycle sono coperti
nei gruppi Manx TT e Motor Raid descritti sotto.

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
Rispetto alla variante VR2 espone esclusivamente `VR1` su D-Pad Up; cambio,
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

Stato: Joystick (Analog) e famiglia Gun implementati; resta la prova manuale
con periferiche fisiche Lightgun e Mouse.

Criterio: calibrazione, polarità, pulsanti e cambio sorgente verificati; prova
con periferiche reali per le combinazioni dichiarate supportate.

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
una partita in RetroArch macOS con audio e Save RAM validi. La validazione con
Lightgun e Mouse fisici è demandata alle prove manuali dell'utente.

### 3.5 Cabinet speciali e casi da verificare

Primo gruppo completato: `dynabb` e `dynabb97` espongono il profilo condiviso
`Joystick (Standard): Baseball (Dynamite Baseball)`, con joystick, due pulsanti
e `Bat Swing` su Right Analog Y verso il basso per ciascun giocatore. I
controlli ROM-free verificano firma, descrizioni, P1/P2, semiasse e varianti
Test/Service; entrambi i parent hanno raggiunto una partita in RetroArch macOS.

Secondo gruppo completato: `segawski` espone `Special: Water Ski` con Set su
South, Pitch Left/Right su L1/R1 e sui binding secondari West/East, Slide su
Left Analog X e l'alias hardware
`Start / Select Down`. La polarità di Slide è selezionabile tramite Core Option,
con `Inverted` come default. `skisuprg` espone `Special: Ski Super G`
con Inclining su Right Analog X/canale 0 e Swing su Left Analog X/canale 1.
La polarità di Swing è selezionabile tramite Core Option, con `Inverted` come
default, e
Foot Sensor attivi alti su L1/R1. I controlli ROM-free verificano
firme, descrittori, bit, assi e varianti Test/Service. Water Ski ha raggiunto
una gara reale. Ski Super G carica profilo, audio e Save RAM, ma resta bloccato
su `DRIVE BOARD TROUBLE CODE: FF`: il set è preliminary e il database SM2 non
include la ROM drive-board definita da MAME. La relativa emulazione resta un
sottopunto futuro separato dai profili.

Terzo gruppo completato: `topskatr`, `topskatrj`, `topskatru` e `topskatruo`
espongono `Special: Top Skater`. Curving e Slide restano assi distinti su Left
Analog X e Right Analog X. La polarità di Curving è selezionabile tramite Core
Option, con `Inverted` come default; Slide resta invariato. Select Left/Right e
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

Al termine dell'intera macro-attività dei profili: implementare la crosshair,
determinare i dati di calibrazione dei sei parent Gun e integrarli nei campioni
di `Automatic Initial NVRAM Setup`. La crosshair è ora implementata per tutti i
sei parent Gun nei backend Software, Vulkan e OpenGL, con opzione e convenzioni
derivate da Supermodel; restano l'acquisizione delle calibrazioni e la
rigenerazione dei campioni iniziali.

### 3.6 Persistenza NVRAM/EEPROM

Stato: persistenza e matrice dei parent implementate. `RETRO_MEMORY_SAVE_RAM`
contiene backup RAM ed EEPROM in formato versionato; import frontend, fallback
nativo e 196 impostazioni operatore per 35 parent sono coperti da test senza
ROM e da avvii reali consecutivi in RetroArch. `Automatic Initial NVRAM Setup`
carica per questi parent un campione completo validato prima del primo frame,
soltanto in assenza di `.srm` e NVRAM native valide, quindi applica Country/Nation
USA o Export e i valori offline necessari. Restano la validazione esplicita dei
dati di calibrazione e i casi di errore del frontend sulle piattaforme target.

- Verificare riavvio, separazione per gioco, file mancanti, invalidi e directory
  non scrivibili; preservare i salvataggi esistenti.
- Valutare SRAM gestita dal frontend definendo prima formato, precedenza e
  import conservativo dei file nativi. Non confonderla con i save state.
- Rigenerare i campioni iniziali dopo la validazione per ROM set dei dati di
  calibrazione; non inizializzare o modificare automaticamente i salvataggi
  esistenti.
- Eseguire una campagna dedicata a tutti i clone presenti in `games.xml`:
  acquisire per ciascuno un `.srm` nativo da un avvio pulito, con NVRAM Settings
  e Automatic Initial NVRAM Setup disabilitati, quindi confrontarne backup RAM,
  struttura EEPROM, checksum e valori predefiniti con il parent. Se i dati
  coincidono, verificare con un avvio pulito e uno consecutivo che l'ereditarietà
  del campione parent funzioni. Se differiscono, sospendere l'ereditarietà NVRAM
  per quel clone e valutarne separatamente layout, offset delle opzioni e
  campione iniziale tramite service menu prima di abilitarlo.
- Registrare `indy500d` come primo caso noto da approfondire nella campagna:
  dopo l'inizializzazione usa una banca EEPROM specchiata da 44 byte, mentre
  `indy500`/`indy500to` usano 36 byte. Gli offset parent successivi a Country,
  Cabinet Type e Difficulty possono sovrapporsi ai dati specifici della Deluxe;
  per ora non introdurre una correzione non validata.

Criterio: persistenza e import provati con casi positivi e negativi; eventuali
campi NVRAM esposti devono avere valori e precedenza espliciti.

### 3.7 Revisione dei menu e regressione complessiva

- Consolidare le opzioni introdotte nei sottopunti precedenti secondo
  LIBRETRO_DESIGN.md: categorie pertinenti, visibilità, default e riavvio richiesto.
- Verificare cambio gioco/profilo, remapping e opzioni per gioco in RetroArch,
  oltre alle regressioni video/audio/NVRAM sulle quattro schede.

Criterio: matrice dei risultati automatici e delle prove fisiche. La milestone
3 completa richiede prove di guida, combattimento e lightgun con almeno un
controllo fisico appropriato per le categorie dichiarate supportate.

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
pedali normali/invertiti e Throttle motociclistico. Restano da introdurre:

- recoil della pistola e intensità;
- rumble e force feedback, distinguendo le capacità realmente offerte da
  Libretro da quelle direzionali dello standalone.

Come in Supermodel, le quattro regolazioni di guida devono restare sempre
visibili e non produrre effetti fuori dai profili Driving.

Altre opzioni da valutare separatamente:

- abilitazione dell'emulazione audio, solo se produce un risparmio reale;
- volume separato della musica DSB/MPEG, se il mixer conserva flussi distinti;
- network board e numero di cabinet dopo un trasporto Libretro Netpacket;
- widescreen reale, con modifica di viewport 3D e composizione 2D;
- filtri di upscaling 2D e supersampling;
- adattamento colore CRT specifico Model 2, solo con una necessità misurata.

Queste voci sono una roadmap, non funzionalità dichiarate. Renderer Model 3,
PowerPC/JIT e DSP specifici di Supermodel restano esclusi perché non applicabili.

## 4. Compatibilità e build di distribuzione

Prima matrice CI implementata: Linux x86_64, macOS arm64/Intel e Windows
x86_64, con software + Vulkan, controlli ABI senza ROM e pacchetti corredati
dal database giochi. Evidenze e limiti in [CI.md](CI.md). Linux arm64 resta
da aggiungere e la milestone completa richiede ancora la matrice estesa sotto.

- Estendere le regressioni a Model 2/2A/2B/2C, ROM parent/clone, ZIP/7z e
  caricamenti falliti; includere più giochi e sequenze rappresentative.
- Automatizzare build macOS arm64 e Linux x86_64/arm64, controlli ABI e test
  eseguibili senza ROM. Le ROM restano esterne al repository e alla CI.
- Preparare core info e istruzioni di installazione; provare l'artefatto su
  RetroArch e Batocera reali, mantenendo lo standalone disponibile.
- Misurare prestazioni e memoria su hardware target. Ottimizzare senza
  semplificare emulazione o alterare la fedeltà.

Criterio: artefatti installabili e testati sulle piattaforme dichiarate, con
matrice di compatibilità e limiti documentati. Prima di allargare la
redistribuzione, verificare i termini dei componenti e conservare gli avvisi.

## 5. Rendering GPU integrato nel frontend — Vulkan e OpenGL implementati

- Scegliere il backend dopo aver verificato i contesti realmente disponibili
  nelle build RetroArch target. Non assumere OpenGL 4.3 su macOS.
- Separare rendering da finestra, swapchain e presentazione possedute dall'OSD;
  usare il contesto fornito dal frontend e gestirne perdita e ricreazione.
- Confrontare la resa con la baseline software, compresi scaling e trasparenze.

Criterio: rendering hardware in RetroArch con recupero del contesto e
confronti visivi documentati, mantenendo il percorso software funzionante.

Stato: adattatori Vulkan e OpenGL implementati con passaggi/shader upstream condivisi,
contesto del frontend, Core Options Video e risoluzioni 1×–4×. VF2 provato in
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

Da fare: validazione su Windows 11 con GPU dedicata e RetroArch reale. La prova
deve distinguere build/ABI, caricamento del core e resa sulla GPU fisica; coprire
OpenGL e Vulkan, scale 1×/4×, lifecycle e persistenza SRAM, conservando log e
screenshot. L'accesso remoto può usare SSH per trasferimenti e comandi, con il
test grafico avviato nella sessione desktop dell'utente.

## 6. Save state e funzioni avanzate

- Inventariare tutto lo stato di CPU, coprocessori, RAM, FIFO, video, audio,
  timer e I/O; definire un formato versionato e controlli sui dati in ingresso.
- Implementare serializzazione e ripristino completi, verificando determinismo
  dopo il caricamento e rifiuto sicuro degli stati incompatibili.
- Solo dopo: valutare rewind e run-ahead. Netplay richiede verifiche ulteriori.

Criterio: dopo save/load, la stessa sequenza di input produce gli stessi frame
e campioni audio su prove rappresentative delle quattro varianti.

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
