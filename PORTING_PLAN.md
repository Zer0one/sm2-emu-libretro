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

## Implementazioni ancora aperte

Nessuna. Le prove senza nuovo sviluppo sono raccolte esclusivamente nella
sezione finale.

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
Start con binding secondario su D-Pad Down. La polarità Bank segue il metadato
invertito. La ROM parent ha raggiunto il controllo iniziale del motion slider
in RetroArch macOS.

Il quinto gruppo aggiunge `motoraid` e `motoraiddx` come `Driving: Sequential
(Motor Raid)`. Il gioco non ha un cambio: Kick usa South/L1 e Punch usa East/R1,
con i dorsali come binding secondari sugli stessi due ingressi arcade. Le
posizioni corrispondono al foglio approvato. La ROM parent ha raggiunto una gara in RetroArch macOS;
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
Coordinate e stato off-screen sono separati fino alla scheda seriale emulata:
Libretro usa `IS_OFFSCREEN`, mentre raggiungere il bordo con il Mouse non attiva
più implicitamente la ricarica. La Core Option `Mouse Edge Off-Screen Reload`,
Disabled per default, consente di ripristinare esplicitamente il comportamento
MAME per Mouse Left entro il 5% dei bordi.

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
una gara reale. Ski Super G carica profilo, audio e Save RAM, ma l'emulazione
comune non risponde al controllo della Drive Board esterna e raggiunge
`DRIVE BOARD TROUBLE CODE: FF`. La Core Option specifica `Drive Board Error
Bypass (Restart Required)`, disabilitata per default, riproduce la singola
pressione di Test verificata su quella schermata. Abilitandola, la ROM parent ha
raggiunto la sequenza sciabile in RetroArch macOS; disabilitandola resta ferma
sull'errore come upstream. Il bypass non dichiara emulata la scheda o la
protezione e non modifica ROM, NVRAM o binding.

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

Quinto gruppo completato: `airwlkrs` espone P1-P4 con
`Joystick (Standard): Basketball (Air Walkers)`; la matrice I/O commuta P1/P2
e P3/P4 sulle porte C/D insieme alle rispettive linee Start, mentre Coin 1-4
restano indipendenti. `rascot2` espone
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

### 3.6 Persistenza NVRAM/EEPROM — completata

Stato: persistenza e matrice dei parent implementate. `RETRO_MEMORY_SAVE_RAM`
contiene backup RAM ed EEPROM in formato versionato; import frontend, fallback
nativo e 289 impostazioni operatore per 35 parent più quattordici cloni con menu
specifico sono coperti da test senza
ROM e da avvii reali consecutivi in RetroArch. `Automatic Initial NVRAM Setup`
carica per questi parent un campione completo validato prima del primo frame,
soltanto in assenza di `.srm` e NVRAM native valide, quindi applica Country/Nation
USA o Export, i valori offline necessari e I/O Type C per Super GT 24h.

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
del frontend. L'overlay riutilizza il pannello Dear ImGui di Supermodel,
composto dal core nel frame Software, Vulkan o OpenGL, e riporta medie su 61
callback per macchina, video, audio, `retro_run`, frame peggiore, cadenza
effettiva e capacità stimata. `Frame Skip` è stato escluso per scelta progettuale.

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
di guida restano silenziosi. Force feedback direzionale per volanti e recoil
lightgun non dispongono di un'API Libretro dedicata: eventuali backend `evdev`
specifici Linux appartengono al frontend o a componenti esterni, non al core.

Come in Supermodel, le quattro regolazioni di guida devono restare sempre
visibili e non produrre effetti fuori dai profili Driving.

`Enhanced Audio Balance` è implementata come unico interruttore globale nella
categoria Audio. Enabled applica automaticamente i profili SCSP upstream 0.9.7
a tutti i set supportati; Disabled ripristina unity gain in tempo reale. Il fix
INTENA di `overrevb`/`overrevba` resta sempre attivo perché corregge il timer
audio della macchina e non appartiene al mastering opzionale.

