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
  generiche e 18 gruppi proposti, con matrice Model 2 / Model 3 e casi da verificare.

Criterio: copertura del catalogo e revisione dei profili proposti. Nessuna
nuova mappatura, opzione o modifica alla persistenza in questo sottopunto.

### 3.2 Riconoscimento e controlli digitali

- Implementare il riconoscitore secondo il catalogo revisionato, con fallback
  esplicito; registrare dispositivo e descrittori pertinenti al gioco.
- Completare Joystick (Standard), comprese Fighting/Soccer, e Joystick (Twin),
  preservando azioni e alias e verificando la permutazione Soccer in partita;
  aggiungere le varianti comuni con/senza Test/Service rimappabili.

Criterio: test del riconoscitore sui set catalogati, nessun controllo improprio,
P1/P2 indipendenti e prova dei profili digitali in RetroArch.

### 3.3 Profili di guida

- Procedere per varianti 4-Speed + VR4/VR1, Sequential e Motorcycle.
  Per Sega Rally verificare Handbrake prima di esporre il profilo completo
  condiviso con Sega Rally 2; la lacuna è documentata nel catalogo.
- Rispettare canali, calibrazioni, polarità e comandi del cabinet; aggiungere
  cambio e regolazioni di sterzo/acceleratore/freno con default trasparenti.
- Introdurre insieme alle funzioni le relative Core Options v2 e descrizioni.

Criterio: estremi/riposo degli assi, cambio e azioni verificati per variante,
più prova con un dispositivo di guida appropriato per il supporto dichiarato.

### 3.4 Puntamento e joystick analogico

- Implementare la famiglia Gun con varianti interne seriale/posizionale e
  Missile, oltre a Joystick (Analog); conservare i comportamenti fisici distinti.
  I nomi Gun (Lightgun)/Gun (Mouse) identificano la modalità di sorgente.
- Verificare P1/P2, trigger, eventuale ricarica, modalità delle sorgenti e mirini;
  introdurre le opzioni solo quando il relativo comportamento funziona.

Criterio: calibrazione, polarità, pulsanti e cambio sorgente verificati; prova
con periferiche reali per le combinazioni dichiarate supportate.

### 3.5 Cabinet speciali e casi da verificare

- Trattare singolarmente Baseball, Water Ski, Ski Super G, Top Skater e Wave
  Runner; riesaminare i tre casi generici 1P sulla base di dati verificati.
- Verificare e allineare gli I/O di Air Walkers alla matrice MAME P1-P4,
  includendo multiplexing e linee Start/Coin, prima di implementare il profilo
  a quattro giocatori.
- Integrare ciascun profilo con i suoi test senza estendere implicitamente le
  conclusioni agli altri cabinet o confondere input supportati e gioco funzionante.

Criterio: chiusura documentata per ogni profilo; i casi non verificati restano
indicati nel catalogo e nel frontend.

### 3.6 Persistenza NVRAM/EEPROM

Stato: prima iterazione implementata. `RETRO_MEMORY_SAVE_RAM` contiene backup
RAM ed EEPROM in formato versionato; import frontend, fallback nativo e prime
opzioni verificate (`vf2` Difficulty/Country/Display Type/Drink) sono coperti da test ROM-free e da avvii
reali consecutivi in RetroArch. Restano la matrice degli altri parent e i casi
di errore del frontend sulle piattaforme target.

- Verificare riavvio, separazione per gioco, file mancanti, invalidi e directory
  non scrivibili; preservare i salvataggi esistenti.
- Valutare SRAM gestita dal frontend definendo prima formato, precedenza e
  import conservativo dei file nativi. Non confonderla con i save state.
- Eventuali preset iniziali o campi configurabili richiedono validazione per
  ROM set; nessuna inizializzazione automatica dei salvataggi esistenti.

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

Opzioni legate ai controlli, da introdurre soltanto con i relativi profili:

- modalità Lightgun/Mouse/Analog Stick, mirini e ricarica fuori schermo;
- recoil della pistola e intensità;
- cambio a quattro marce H-Gate/Standard;
- risposta e intervallo di sterzo, acceleratore e freno;
- rumble e force feedback, distinguendo le capacità realmente offerte da
  Libretro da quelle direzionali dello standalone.

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
