# Menu e controlli: riferimento Supermodel

Linee guida per le milestone 2 e successive di [PORTING_PLAN.md](PORTING_PLAN.md).
Il primo core software della milestone 2 è implementato: vedere [LIBRETRO.md](LIBRETRO.md).
Sono implementate anche le Core Options Video dei renderer Vulkan/OpenGL, SRAM,
NVRAM, profili di controllo, Netpacket sperimentale, rumble e bilanciamento
audio. Le funzioni ancora aperte sono indicate nella roadmap.

## Struttura per gli aggiornamenti upstream

Requisito dell'utente: facilitare l'integrazione delle versioni future di
`dmanlfc/sm2-emu`, mantenendo la struttura originale e riducendo le divergenze.

- Concentrare il futuro adattatore in `src/libretro/`, con target CMake proprio:
  ABI e ciclo di vita, callback, opzioni, input e persistenza del frontend.
  Separare questi compiti in file quando serve, evitando un unico file enorme
  oppure un framework generico anticipato.
- Riutilizzare loader, macchina, audio e renderer. L'adattatore dipende dalle
  interfacce della macchina; CPU e hardware restano indipendenti da Libretro.
  Aggiungere solo gli accessi neutrali indispensabili, senza copiare il motore
  né distribuire condizioni `LIBRETRO` nei sorgenti di emulazione.
- Conservare nomi, percorsi e formattazione upstream. Limitare gli agganci al
  build system; preferire impostazioni CMake per target. Tenere le dipendenze
  del frontend fuori dai target condivisi e preservare la build standalone.
- Usare i metadati originali dei giochi; tenere descrittori e preferenze del
  frontend nell'adattatore, senza duplicare l'intero database upstream.

Punti di integrazione introdotti dalla milestone 1: opzioni e condizioni CMake
per separare standalone/headless; accesso neutrale `sound_board()` su
`Model2MachineBase` e implementazioni nelle quattro varianti. Il runner vive
in `src/headless/` e il confronto in `scripts/compare-headless.py`. Mantenere
questo inventario aggiornato quando cambia la superficie modificata del motore.
La milestone 2 aggiunge `src/libretro/` e due agganci CMake (opzione e
subdirectory). La persistenza frontend aggiunge alla macchina soltanto viste
neutrali su backup RAM ed EEPROM; formato, import e opzioni restano nell'adattatore.
La GPU aggiunge `render/vk/pass_context.h`, implementato dal contesto standalone
e dall'adattatore. I passaggi tilemap/poly3D dipendono da questa interfaccia
anziché dal contesto con finestra; shader e algoritmi restano condivisi.
CMake abilita gli shader anche per Libretro Vulkan/OpenGL e compila i passaggi
con entry point risolti dal frontend. Nessun nuovo accesso alla macchina/CPU.
OpenGL riusa i passaggi upstream GL e riceve dal frontend funzioni e framebuffer;
non introduce una finestra SDL né una copia dei renderer nell'adattatore.
Gli enhancement GPU del commit upstream `ce59cf5` restano nei passaggi e negli
shader condivisi: l'adattatore espone le scelte tramite Core Options e passa
solo gli identificatori ai renderer. xBR/ScaleFX opera sulle tilemap 2D prima
della composizione col 3D; aspect, scaling finale e shader dell'immagine
completa restano responsabilità del frontend.
Il collegamento tra cabinet usa l'interfaccia neutrale upstream `CommTransport`.
`M2Comm` possiede sia il trasporto sia il protocollo ad anello; la macchina
espone soltanto la communication board comune alle quattro varianti hardware.
`src/libretro/netpacket.*` adatta i frame completi all'interfaccia ufficiale
Libretro Netpacket. L'unica estensione al contratto upstream è `ready()`, che
impedisce al timer di collegamento di partire prima che sia completo il roster
dei cabinet richiesti. L'adattatore riusa dal core Supermodel handshake,
conteggio atteso e roster ordinato; ogni frame Model 2 viene inviato soltanto al
successore, lasciando a `M2Comm` il protocollo hardware ad anello. Per i
trasporti standalone `ready()` coincide con `connected()`. Il motore non
include header Libretro e il core non apre socket. Senza trasporto esterno resta
attivo il `LoopbackTransport` dello standalone per un solo cabinet.

