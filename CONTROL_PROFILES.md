# Catalogo proposto dei profili Libretro — punto 3.1

Stato: catalogo revisionato; i profili implementati e verificati sono descritti
nella roadmap e in `LIBRETRO.md`.

## Metodo e confini

Analizzato `data/games.xml` della base upstream
`8b3a468c5b51387093811cb16b076e6fd9289d66` attraverso il vero
`rom::GameDatabase::load()` e `games()`, collegando le librerie già compilate.
L'analisi usa quindi i metadati effettivi dopo `merge_clones()`, non una
ricostruzione approssimata dell'ereditarietà XML.

Il database contiene **83 set**, con **36 voci senza parent**. Non sono 83
profili, né necessariamente 36 titoli distinti. Raggruppando input flags,
tipi degli assi, presenza della lightgun, gearbox, shift_buttons e gun_missile
si ottengono **14 firme generiche**. Cablaggio, limiti, riposo, inversione e
bit Start rimangono parametri del singolo set.

Il riferimento Supermodel è `Docs/CONTROL_PROFILES.md` del progetto personale,
ramo `supermodel-modern`, commit `c99444be536a09f27a1f3dec2731b9ef42b99cdf`:
una famiglia condivide una disposizione fisica del RetroPad; ogni gioco
abilita soltanto le proprie azioni. Per SM2 i nomi sotto sono proposte di
catalogo, non etichette già registrate nel frontend.

## Coerenza Model 2 / Model 3

Requisito dell'utente: quando il tipo di controllo resta equivalente fra un
gioco Model 2 e il seguito Model 3, conservare **lo stesso nome del profilo,
le stesse posizioni sul RetroPad e le stesse convenzioni delle opzioni** di
Supermodel. Una differenza nei bit o nei canali hardware richiede una
traduzione nell'adattatore, non un nuovo profilo per l'utente.

La matrice distingue il profilo condiviso dalle azioni disponibili: un seguito
può aggiungere un comando senza spostare gli altri. Le azioni non presenti nel
gioco Model 2 non vanno esposte. Gli alias esistenti restano aggiuntivi e
seguono l'azione, non il numero del bit.

| Model 2 | Riferimento Model 3 | Profilo da conservare | Differenze / stato |
| --- | --- | --- | --- |
| Virtua Fighter 2 | Virtua Fighter 3 / Team Battle | Joystick (Standard): Fighting | Punch, Kick, Guard nelle stesse posizioni; VF2 non ha Escape |
| Fighting Vipers | Fighting Vipers 2 | Joystick (Standard): Fighting | Conservare Punch, Kick, Guard; nessun quarto comando da dedurre dal descrittore Fighting generico di Supermodel |
| Virtua Striker | Virtua Striker 2 e revisioni | Joystick (Standard): Soccer | Stesse azioni Short Pass, Long Pass, Shoot; ordine dei bit Model 2 diverso dall'ordine logico del profilo |
| Virtual On | Virtual-On Oratorio Tangram | Joystick (Twin) | Stessi due stick per un giocatore, grilletti e pulsanti superiori; conservare il nome Dash del Model 2 e indicare Turbo come equivalente Supermodel |
| Daytona USA | Daytona USA 2 / Power Edition | Driving: 4-Speed + VR4 | Stessi quattro rapporti e quattro pulsanti VR; cablaggio e calibrazione restano Model 2 |
| Sega Rally Championship | Sega Rally 2 | Driving: 4-Speed + VR1 + Handbrake | Cambio e VR1 definiti; Hand Brake usa l'ingresso analogico IN2, da 0x00 rilasciato a 0xFF premuto |
| Gunblade NY | L.A. Machineguns | Gun | Famiglia e modalità delle sorgenti condivise; Gunblade dichiara un trigger per giocatore, Supermodel espone Left/Right Shot. Mantenere la variante di azioni del gioco, senza inventare un secondo trigger |

