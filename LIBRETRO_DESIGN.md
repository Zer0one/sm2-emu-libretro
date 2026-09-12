# Menu e controlli: riferimento Supermodel

Linee guida per le milestone 2 e successive di [PORTING_PLAN.md](PORTING_PLAN.md).
Il primo core software della milestone 2 è implementato: vedere [LIBRETRO.md](LIBRETRO.md).
Sono implementate anche le Core Options Video dei renderer Vulkan/OpenGL, la SRAM
gestita dal frontend e le prime opzioni System per VF2. Profili completi, opzioni Input
e altre funzioni descritte qui restano proposte per i passaggi successivi.

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
| Video | Risoluzione nativa 496 × 384 e aspect 4:3; selezione dei mirini per giocatore; scala interna 1×–4× solo con GPU integrata | Mirini nella 3; scala nella 5 |
| Audio | Riproduzione fedele al rate della scheda; eventuali regolazioni specifiche solo se il mixer le supporta e sono utili | Nessuna opzione obbligatoria nel primo core |
| Input | Profili per gioco, modalità delle sorgenti di puntamento, cambio e regolazioni separate di sterzo/acceleratore/freno | Milestone 3 |
| CPU | Soltanto scelte di esecuzione effettivamente disponibili e verificate | Rinviata; nessun overclock o JIT presunto |

Ogni descrizione deve spiegare effetto, giochi/dispositivi interessati e
applicazione immediata oppure dopo riavvio del contenuto. I valori iniziali
devono preservare fedeltà e comportamento della macchina. Le funzionalità
sperimentali vanno indicate come tali, con i limiti verificati.

Supermodel mantiene le regolazioni di guida visibili per prevedibilità,
applicandole soltanto ai profili Driving. Riprendere questa convenzione quando
implementate; mostrare invece campi NVRAM e opzioni specifiche di un singolo
gioco soltanto nel contesto pertinente. I descrittori dei controlli devono
sempre corrispondere al gioco caricato. Una voce visibile ma non applicabile
deve dichiararlo chiaramente e non alterare altri profili.

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
  prevedere Test su L3 e Service su R3 rimappabili, con variante del dispositivo
  senza questi slot; verificare eventuali conflitti per ogni profilo.
- Per la guida, valutare cambio H-Gate/Standard dove esiste il selettore a
  quattro marce, mantenendo i comandi sequenziali pertinenti. Esporre un asse
  sterzo lineare rimappabile; regolazioni opzionali con default Linear/100% e
  acceleratore/freno indipendenti, senza cambiare la calibrazione del gioco.
  Non copiare i preset numerici particolari di Supermodel senza misure SM2.
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

## Persistenza e responsabilità del frontend

La memoria persistente è esposta come SRAM Libretro in un contenitore versionato
che identifica il set e protegge il payload con checksum. Il `.srm` corrente ha
precedenza; in sua assenza il core importa le memorie native senza riscriverle.
Questo non equivale ai save state.

I primi override verificati sono `VF2 Difficulty`, `VF2 Country`, `VF2 Display
Type` e `VF2 Drink`, visibili soltanto per `vf2` e autonomi. Come in Supermodel, un interruttore generale `NVRAM Settings`
è Disabled per default; quando è Enabled, ogni parametro mostra soltanto i valori
reali e viene applicato al caricamento. La descrizione di Country documenta
l'effetto collaterale del menu Service originale senza imporlo nel core. Gli
override aggiornano il CRC e non toccano altri campi o calibrazioni.

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

## Verifica prima di dichiarare il supporto

Provare in RetroArch categorie, cambi di opzione, riavvii richiesti,
load/unload tra profili diversi e override per gioco. Confermare che i valori
iniziali mantengano la baseline e che i salvataggi esistenti siano preservati.
Verificare comandi canonici e alias, calibrazione e indipendenza P1/P2 con
dispositivi reali sui frontend dichiarati. Una build o un menu correttamente
visualizzato non dimostrano il funzionamento dei controlli.