Eccezione hardware condivisa con lo standalone: il workaround BEL in
`src/hw/model2c.cpp` mantiene i parametri temporanei della calibrazione anche
quando la NVRAM contiene già il preset prodotto dal workaround stesso. Senza
questo riconoscimento, `Automatic Initial NVRAM Setup` rende i byte persistenti
validi ma impedisce al percorso precedente, limitato ai byte `0xff`, di
inizializzare la trasformazione usata dal mirino durante la partita. Valori di
calibrazione differenti dal preset non vengono sostituiti.

Correzioni hardware selettive riprese dall'upstream 0.9.7 senza aggiornare la
baseline completa: `b93e73c` lascia scollegati i canali analogici non dichiarati
su Model 2A/2B/2C, affinché restituiscano il valore aperto `0xff`; il solo hunk
Model 2B di `f415009` completa in ordine le scritture INTENA differite usate da
`overrevb` e `overrevba`. Il bilanciamento dello stesso commit resta nel motore
audio condiviso: `Model2Sound` conserva i coefficienti upstream e l'accessore
neutrale `SoundBoard::set_audio_balance_enabled()` consente all'adattatore di
ripristinare unity gain senza introdurre dipendenze Libretro nell'hardware.
L'opzione globale non duplica la tabella e si applica a tutte le schede SCSP.

I metadata di guida sono allineati alle aggiunte upstream fino alla 0.9.9:
`start_gear=4` consegna Daytona e i suoi cloni alla partenza lanciata in quarta
quando la Core Option globale `Automatic Start Gear` è abilitata; disabilitandola
i giochi con cambio a quattro marce partono in prima. L'opzione è abilitata per
default per seguire il comportamento upstream. `drive_protocol` distingue
Daytona, STCC e Sega Rally senza modificare i
profili RetroArch. Il parser conserva entrambi in `GameSpec`; l'adattatore usa
`start_gear` durante l'inizializzazione dell'input e `drive_protocol` nel decoder
frontend-neutral importato dall'upstream. La macchina conserva tutte le scritture
drive-board del frame e il rumble Libretro riceve l'effetto decodificato senza
dipendere da SDL o da un dispositivo specifico.

Il clone `daytonam` mantiene il metadato upstream `protection="daytona-maxx"`.
Il parser lo traduce in un tipo di protezione neutrale e `Model2Original`
decodifica la sola finestra PIC/ROM a `0x00240000`, seguendo la macchina a stati
documentata da MAME. Il frontend non contiene condizioni specifiche per MAXX.

### Versioni locali che distinguono parent e clone

`games.xml` integra versioni descrittive ricavate dai commenti e dalle
registrazioni MAME per evitare che parent e clone risultino indistinguibili:

- `daytona`: `Revision A`; `daytona93`: `1993, Deluxe`;
- `stcc`: `Newer`; `stcco`: `Original`;
- `vstriker`: `Revision A`; `vstrikero`: `Original`.

Questi campi sono una personalizzazione locale dei soli metadata di
presentazione. Se l'upstream introdurrà una distinzione equivalente, adottare la
soluzione upstream; diversamente conservarli durante i futuri allineamenti.

Per ogni aggiornamento, registrare il commit upstream integrato e i riferimenti
dei submodule, rivedere API e metadati modificati e risolvere i conflitti nel
minor numero di punti possibile. Quando autorizzati, tenere distinti i commit
di import upstream, degli accessi generici e dell'adattatore. Lavorare su `main`
rimane compatibile con questa separazione delle modifiche.

Confrontare il port con una build originale della **stessa revisione upstream**:
build standalone e headless, frame/audio/NVRAM sulle quattro varianti e, quando
disponibile, prove reali in RetroArch. Conservare anche il confronto tra vecchia
e nuova baseline: una correzione upstream può cambiare legittimamente gli hash.
La separazione limita i conflitti, ma ogni aggiornamento richiede comunque
revisione e validazione.

## Riferimento verificato

Su richiesta dell'utente, usare il progetto personale
`Zer0one/Libretro-Supermodel`, ramo `supermodel-modern`, come riferimento di
organizzazione e usabilità. Consultato il 9 settembre 2026 al commit
`c99444be536a09f27a1f3dec2731b9ef42b99cdf`, nel checkout adiacente
`../libretro-supermodel-modern`. Il progetto rimane separato e si consulta
in sola lettura; non diventa una dipendenza di SM2.

Fonti nel checkout Supermodel:

- `Src/OSD/libretro/libretro_core_options.h`: categorie, nomi, valori iniziali
  e spiegazioni delle opzioni.