Il confronto non è limitato al nome della serie: vale per ogni equivalenza
verificata. La somiglianza tematica da sola non basta per assegnare, per
esempio, Ski Super G a Ski Champ o le motociclette al profilo Harley-Davidson.

### Convenzioni da mantenere nell'implementazione successiva

- **Fighting:** Kick su South, Punch su East, Guard su West. VF2/Fighting
  Vipers usano IN1/IN2 con maschere 0x01/0x02/0x04. North/Escape non è
  un'azione VF2.
- **Soccer:** Short Pass su South, Long Pass su East, Shoot su West. Su Model 2
  corrispondono rispettivamente a **0x04, 0x01, 0x02** in IN1/IN2. Il driver
  MAME segnala che il menu Service mostra l'ordine standard 1-2-3 mentre in
  partita è 2-3-1: la prova deve verificare le azioni in partita.
- **Twin:** stick sinistro/destro sui corrispondenti assi, Left/Right Shot
  Trigger su L2/R2, Left/Right Dash (Turbo) su L/R. Il cablaggio IN1/IN2 del
  Model 2 descrive le due metà dello stesso giocatore.
- **Driving:** Steering sullo stick sinistro X, Brake su L2, Accelerator su R2,
  Shift Down/Up su L/R; per quattro marce, stesso H-Gate/Standard sullo stick
  destro e Neutral su West. Daytona usa VR1 Red/Down, VR2 Blue/Left,
  VR3 Yellow/Right, VR4 Green/Up. Sega Rally usa VR1 su Down e porta
  `Handbrake (Analog)` su South, traducendolo negli estremi 0x00/0xFF di IN2.
- **Gun:** riusare il nome di famiglia `Gun`; `Gun (Lightgun)`, `Gun (Mouse)`,
  `Gun (Mouse + Analog Stick)` e `Gun (Analog Sticks)` indicano la sorgente
  selezionata come in Supermodel. Tutti i sei parent usano
  coordinate assolute. Virtua Cop, Virtua Cop 2 e House of the Dead le ricevono
  dalla lightgun seriale RS-422 a 10 bit e ricaricano sparando fuori dall'area
  calibrata: South/RB sono `Shot` ed East/LB sono `Reload Offscreen`. Gunblade NY, Rail
  Chase 2 e Behind Enemy Lines usano invece assi posizionali a 8 bit che non
  ritornano al centro e non hanno reload fuori schermo; BEL assegna `Missile`
  a East/LB. L'interfaccia del cabinet resta una proprietà interna del profilo.

La famiglia Gun è implementata per tutti i dieci set parent/clone. La Core
Option `Gun Input Mode` espone Standard, Lightgun, Mouse + Analog Stick, Mouse e
Analog Stick. La traduzione mantiene separati il percorso seriale e quello
posizionale; i descrittori mostrano `Reload Offscreen` soltanto per i tre parent
che lo supportano e `Missile` soltanto per BEL. Per i primi, `Off-Screen Reload
Shortcut` controlla i binding RetroPad East/LB, Mouse destro e Lightgun Reload ed è
attiva per default; il grilletto fisicamente fuori schermo resta indipendente.

### Evidenza e limiti del confronto

Verificati nel checkout Supermodel alla revisione sopra indicata:
`Config/Games.xml`, `Src/OSD/libretro/LibretroInputProfiles.h`,
`Src/OSD/libretro/libretro.cpp` e `Docs/CONTROL_PROFILES.md`.
Per la semantica Model 2, confronto con `src/mame/sega/model2.cpp` del checkout
MAME alla revisione `2bb77170f3f4ec0693ffb76d00680d333e18244d`:
`INPUT_PORTS_START(vf2/vstriker/daytona/srallyc/von/gunblade)`, inclusione
`rchase2` per Gunblade e associazione di Fighting Vipers agli input `vf2`.
I file di riferimento sono stati consultati in sola lettura.