`Music Volume` segue la convenzione del core Supermodel: 0–200% a passi di 10,
100% per default e applicazione immediata. Regola soltanto il contributo MPEG
della scheda musicale separata, senza alterare SCSP, effetti o voci. È attiva
sulle famiglie STCC (DSB Z80) e Top Skater (DSB2); sugli altri set resta inerte.

`Aspect Ratio` è implementata con `Auto` come default globale e override `4:3`
o `16:9`. In Auto legge la configurazione attiva dalla NVRAM: Indy 500 e STCC
usano 16:9 nei rispettivi cabinati Deluxe e 4:3 in Twin; gli altri giochi usano
4:3. Il framebuffer resta 496×384 e il frontend riceve soltanto la geometria.

Renderer Model 3, PowerPC/JIT e DSP specifici di Supermodel restano esclusi
perché non applicabili.

## 4. Compatibilità e build di distribuzione

Matrice CI implementata: Linux x86_64, Linux arm64, macOS arm64/Intel e Windows
x86_64, con Software + Vulkan + OpenGL, controlli ABI senza ROM e pacchetti
corredati dal database giochi. Linux arm64 usa il runner GitHub nativo
`ubuntu-24.04-arm` e pubblica `sm2-libretro-linux-arm64`, seguendo lo stesso
percorso degli altri Linux senza cross-compilazione o emulazione. Evidenze e
limiti in [CI.md](CI.md). `sm2_libretro.info`, database e istruzioni sono già
inclusi nei pacchetti. Il controllo di distribuzione verifica inoltre licenze
del core e delle dipendenze incorporate, revisione sorgente, checksum, struttura
del pacchetto e assenza di ROM. La prima esecuzione remota del nuovo job richiede
il prossimo commit/push autorizzato.

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
Dettagli, limiti e avvio con MoltenVK aggiornato: [GPU.md](GPU.md).

## 6. Networking, save state e funzioni avanzate

### 6.1 Collegamento tra cabinet — tutti gli schemi censiti implementati

- Usare direttamente la struttura upstream 0.9.7: `M2Comm` possiede il
  `CommTransport` e conserva il protocollo della communication board Model 2.
- Adattare a RetroArch il solo trasporto, sostituendo UDP con Netpacket; la
  piccola estensione `ready()` attende il peer prima di avviare il timer del link.
- Usare l'interfaccia ufficiale Libretro Netpacket: nessun socket, discovery o
  configurazione di rete appartiene al core.
- Conservare il comportamento standalone a cabinet singolo quando la Core
  Option `Linked Cabinets` è lasciata su `Disabled`, valore predefinito.
- Esporre `Linked Cabinets` per contenuto come le opzioni NVRAM: ogni set
  supportato mostra soltanto la propria scelta e non la propaga agli altri giochi.

Il trasporto è abilitato per le famiglie Daytona USA e STCC. Daytona accetta da
due a otto partecipanti; STCC arriva a nove includendo il Relay. Riusa dal core
Supermodel handshake broadcast, verifica del
numero atteso e roster ordinato. Il payload Model 3 non viene copiato: ciascun
frame completo Model 2 viene inviato al successore del roster, conservando in
`M2Comm` il protocollo ad anello. Host e client usano lo stesso ROM set e core,
salvo il programma Relay `vonr` nella configurazione Virtual On a tre istanze;
`NVRAM Settings` imposta `Link ID=Master`/`Car Number=1` sull'host e
`Link ID=Slave`/`Car Number=2..8` sui client. Un test reale
con due istanze RetroArch 1.22.2 su macOS ha formato l'anello con ID `01/02` e
`02/02`, eseguito 3600 frame per istanza e avviato una gara condivisa. Le
catture finali mostrano l'host rosso `2nd/2`, con l'auto blu `2P` davanti, e il
client blu `1st/2`, entrambi in movimento nella stessa sessione.
I test senza ROM verificano inoltre anelli emulati da 2, 3, 4 e 8 nodi,
assegnazione univoca degli ID, conteggio, ordine predecessore/successore,
payload integro, roster incompleto, configurazioni discordanti, capacità massima
di nove partecipanti, rifiuto del decimo e disconnessione. Un test RetroArch
isolato a tre istanze ha
inoltre formato il roster reale con partecipanti 0, 1 e 2; tutti i processi si
sono chiusi con codice 0, senza terminazione forzata né residui. La gara
sincronizzata a tre auto resta una verifica separata.