- `Src/OSD/libretro/libretro.cpp`: registrazione Core Options v2, visibilità
  contestuale e descrittori dei dispositivi.
- `Docs/CONTROL_PROFILES.md`: profili per gioco, guida, lightgun e Test/Service.
- `Config/Games.xml` e `Src/OSD/libretro/LibretroInputProfiles.h`: firme dei
  giochi e nomi effettivi dei profili usati nel confronto fra le generazioni.
- `Docs/README.md`: salvataggi gestiti dal frontend e impostazioni NVRAM.

I nomi dei comandi originali e gli alias aggiuntivi riprendono preferenze
esplicite dell'utente. Le altre convenzioni osservate in Supermodel sono
una guida da adattare alle capacità effettive di Model 2.

## Organizzazione delle opzioni

Adottare Core Options v2 con l'ordine di categorie di Supermodel:
**System, Video, Audio, Input, CPU**. Pubblicare una categoria solo quando
contiene opzioni implementate e utili; il primo core software può averne meno.
Usare nomi coerenti per funzioni equivalenti e chiavi proprie con prefisso
`sm2_`, stabili dopo la prima pubblicazione. Verificare il comportamento sui
frontend target quando categorie o visibilità dinamica non sono disponibili.

| Categoria | Direzione per SM2 | Quando |
| --- | --- | --- |
| System | Persistenza per gioco; eventuale inizializzazione della sola NVRAM nuova e override espliciti dei campi verificati | Milestone 3; nessun preset prima della validazione |
| Video | Risoluzione nativa 496 × 384; aspect automatico 4:3/16:9 dalla NVRAM con override; selezione dei mirini; scala interna 1×–4×, filtro texture 3D e xBR/ScaleFX sulle tilemap solo con GPU integrata | Implementato |
| Audio | Riproduzione fedele al rate della scheda; eventuali regolazioni specifiche solo se il mixer le supporta e sono utili | Nessuna opzione obbligatoria nel primo core |
| Input | Profili per gioco, modalità delle sorgenti di puntamento, cambio e regolazioni separate di sterzo/acceleratore/freno | Milestone 3 |
| CPU | Soltanto scelte di esecuzione effettivamente disponibili e verificate | Rinviata; nessun overclock o JIT presunto |

Ogni descrizione deve spiegare effetto, giochi/dispositivi interessati e
applicazione immediata oppure dopo riavvio del contenuto. I valori iniziali
devono preservare fedeltà e comportamento della macchina. Le funzionalità
sperimentali vanno indicate come tali, con i limiti verificati.

Supermodel mantiene le regolazioni di guida visibili per prevedibilità,
applicandole soltanto ai profili Driving. Tutte le opzioni non NVRAM restano
sempre visibili; la descrizione dichiara gli eventuali giochi o profili ai quali
si applicano. Soltanto i campi NVRAM sono filtrati per gioco. I descrittori dei
controlli devono sempre corrispondere al gioco caricato e una voce non
applicabile non deve alterare altri profili.

`Aspect Ratio` è una Core Option Video globale con default unico `Auto`.
L'adattatore legge il tipo di cabinato dalla EEPROM attiva per le famiglie
Indy 500 e STCC e comunica 4:3 o 16:9 al frontend con
`RETRO_ENVIRONMENT_SET_GEOMETRY`; 4:3 e 16:9 restano override manuali. Questa
logica appartiene all'adattatore perché riguarda la presentazione del frame,
senza modificare renderer, macchina emulata o formato della NVRAM.

## Controlli e preferenze da conservare

Il catalogo proposto per il punto 3.1 è in [CONTROL_PROFILES.md](CONTROL_PROFILES.md),
con firme risolte, varianti, copertura dei set e lacune da verificare.

- Per giochi Model 2 e seguiti Model 3 con controlli equivalenti, conservare
  lo stesso profilo Supermodel: nome, posizioni sul pad e convenzioni delle
  opzioni. Differenze reali aggiungono o rimuovono solo le azioni pertinenti;
  bit e canali diversi si traducono nell'adattatore. Consultare la matrice in
  CONTROL_PROFILES.md, inclusi Soccer e il caso Handbrake di Sega Rally.
- Ricavare i profili dai metadati SM2 in `data/games.xml`, compresi cablaggio,
  polarità, riposo e calibrazione. Non trasferire maschere o valori ADC Model 3:
  in Model 2 anche i canali di sterzo e pedali cambiano tra giochi.