Queste fonti aggiungono semantica assente dai flag SM2; non costituiscono una
prova del funzionamento in SM2. Il descrittore Fighting di Supermodel espone
Escape per tutta la firma generica: non copiarlo automaticamente su VF2 o
Fighting Vipers. Per Sega Rally il valore Hand Brake di MAME su IN2 è un byte
analogico, non un bit digitale da copiare dal Model 3. Verificare utilizzo,
riposo, polarità e percorso macchina prima di esporlo; documentare e proporre
separatamente l'eventuale integrazione dei metadati upstream.

## Profili proposti

| Profilo | Criterio / particolarità | Esempi | Set |
| --- | --- | --- | ---: |
| Joystick (Standard): Fighting | Famiglie vf2/fvipers + firma digitale attesa; azioni verificate Punch/Kick/Guard | VF2, Fighting Vipers | 7 |
| Joystick (Standard): Soccer | Famiglia vstriker + firma digitale attesa; traduzione per azione | Virtua Striker | 2 |
| Joystick (Standard): varianti da specificare | Rimanenti joystick digitali + buttons3; nomi delle azioni da verificare prima di assegnare Fighting o altre varianti | Last Bronx, Dynamite Cop, Dead or Alive, Zero Gunner | 21 |
| Joystick (Standard): Baseball (Hanguk Pro Yagu 98) | Layout digitale VF2 a due giocatori; Button 1/2/3, senza assi Bat Swing | Hanguk Pro Yagu 98 | 1 |
| Joystick (Twin) | Eccezione esplicita Virtual On: due stick dello stesso giocatore | Virtual On e revisioni | 4 |
| Joystick (Analog): Sky Target | stickx/sticky; joystick centrato, comandi di azione digitali | Sky Target | 1 |
| Driving: 4-Speed + VR4 | steer/accel/brake + gearbox, famiglia daytona con quattro VR verificati | Daytona USA | 4 |
| Driving: 4-Speed + VR1 + Handbrake | steer/accel/brake + gearbox, famiglia srallyc; Handbrake analogico su IN2 | Sega Rally | 5 |
| Driving: Sequential + VR2 | steer/accel/brake + shift_buttons; due View | Indy 500, Over Rev, Sega Touring Car | 10 |
| Driving: Sequential + VR1 | steer/accel/brake + shift_buttons; un View | Super GT 24h | 1 |
| Driving: Sequential (Manx TT Superbike) | bank/throttle/brake + shift_buttons; Start condiviso con VR | Manx TT | 3 |
| Driving: Sequential (Motor Raid) | bank/throttle/brake + shift_buttons; azioni specifiche Motor Raid | Motor Raid | 2 |
| Gun — seriale lightgun | Interfaccia seriale lightgun, coordinate a 10 bit | Virtua Cop 1/2, The House of the Dead | 6 |
| Gun — posizionale, Shot | Assi gun1/gun2 sul mux analogico; pulsanti secondo il gioco | Gunblade NY, Rail Chase 2 | 3 |
| Gun — posizionale, Shot + Missile | Stessi assi, gun_missile abilitato | Behind Enemy Lines | 1 |
| Special: Baseball | Joystick digitali + bat1/bat2 analogici | Dynamite Baseball / 97 | 2 |
| Special: Water Ski | slide | Sega Water Ski | 1 |
| Special: Ski Super G | swing/inclining | Sega Ski Super G | 1 |
| Special: Top Skater | curving/slide e comandi digitali particolari | Top Skater e revisioni | 4 |
| Special: Wave Runner | handle/roll/throttle/pitch; non trattarlo come volante a tre assi | Wave Runner | 1 |
| Joystick (Standard): Basketball (Air Walkers) | common + joystick1 + buttons3; P1-P4 implementati tramite la matrice I/O del cabinet | Air Walkers | 1 |
| Joystick (Standard): Horse Racing (Royal Ascot II) | common + joystick1 + buttons3; migliore mappatura consentita dai metadata correnti | Royal Ascot II | 1 |
| Joystick (Analog): Desert Tank + VR3 | Scheda Model 1 I/O già emulata; steer/accel/elevation e cablaggio digitale verificati da MAME | Desert Tank | 1 |