`daytona93` è escluso: il suo Service Menu acquisito contiene soltanto quattro
voci e non espone `Link ID` né `Car Number`. `daytonas`, `daytonase` e
`daytonam` mantengono invece le due impostazioni e restano abilitati fino a
otto cabinet.

Il primo gruppo non-Daytona comprende `stcc`, `stcca`, `stccb` e `stcco`.
I valori NVRAM già acquisiti mappano `Car 1` sull'host, `Car 2`…`Car 8` sui
client giocabili e `Relay` sulla nona istanza. La prova RetroArch massima sul
parent ha salvato nei nove `.srm` le codifiche 1…9, formato il roster 9/9 su
tutte le istanze e chiuso tutti i processi con codice 0, senza kill forzato né
residui. La gara STCC resta nella sezione delle verifiche.

Il gruppo Sega Rally con `Car 1`…`Car 4` e `Relay` usa lo stesso schema combinato
di STCC. `Linked Cabinets` arriva a cinque contando il Relay.
Il parent ha formato il roster massimo 5/5 con `Car 1`…`Car 4` e `Relay`,
codifiche 1…5 verificate negli `.srm`; le revisioni B e C hanno formato roster
reali a due istanze. Tutte le prove sono terminate senza chiusure forzate o
processi residui.
`srallycdx` e `srallycdxa` sono esclusi perché i Service Menu acquisiti non
espongono né `Cabinet Type` né `Link Type`. Entrambi usano cataloghi NVRAM
specifici con le sole quattro voci realmente disponibili; nessuno dei due
riceve la Core Option `Linked Cabinets`. Una gara Sega Rally sincronizzata
resta nella sezione delle verifiche.

Le integrazioni successive vengono organizzate per meccanismo di collegamento,
così l'adattamento e le prove del titolo base coprono nello stesso gruppo anche
i giochi compatibili. Il gruppo con ruoli `Master`/`Slave` e `Cabinet ID`
separato comprende `indy500`, `indy500d`, `indy500to`, `motoraid`,
`motoraiddx`, `waverunr` e `skisuprg`. Indy 500 ammette fino a otto cabinet;
Motor Raid, Wave Runner e Sega Ski Super G fino a quattro. La voce `Live` di
Motor Raid rappresenta il Relay/live monitor: conta nel totale, è ammessa una
sola volta e non viene equiparata a un normale client giocabile.

Le prove a due istanze hanno formato il roster per tutti questi set salvo
`motoraiddx`: il clone DX stabilisce la connessione frontend 2/2 e salva
correttamente ruolo e ID, ma non porta la communication board al polling del
roster, coerentemente con il suo stato upstream non funzionante. Tutti i sette
set hanno prodotto `.srm` con `Master/ID 1` sull'host e `Slave/ID 2` sul client,
incluse le copie speculari e il layout dedicato di `indy500d`. La verifica in
gara di Sega Ski Super G richiede ancora di abilitare il bypass opzionale della
schermata `DRIVE BOARD TROUBLE CODE: FF`; questo consente l'avvio ma non emula la
Drive Board.

Motor Raid ha inoltre formato un roster reale 3/3 con `Master`, `Slave` e
`Live`. Gli `.srm` conservano i valori nativi 1, 2 e 3 e gli ID progressivi in
entrambe le copie EEPROM; tutti i processi sono terminati con codice 0, senza
forzature o residui. Il runner espone `--include-relay` per usare Live come
ultima istanza, sempre incluso nel totale selezionato.