- Nomi basati sulla funzione del comando originale; preservare i comandi
  canonici e aggiungere alias senza sostituirli. Identificare i pulsanti del
  pad anche per posizione South/East/West/North, evitando ambiguità A/B.
  Le etichette Star Wars Trilogy restano un esempio di precisione, non un
  insieme di azioni da introdurre nei giochi Model 2.
- Usare il remapping nativo del frontend. Come riferimento Supermodel,
  prevedere Service su L3 e Test su R3 rimappabili, con variante del dispositivo
  senza questi slot. Il Model 2 attualmente supportato espone soltanto Service A
  e Test A sul Player 1. Verificare eventuali conflitti per ogni profilo.
- Per la guida, il cambio H-Gate/Standard è disponibile dove esiste il selettore
  a quattro marce, mantenendo i comandi sequenziali pertinenti. Lo sterzo resta
  rimappabile e offre risposta Linear/Progressive/FBNeo; sterzo, acceleratore e
  freno hanno intervalli indipendenti con default 100%. Le trasformazioni
  preservano la calibrazione del gioco. I preset numerici particolari di
  Supermodel non sono copiati senza misure SM2.
- Per le pistole, prevedere modalità Standard con sorgenti combinate,
  Lightgun, Mouse, Mouse + Analog Stick e Analog Stick, una volta verificate.
  Distinguere coordinate assolute, movimento relativo e arbitraggio degli
  assi; combinare con OR soltanto pulsanti della stessa azione. Evitare
  suffissi che attribuiscano a un solo dispositivo un'azione condivisa.
- Tenere P1/P2 indipendenti. Ricarica fuori schermo, fuoco secondario e
  calibrazione dipendono dal titolo: non generalizzare il comportamento
  di The Lost World o Star Wars di Supermodel ai giochi Model 2.
- Un profilo sconosciuto richiede fallback esplicito e avviso nel log, senza
  assegnare per somiglianza i controlli di un altro cabinet.
- Una porta 2 limitata a Coin/Start mantiene invariato il profilo del gioco; i
  descrittori disponibili sulla porta esprimono la limitazione senza creare una
  famiglia di profilo separata.
- Air Walkers pubblica quattro porte con lo stesso profilo approvato. Il backend
  mantiene quattro ingressi logici e riproduce la matrice reale: la porta F
  seleziona P1/P2 oppure P3/P4 sulle porte C/D e sulle due linee Start. Questo
  adattamento resta frontend-neutral; l'adattatore Libretro si limita a
  raccogliere le quattro porte RetroPad.

## Persistenza e responsabilità del frontend

La memoria persistente è esposta come SRAM Libretro in un contenitore versionato
che identifica il set e protegge il payload con checksum. Il `.srm` corrente ha
precedenza; in sua assenza il core importa le memorie native senza riscriverle.
La directory save restituita dal frontend viene usata direttamente: il core non
aggiunge un proprio livello `sm2-emu/<gioco>`. Il `.srm` gestito dal frontend e
gli eventuali `<gioco>.nv`/`<gioco>.eeprom` nativi condividono quindi la stessa
directory, anche quando RetroArch l'ha già organizzata per nome del core.
Questo non equivale ai save state.

I save state riusano la serializzazione frontend-neutral upstream 0.9.8. Le
macchine espongono la stessa immagine `SM2STATE` sia su file sia in memoria;
l'adattatore Libretro implementa soltanto `retro_serialize_size`,
`retro_serialize` e `retro_unserialize`, lasciando slot e nomi dei file al
frontend. Il buffer Libretro è fisso a 9 MiB: RetroArch 1.22.2 interroga la
dimensione prima del caricamento del contenuto e considera uno zero come
funzione assente. Il payload reale resta più piccolo ed è completato con zeri.
Il core negozia inoltre `RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS` per
dichiarare la dipendenza da piattaforma ed endianness; il rifiuto del comando
da parte di un frontend precedente costituisce il fallback e non modifica il
formato né i callback di serializzazione.

Magic, versione, gioco e scheda vengono verificati prima di modificare la
macchina. Un payload troncato viene applicato in modo transazionale e causa il
ripristino dello snapshot precedente. Il formato del core è versione 4: separa
lo stato off-screen seriale dalle coordinate lightgun e include la matrice P3/P4
di Air Walkers e lo stato di `Enhanced Audio Balance`, assenti nel layout
upstream versione 1. Dopo un caricamento, la macchina invalida le generazioni video; il
solo adattatore svuota audio pendente, cadenza, rumble e latch input del
frontend. Una sessione `Linked Cabinets` rifiuta save/load perché il peer
Netpacket esterno non fa parte dello stato emulato.