Totale: **83 set in 23 raggruppamenti di lavoro**.
La divisione tra famiglie e varianti evita di creare un profilo completo per
ogni revisione ROM. I gruppi Special restano separati perché gli assi hanno
significati fisici diversi.

I gruppi Gun distinguono qui le interfacce e le azioni per la pianificazione:
la famiglia esposta resta `Gun`, con le varianti di sorgente di Supermodel.
I 23 raggruppamenti non sono quindi 23 nuove famiglie indipendenti nel frontend.

Dentro Joystick (Standard), Fighting e Soccer sono ora espliciti per i set
verificati; completare le altre varianti per azione, combattimento, sparatutto
e baseball digitale senza dedurle dal solo genere del gioco.
I soli flag `buttons3` non permettono di ricavare nomi come Punch, Kick o Pass:
le etichette effettive richiedono verifica del titolo. `hpyagu98` usa il profilo
digitale approvato, distinto dai due Dynamite Baseball con assi bat1/bat2.

## Ambiguità da risolvere prima delle mappature

1. **Virtual On:** i metadati dichiarano `joystick1 + joystick2 + buttons3`,
   come molti giochi per due giocatori. Lo standalone riconosce invece `von`
   e il suo parent in `src/osd/input.cpp` e unisce IN1/IN2 in un cabinet
   twin-stick. Serve un'eccezione esplicita documentata finché manca un
   metadato capace di distinguerlo.
2. **Semantica dei pulsanti:** i flag e `wheel_button_bits` descrivono gruppi
   e cablaggio, non il numero e il nome di tutti i comandi realmente usati.
   Non dedurre quattro pulsanti VR attivi dal solo array di quattro elementi.
   Manx TT e Motor Raid condividono gli assi ma non necessariamente le azioni.
3. **Parent/clone:** il risultato risolto di `hotdo` e `hotdp` ha il grilletto
   P2 su IN1, mentre `hotd` indica IN2. `rchase2a` espone limiti 0–255 e nessuna
   inversione, mentre `rchase2` specifica intervalli calibrati e reverse.
   Sono differenze osservate nel loader, da confrontare con l'hardware/sorgente
   di riferimento: non uniformarle e non correggerle automaticamente.
4. **Calibrazione:** Rail Chase 2 mette P2 sui canali pari; le motociclette
   collocano throttle/brake/bank su 0/1/2; Sky Target usa 0 e 2. Il profilo
   traduce azioni fisiche in comandi logici, poi il cablaggio del set decide
   canale, polarità e scala.
5. **Continuità con Supermodel:** i flag SM2 non distinguono Fighting e Soccer.
   Servono associazioni semantiche minime per le famiglie verificate, con
   firma attesa e prova delle azioni. Per Sega Rally il riconoscitore esplicito
   completa la firma con il freno a mano analogico documentato su IN2.
6. **Copertura:** il riconoscimento del cabinet non dimostra che il titolo
   sia giocabile. Air Walkers espone P1-P4 attraverso le due coppie selezionate
   dalla matrice I/O documentata da MAME. Il fallback deve essere esplicito e
   conservativo.

## Regole per il successivo riconoscitore

- Per controlli equivalenti mantenere il profilo Supermodel, compresi nome,
  posizioni fisiche e convenzioni. Variare soltanto azioni effettive e
  traduzione hardware; una differenza di maschera non crea una nuova famiglia.
- Usare `GameSpec` già risolto. Verificare prima firme specifiche e deroghe
  documentate, poi quelle generiche; nessuna scelta del profilo per somiglianza.
- Tenere le eccezioni nell'adattatore Libretro con set/parent e firma attesa.
  Un aggiornamento che cambia quella firma richiede riesame, non una deroga
  applicata silenziosamente. Evitare una seconda copia di `games.xml`.