Super GT 24h usa `Link Type=Car No.1 Master` sull'host e `Car No.2`…`Car No.4
Slave` sui client; `Link Max` fissa il totale da due a quattro. La prova reale
a due istanze ha formato il roster su entrambe le communication board e gli
SRM conservano `Master/Link Max 2` e `Slave Car No.2`.

Over Rev usa `Link Max=2/3/4 Links` e un `Link Type` che include ruolo e numero.
Il parent e i due cloni `overrevb`/`overrevba` hanno formato roster reali a due
istanze; in tutti e tre i casi le due copie EEPROM contengono rispettivamente
`Master CarNo.1/2 Links` e `Slave CarNo.2/2 Links`.

Manx TT non usa un numero cabinet separato: a due istanze assegna `Master` e
`Slave`; a tre aggiunge `Relay`. Sia `manxtt` sia `manxttc` hanno formato roster
reali 3/3 e salvato rispettivamente i valori NVRAM 1, 2 e 3. `manxttdx` resta
escluso perché il suo Service Menu acquisito non contiene `Cabinet Type` né
`Link Type`.

Virtual On usa `Network Link Attribute=Master/Slave` sui due programmi Twin e
il set dedicato `vonr` come terzo partecipante Relay. Poiché il Relay è un ROM
set distinto, l'adattatore identifica la famiglia Netpacket `von` invece del
nome esatto del set, seguendo la separazione tra famiglia e ruolo già usata dal
core Supermodel. Le prove reali hanno formato sia il roster Twin 2/2 sia il
roster 3/3 con `von`, `von` e `vonr`; gli `.srm` conservano Master, Slave e il
valore No Link nativo del programma Relay. Tutte le istanze si sono chiuse senza
forzature o residui. Le prove distribuite macOS/Batocera hanno inoltre mostrato
il live monitor di Motor Raid e del programma `vonr`, oltre alla schermata Relay
di Manx TT e alle visuali Relay di STCC e Sega Rally. La partita sincronizzata
con input reali resta nella sezione delle verifiche.

### 6.2 Save state — implementato

L'upstream 0.9.8 ha già introdotto `Archive`, la serializzazione delle quattro
varianti di scheda e il ripristino transazionale: magic, versione, gioco e board
sono verificati prima di modificare la macchina e un payload troncato ripristina
lo snapshot precedente. Il frontend standalone aggiunge file e slot propri;
questa parte non va portata nel core, perché RetroArch possiede slot e file.

Sono importati la serializzazione frontend-neutral e i fix-up delle quattro
schede. Il core usa un'immagine in memoria versionata, un buffer Libretro fisso
da 9 MiB compatibile con l'interrogazione anticipata di RetroArch 1.22.2 e non
introduce un secondo sistema di slot. Dopo il caricamento invalida le risorse
video derivate e ripulisce code audio, cadenza, rumble e latch input del
frontend. Stati corrotti, troncati o appartenenti a gioco/board diversi vengono
rifiutati senza alterare la macchina. Save/load viene rifiutato durante una
sessione `Linked Cabinets` perché il peer Netpacket esterno non è serializzabile.

Round-trip e riesecuzione deterministica sono verificati con ROM reali su Model
2, 2A, 2B e 2C. Il core conserva in una coda separata lo stato runtime Libretro
necessario a cambio, controlli relativi, puntamento e cadenza, mantenendo intatto
il formato macchina upstream. RetroArch macOS ha caricato uno stato VF2 nei
renderer Software e Vulkan; la prova manuale dell'utente ha inoltre confermato
save/load su Indy 500 (Model 2B). Il controllo CI impedisce di distribuire
nuovamente un binario serializzabile con
`savestate = "false"`. Venti cicli per ciascuna board verificano inoltre i
pattern run-ahead e rewind; Daytona supera la prova anche a 60 Hz.

## 7. Integrazioni derivate dall'upstream 0.9.8/0.9.9

L'analisi è allineata al commit upstream
`af0be801980e40eb1f7eeff7f72ecc2f8ffe1024` del 19 settembre 2026. Il core
dichiara 0.9.9 e conserva la provenienza della base iniziale 0.9.4.

| Integrazione applicabile | Stato nel core |
| --- | --- |
| Save state 0.9.8 | Integrati serializzatori delle quattro board, contenitore transazionale e adattamento all'API Libretro; file e slot restano al frontend. |
| Daytona `start_gear=4` | Integrato con la Core Option globale `Automatic Start Gear`, abilitata per default. |
| FPS toggle standalone | Coperto da `Timing / FPS Overlay`; tasto e UI SDL esclusi. |
| Configurazione SDL dei volanti | Esclusa: dispositivi, mapping e conversione degli assi appartengono a RetroArch. |
| Drive command 0.9.9 | Integrati coda per frame, protocolli Daytona/STCC/Rally e adattamento Gamepad Rumble. |
| Metadata `games.xml` | Integrati `start_gear`, `drive_protocol`, correzioni `drive_board`, clone Daytona MAXX e aggiornamenti approvati. |
| Bilanciamento audio e timer INTENA | Integrati tramite `Enhanced Audio Balance` e fix macchina permanente. |
| Filtri xBR/ScaleFX | Integrati come filtri dei livelli 2D prima della composizione 3D. |

Tutte le integrazioni applicabili già individuate nelle release upstream
analizzate sono ora recepite o escluse esplicitamente. L'allineamento upstream
non è una voce generica della roadmap: quando `upstream/main` avanzerà, ogni modifica
applicabile diventerà una specifica voce di implementazione; le modifiche già
integrate resteranno nella cronologia e quelle non applicabili saranno indicate
esplicitamente come escluse.

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

- Ripetere un controllo incrociato completo tra il documento autoritativo
  `GAME_SETTINGS_CATALOG.md` e l'implementazione delle NVRAM Settings per ogni
  parent e clone. Confrontare insieme delle opzioni, ordine, descrizioni, valori,
  default e routing parent/clone; verificare esplicitamente anche che le voci
  assenti dal Service Menu non vengano ereditate o rese visibili. Il controllo
  deve includere la selezione del catalogo eseguita dal core, non soltanto la
  compatibilità del layout o delle scritture NVRAM.

Completate il 20 settembre 2026: la verifica strutturale di menu e opzioni cambia
set, parent/clone e profilo senza rilevare voci per gioco residue; la regressione
generale copre le quattro board, parent e clone, ZIP e 7z, input sintetico,
video/audio, cicli load/unload/reset, Save RAM e rifiuto controllato di archivi
mancanti o invalidi. Nessun crash, blocco o salvataggio corrotto è emerso.
Save State, rewind e run-ahead sono inoltre verificati localmente sulle quattro
board con 20 cicli per scheda; il precedente scarto Daytona è risolto salvando
lo stato runtime dell'adattatore Libretro. La qualifica Windows usa l'artifact
CI esatto della revisione `1d8a1ab`: RetroArch 1.22.2 su Radeon Vega 11 completa
VF2 in Vulkan e OpenGL a 1×/4×, con una ripetizione Vulkan dopo il cambio di
backend, uscita regolare, SRAM, audio e screenshot verificati. Restano fuori
dalla prova controller fisici, ascolto soggettivo e GPU/driver differenti. Gli
artifact CI macOS arm64 e Windows x86_64 della stessa revisione hanno inoltre
formato via LAN una gara Daytona 2/2 e un roster STCC 3/3 con il Relay Windows;
tutte le istanze hanno salvato SRAM e screenshot e sono uscite senza forzature
o residui. Una verifica Daytona in gara ha inoltre confermato la corretta ombra
retinata: la dominante bluastra osservata sul televisore era prodotta dalla
modalità Luce notturna di Windows, non dal core o dal backend grafico.
Il worktree corrente è stato infine compilato per Linux x86_64 e provato in una
directory isolata di Batocera 43.1: Daytona ha completato 2300 frame di gara con
OpenGL Core 4.6, audio e SRAM validi, mentre 20 cicli Save State hanno coperto
round-trip, replay deterministico, run-ahead, rewind e rifiuto degli stati
corrotti. Non restano quindi verifiche tecniche specifiche di Windows o
Batocera nella roadmap corrente.

## Vincolo esterno prima della pubblicazione

L'audit di distribuzione è concluso: il pacchetto contiene 15 file previsti,
licenze delle dipendenze, revisione sorgente e checksum completi, senza ROM o
contenuti inattesi. Rimane una decisione del titolare upstream, non una verifica
tecnica locale: `LICENSE` dichiara BSD-3-Clause, mentre 114 intestazioni vietano
l'uso commerciale senza permesso. Prima di un PR o di una pubblicazione esterna
va chiarito quale dei due testi governi quei file.