`Automatic Initial NVRAM Setup` è Enabled per default. In assenza sia del `.srm`
sia di NVRAM native valide, i parent supportati ricevono un campione completo
validato prima del primo frame; il core vi applica Country/Nation USA o Export e
i valori offline necessari; il core imposta inoltre Daytona su Cabinet=Deluxe e
Super GT 24h su I/O Type=C, senza attribuire questo valore al default originale
del gioco. Il campione
stabilisce subito un layout integro e
non richiede una modifica tardiva seguita da riavvio. I salvataggi esistenti
restano sempre prioritari; le successive scritture ordinarie del gioco
continuano a essere esportate dal frontend.

I primi override verificati sono `VF2 Difficulty`, `VF2 Country`, `VF2 Display
Type` e `VF2 Drink`, visibili soltanto per `vf2` e autonomi. Come in Supermodel, un interruttore generale `NVRAM Settings`
è Disabled per default; quando è Enabled, ogni parametro mostra soltanto i valori
reali e viene applicato al caricamento. La descrizione di Country documenta
l'effetto collaterale del menu Service originale senza imporlo nel core. Gli
override aggiornano il CRC e non toccano altri campi o calibrazioni.

La scheda lightgun seriale riceve coordinate e stato off-screen come segnali
distinti. L'adattatore Libretro alimenta il secondo esclusivamente da
`RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN` o dalle scorciatoie di ricarica
esplicite; un Mouse arrivato al bordo resta quindi on-screen. La deduzione MAME
basata sul 5% degli estremi calibrati resta confinata all'adattatore standalone,
che non dispone del segnale Libretro. Nel core Libretro la Core Option
`Mouse Edge Off-Screen Reload`, disabilitata per default, può applicare la stessa
soglia esclusivamente a un colpo Mouse Left; non converte in off-screen gli assi
RetroPad né le sole coordinate al bordo.

Affidare al frontend remapping, opzioni per gioco, shader, volume generale,
pausa, screenshot e sincronizzazione della presentazione. Il core comunica
per default il timing hardware; la modalità esplicita `60 Hz Compatibility`
mantiene la velocità della macchina distribuendo duplicazioni video e pacchetti
audio sulla cadenza del frontend. Evitare una seconda interfaccia ImGui o nuovi parametri
EmulationStation per funzioni già gestite da RetroArch.

## Funzioni da valutare separatamente

PowerPC/JIT, New3D/Legacy3D, DSB, widescreen, threading e collegamento tra
cabinet di Supermodel non dimostrano la disponibilità degli equivalenti SM2.
Anche il supporto standalone SM2 a Vulkan, periferiche SDL/evdev, bordi Sinden
e feedback richiede un adattamento alle API e ai driver del frontend.
Rumble e forza direzionale del volante sono capacità distinte: non promettere
force feedback completo da una sola interfaccia di vibrazione.

Il gamepad rumble usa un adattatore isolato in `src/libretro/rumble.*`. Legge
il comando della drive board tramite l'accessore neutrale della macchina e lo
sterzo già acquisito dal frontend, quindi invia strong/weak tramite
`RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE`. La macchina non include header
Libretro e non conosce il dispositivo del frontend. Il modello delle intensità
relative e della durata replica upstream 0.9.7 sulla scala normalizzata
Libretro; il guadagno complessivo resta di RetroArch. La gestione SDL degli effetti temporanei non è
necessaria perché il frontend riceve direttamente lo stato persistente dei due
motori. Reset, unload, errore e disattivazione azzerano sempre entrambi.

Il recoil delle lightgun `evdev` resta una funzione futura specifica della
piattaforma. Non viene mescolato con il gamepad rumble né con i descrittori di
input, perché Libretro non espone un comando recoil lightgun equivalente.

## Verifica prima di dichiarare il supporto

Provare in RetroArch categorie, cambi di opzione, riavvii richiesti,
load/unload tra profili diversi e override per gioco. Confermare che i valori
iniziali mantengano la baseline e che i salvataggi esistenti siano preservati.
Verificare comandi canonici e alias, calibrazione e indipendenza P1/P2 con
dispositivi reali sui frontend dichiarati. Una build o un menu correttamente
visualizzato non dimostrano il funzionamento dei controlli.