- Separare famiglia, variante delle azioni e parametri del set. Calibrazione,
  `start1_bit`, `wheel_button_bits`, destinazione del trigger P2 e presenza di
  comandi speciali restano dati espliciti.
- Per ogni profilo registrare nome del dispositivo e descrittori coerenti con
  le azioni implementate. Test/Service sono varianti comuni del dispositivo,
  non nuove famiglie di gioco.
- La scelta Mouse/Lightgun/Analog Stick è una modalità di sorgente: non cambia
  automaticamente un cabinet posizionale in una lightgun con ricarica fuori
  schermo, né un joystick centrato in un cursore relativo.

## Criterio di chiusura del punto 3.1

Catalogo revisionato con copertura di tutti gli 83 set e matrice di continuità
con Supermodel; equivalenze e differenze documentate senza inventare controlli. Le mappature RetroPad, i valori delle opzioni e la
persistenza appartengono ai sottopunti successivi. Le lacune dei metadati
possono mantenere esplicitamente un set nel gruppo da verificare.

## Inventario dei set per gruppo

- **Joystick (Standard): Fighting (7):** `vf2`, `vf2b`, `vf2a`, `vf2o`, `fvipers`, `fvipersa`, `fvipersb`.
- **Joystick (Standard): Soccer (2):** `vstriker`, `vstrikero`.
- **Joystick (Standard): varianti da specificare (21):** `zeroguna`, `zerogunaj`, `doaa`, `doaab`, `doa`, `doaae`, `doab`, `dynamcop`, `dynamcopb`, `dynamcopc`, `dyndeka2`, `dyndeka2b`, `lastbrnx`, `lastbrnxj`, `lastbrnxu`, `pltkids`, `pltkidsa`, `schamp`, `sfight`, `zerogun`, `zerogunj`.
- **Joystick (Standard): Baseball (Hanguk Pro Yagu 98) (1):** `hpyagu98`.
- **Joystick (Analog): Sky Target (1):** `skytargt`.
- **Gun — seriale lightgun (6):** `vcop2`, `hotd`, `hotdo`, `hotdp`, `vcop`, `vcopa`.
- **Joystick (Standard): Basketball (Air Walkers) (1):** `airwlkrs`.
- **Joystick (Standard): Horse Racing (Royal Ascot II) (1):** `rascot2`.
- **Joystick (Analog): Desert Tank + VR3 (1):** `desert`.
- **Gun — posizionale, Shot + Missile (1):** `bel`.
- **Driving: 4-Speed + VR4 (4):** `daytona`, `daytona93`, `daytonas`, `daytonase`.
- **Driving: 4-Speed + VR1 (5):** `srallyc`, `srallycb`, `srallycc`, `srallycdx`, `srallycdxa`.
- **Special: Baseball (2):** `dynabb`, `dynabb97`.
- **Gun — posizionale, Shot (3):** `gunblade`, `rchase2`, `rchase2a`.
- **Driving: Sequential + VR2 (10):** `indy500`, `indy500d`, `indy500to`, `overrev`, `overrevb`, `overrevba`, `stcc`, `stcca`, `stccb`, `stcco`.
- **Driving: Sequential + VR1 (1):** `sgt24h`.
- **Driving: Sequential (Manx TT Superbike) (3):** `manxtt`, `manxttc`, `manxttdx`.
- **Driving: Sequential (Motor Raid) (2):** `motoraid`, `motoraiddx`.
- **Special: Water Ski (1):** `segawski`.
- **Special: Ski Super G (1):** `skisuprg`.
- **Special: Top Skater (4):** `topskatr`, `topskatrj`, `topskatru`, `topskatruo`.
- **Joystick (Twin) (4):** `von`, `vonj`, `vonr`, `vonu`.
- **Special: Wave Runner (1):** `waverunr`.
