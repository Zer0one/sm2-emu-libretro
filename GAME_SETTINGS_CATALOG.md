# Catalogo autorevole dei Game Settings SEGA Model 2

Questo documento trascrive i Service Menu acquisiti nelle campagne screenshot.
Gli screenshot sono la fonte autorevole per presenza, ordine e default visibile delle righe;
i nomi delle acquisizioni cicliche forniscono i valori osservati. `games.xml` viene usato
soltanto per nome del set, titolo e relazione parent/clone. Il generatore non legge i file
YAML diagnostici né l'implementazione Libretro.

## Regole di lettura

- **Valori osservati** contiene solo valori per i quali esiste uno screenshot della campagna ciclica.
- **Solo visibile** indica una riga leggibile nella schermata base, ma priva di ciclo dedicato.
- Per un clone senza campagna ciclica, i valori sono marcati come provenienti dal parent e non vengono presentati come verificati sul clone.
- Un dato non acquisito non viene dedotto copiandolo dal parent.

## Copertura

| Elemento | Copertura |
| --- | ---: |
| Set totali in `games.xml` | 84 |
| Parent catalogati | 36 / 36 |
| Cloni catalogati | 48 / 48 |
| Righe parent trascritte | 279 |
| Righe parent con ciclo o campagna dedicata | 274 |
| Righe parent solo visibili nella schermata base | 5 |
| Screenshot primari della campagna parent indicizzati | 1259 |
| PNG supplementari nell'archivio parent | 2 |
| Campagne parent con schermata base ordinaria | 35 |
| Acquisizioni dichiarate incomplete | 0 |

## Matrice delle differenze parent/clone

Questa matrice è calcolata dalle trascrizioni del catalogo. Non richiede un nuovo confronto visivo.

| Clone | Parent | Righe aggiunte | Righe rimosse | Default/valori visibili diversi | Stato |
| --- | --- | --- | --- | --- | --- |
| `daytona93` | `daytona` | — | link-id, car-number, game-mode, rival-arrow | CABINET: TWIN → DELUXE | acquisito |
| `daytonam` | `daytona` | — | — | — | acquisito |
| `daytonas` | `daytona` | promote-saturn | — | CABINET: TWIN → UPRIGHT; COUNTRY: JPN → USA | acquisito |
| `daytonase` | `daytona` | — | — | CABINET: TWIN → SPECIAL | acquisito |
| `doaa` | `doa` | — | — | — | acquisito |
| `doaab` | `doa` | — | — | — | acquisito |
| `doaae` | `doa` | — | — | NATION: JAPAN → EXPORT | acquisito |
| `doab` | `doa` | — | — | — | acquisito |
| `dynamcopb` | `dynamcop` | — | — | — | acquisito |
| `dynamcopc` | `dynamcop` | — | — | — | acquisito |
| `dyndeka2` | `dynamcop` | HP PASSWORD | — | — | acquisito |
| `dyndeka2b` | `dynamcop` | HP PASSWORD | — | — | acquisito |
| `fvipersa` | `fvipers` | — | — | — | acquisito |
| `fvipersb` | `fvipers` | — | — | — | acquisito |
| `hotdo` | `hotd` | — | — | — | acquisito |
| `hotdp` | `hotd` | gun-blowback, cabinet-type | — | BLOOD COLOR: GREEN → RED | acquisito |
| `indy500d` | `indy500` | — | engine-volume, default-view | COUNTRY: USA → JAPAN; CABINET TYPE: TWIN → DELUXE | acquisito |
| `indy500to` | `indy500` | — | — | COUNTRY: USA → JAPAN | acquisito |
| `lastbrnxj` | `lastbrnx` | — | — | — | acquisito |
| `lastbrnxu` | `lastbrnx` | — | — | — | acquisito |
| `manxttc` | `manxtt` | — | — | CABINET TYPE: DELUXE → TWIN; START SWITCH OP.: ON → OFF | acquisito |
| `manxttdx` | `manxtt` | — | cabinet-type, link-type, laxey-revise-mode, tt-revise-mode | START SWITCH OP.: ON → OFF | acquisito |
| `motoraiddx` | `motoraid` | cabinet-type | — | ENGINE VOLUME: STANDARD → OUT OF USE | acquisito |
| `overrevb` | `overrev` | — | — | COUNTRY: JAPAN → U.S.A. | acquisito |
| `overrevba` | `overrev` | — | — | COUNTRY: JAPAN → U.S.A. | acquisito |
| `pltkidsa` | `pltkids` | — | — | — | acquisito |
| `rchase2a` | `rchase2` | — | — | — | acquisito |
| `sfight` | `schamp` | — | — | AUTOMATIC: ON → OFF; COUNTRY: USA → JAPAN | acquisito |
| `srallycb` | `srallyc` | — | — | — | acquisito |
| `srallycc` | `srallyc` | — | — | — | acquisito |
| `srallycdx` | `srallyc` | — | cabinet-type, link-type | — | acquisito |
| `srallycdxa` | `srallyc` | — | cabinet-type, link-type | — | acquisito |
| `stcca` | `stcc` | — | default-view | COUNTRY: JAPAN → USA (selected capture; native default not established) | acquisito |
| `stccb` | `stcc` | — | — | DEFAULT VIEW: BIRD'S → DRIVER'S | acquisito |
| `stcco` | `stcc` | — | default-view | COUNTRY: JAPAN → USA (selected capture; native default not established); NAME ENTRY: BEFORE-3 → AFTER-3 | acquisito |
| `topskatrj` | `topskatr` | — | — | — | acquisito |
| `topskatru` | `topskatr` | — | — | — | acquisito |
| `topskatruo` | `topskatr` | — | — | — | acquisito |
| `vcopa` | `vcop` | — | — | — | acquisito |
| `vf2a` | `vf2` | — | — | — | acquisito |
| `vf2b` | `vf2` | — | — | — | acquisito |
| `vf2o` | `vf2` | — | — | — | acquisito |
| `vonj` | `von` | — | — | — | acquisito |
| `vonr` | `von` | — | — | — | acquisito |
| `vonu` | `von` | — | — | — | acquisito |
| `vstrikero` | `vstriker` | — | one-match-mode | — | acquisito |
| `zerogunaj` | `zeroguna` | — | — | — | acquisito |
| `zerogunj` | `zerogun` | — | — | — | acquisito |

## Parent

### Air Walkers (`airwlkrs`)

- **Menu:** Game Options
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/airwlkrs/generated/screenshots/airwlkrs-base-base.png>)
- **Screenshot della campagna:** 28

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | START BUTTON SELECT | START BUTTON | nessun ciclo dedicato | base |
| 2 | PLAYER SELECTION | 2P SIMULTANEOUS | nessun ciclo dedicato | base |
| 3 | ATTRACT SOUND | OFF | nessun ciclo dedicato | base |
| 4 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 5 | GAME TIME | 2:00 | nessun ciclo dedicato | base |
| 6 | REPLAY | ON | nessun ciclo dedicato | base |
| 7 | WIN FOR FREE PLAY | CPU PLAY: YES / VS PLAY: YES | nessun ciclo dedicato | base |
| 8 | CHARACTER TYPE | NORMAL | nessun ciclo dedicato | base |

### Behind Enemy Lines (`bel`)

- **Menu:** Game System
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/bel/generated/screenshots/bel-base-base.png>)
- **Screenshot della campagna:** 53

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | COIN/CREDIT SETTING | #1 | #1, #2, #3, #4, #5, #6, #7, #8, #9, #10, #11, #12, #13, #14, #15, #16, #17, #18, #19, #20, #21, #22, #23, #24, #25, #26, #27 | base + ciclo |
| 2 | START CREDITS | 1 | 1, 2, 3, 4, 5 | base + ciclo |
| 3 | CONTINUE CREDITS | 1 | 1, 2, 3, 4, 5 | base + ciclo |
| 4 | COUNTRY | U.S. | EXPORT, JAPAN, U.S. | base + ciclo |
| 5 | ADVERTISE SOUND | ON | OFF, ON | base + ciclo |
| 6 | DIFFICULTY | 6 | 1, 10, 2, 3, 4, 5, 6, 7, 8, 9 | base + ciclo |

### Daytona USA (`daytona`)

- **Menu:** Game System
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/daytona/generated/screenshots/daytona-base-base.png>)
- **Screenshot della campagna:** 29

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | LINK ID | MASTER | MASTER, SINGLE, SLAVE | base + ciclo |
| 2 | CAR NUMBER | 1 | 1, 2, 3, 4, 5, 6, 7, 8 | base + ciclo |
| 3 | CABINET | TWIN | DELUXE, TWIN, UPRIGHT | base + ciclo |
| 4 | COUNTRY | JPN | EXPORT, JPN, USA | base + ciclo |
| 5 | DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | base + ciclo |
| 6 | ADVERTISE SOUND | ON | OFF, ON | base + ciclo |
| 7 | GAME MODE | NORMAL | ENDURANCE, GRAND PRIX, NORMAL | base + ciclo |
| 8 | RIVAL ARROW | ON | OFF, ON | base + ciclo |

### Desert Tank (`desert`)

- **Menu:** Game System
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/desert/generated/screenshots/desert-base-base.png>)
- **Screenshot della campagna:** 34

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | COIN/CREDIT SETTING | #1 | nessun ciclo dedicato | base |
| 2 | COUNTRY | JAPAN | EXPORT, JAPAN, USA | base + ciclo |
| 3 | ADVERTISE SOUND | ON | OFF, ON | base + ciclo |
| 4 | DIFFICULTY: DESERT | EASY | EASY, HARD, HARDEST, MONKEY, NORMAL | base + ciclo |
| 5 | DIFFICULTY: CANYON | EASY | EASY, HARD, HARDEST, MONKEY, NORMAL | base + ciclo |

### Dead or Alive (`doa`)

- **Menu:** Game Mode Settings
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/doa/generated/screenshots/doa-base-base.png>)
- **Screenshot della campagna:** 41

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | VJCOM SET COUNT | 2 | 2, 3, 4, 5 | base + ciclo |
| 2 | VJMAN SET COUNT | 2 | 2, 3, 4, 5 | base + ciclo |
| 3 | VJCOM DIFFICULTY | NORMAL | EASY, HARD, NORMAL, VERY HARD | base + ciclo |
| 4 | VJCOM ENERGY | NORMAL | EASY, HARD, NORMAL, VERY HARD | base + ciclo |
| 5 | VJMAN ENERGY | NORMAL | EASY, HARD, NORMAL, VERY HARD | base + ciclo |
| 6 | DEMO SOUND | ON | OFF, ON | base + ciclo |
| 7 | NATION | JAPAN | EXPORT, JAPAN, USA | base + ciclo |
| 8 | CONTINUE | ON | OFF, ON | base + ciclo |
| 9 | V.S FINISH | OFF | 1, 10, 2, 3, 4, 5, 6, 7, 8, 9, OFF | base + ciclo |
| 10 | BURST MODE | OFF | OFF, ON | base + ciclo |

### Dynamite Baseball (`dynabb`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/dynabb/generated/screenshots/dynabb-base-base.png>)
- **Screenshot della campagna:** 28

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 2 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 3 | CABINET TYPE | US | nessun ciclo dedicato | base |
| 4 | FAVORITE | OFF | nessun ciclo dedicato | base |
| 5 | INNINGS | 1 CREDIT / 2 INNINGS | nessun ciclo dedicato | base |

### Dynamite Baseball 97 (`dynabb97`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/dynabb97/generated/screenshots/dynabb97-base-base.png>)
- **Screenshot della campagna:** 26

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 2 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 3 | CABINET TYPE | US | nessun ciclo dedicato | base |
| 4 | FAVORITE | OFF | nessun ciclo dedicato | base |
| 5 | INNINGS | 1 CREDIT / 2 INNINGS | nessun ciclo dedicato | base |

### Dynamite Cop (`dynamcop`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/dynamcop/generated/screenshots/dynamcop-base-base.png>)
- **Screenshot della campagna:** 46

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 2 | GAME DIFFICULTY | 3 | nessun ciclo dedicato | base |
| 3 | LIFE AMOUNT | 104 | nessun ciclo dedicato | base |
| 4 | VIOLENCE MODE | ON | nessun ciclo dedicato | base |

### Fighting Vipers (`fvipers`)

- **Menu:** Game Assignment
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/fvipers/generated/screenshots/fvipers-base-base.png>)
- **Screenshot della campagna:** 26

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | MATCH COUNT (1P) | 2 | 2, 3, 4, 5 | base + ciclo |
| 2 | MATCH COUNT (VS) | 2 | 2, 3, 4, 5 | base + ciclo |
| 3 | DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | base + ciclo |
| 4 | ADVERTISE SOUND | ON | OFF, ON | base + ciclo |
| 5 | CONTINUE | ON | OFF, ON | base + ciclo |
| 6 | COUNTRY | JAPAN | EXPORT, JAPAN, USA | base + ciclo |
| 7 | DISPLAY TYPE | PROJECTOR | CRT, PROJECTOR | base + ciclo |
| 8 | VS FINISH | OFF | OFF, ON | base + ciclo |
| 9 | RANKING MODE | OFF | OFF, ON | base + ciclo |

### Gunblade NY (`gunblade`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/gunblade/generated/screenshots/gunblade-base-base.png>)
- **Screenshot della campagna:** 33

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 2 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 3 | GAME DIFFICULTY | 4/8 | nessun ciclo dedicato | base |
| 4 | SHIFTING DIFFICULTY | 4/8 | nessun ciclo dedicato | base |
| 5 | PLAYER LIFE | 3 | nessun ciclo dedicato | base |
| 6 | GUN REACTION | ON | nessun ciclo dedicato | base |
| 7 | CABINET TYPE | DX | nessun ciclo dedicato | base |

### The House of the Dead (`hotd`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/hotd/generated/screenshots/hotd-base-base.png>)
- **Screenshot della campagna:** 27

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | MEDIUM EASY, MEDIUM HARD, NORMAL, VERY EASY, VERY HARD | base + ciclo |
| 2 | LIFE SETTING | INITIAL 3 / MAX LIFE 5 | INITIAL 1 MAX 3, INITIAL 1 MAX 4, INITIAL 1 MAX 5, INITIAL 2 MAX 3, INITIAL 2 MAX 4, INITIAL 2 MAX 5, INITIAL 3 MAX 3, INITIAL 3 MAX 4, INITIAL 3 MAX 5, INITIAL 4 MAX 4, INITIAL 4 MAX 5, INITIAL 5 MAX 5 | base + ciclo |
| 3 | BLOOD COLOR | GREEN | BLUE, GREEN, PURPLE, RED | base + ciclo |
| 4 | ADVERTISE SOUND | ON | OFF, ON | base + ciclo |
| 5 | COUNTRY | JAPAN | EXPORT, JAPAN, USA | base + ciclo |

### Hanguk Pro Yagu 98 (`hpyagu98`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/hpyagu98/generated/screenshots/hpyagu98-base-base.png>)
- **Screenshot della campagna:** 22

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 2 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 3 | CABINET TYPE | US | nessun ciclo dedicato | base |
| 4 | FAVORITE | OFF | nessun ciclo dedicato | base |
| 5 | INNINGS | 1 CREDIT / 2 INNINGS | nessun ciclo dedicato | base |

### INDY 500 Twin (`indy500`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/indy500/generated/screenshots/indy500-base-base.png>)
- **Screenshot della campagna:** 35

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 2 | RACE MODE | NORMAL | nessun ciclo dedicato | base |
| 3 | HANDICAP | ON | nessun ciclo dedicato | base |
| 4 | ADVERTISE SOUND | OFF | nessun ciclo dedicato | base |
| 5 | COUNTRY | USA | nessun ciclo dedicato | base |
| 6 | CABINET TYPE | TWIN | nessun ciclo dedicato | base |
| 7 | NETWORK TYPE | STAND ALONE | nessun ciclo dedicato | base |
| 8 | CABINET ID | 1 | nessun ciclo dedicato | base |
| 9 | ENGINE VOLUME | 3 | nessun ciclo dedicato | base |
| 10 | DEFAULT VIEW | 4 | nessun ciclo dedicato | base |

### Last Bronx (`lastbrnx`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/lastbrnx/generated/screenshots/lastbrnx-base-base.png>)
- **Screenshot della campagna:** 30

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | 1 NORMAL | nessun ciclo dedicato | base |
| 2 | ADVERTISE SOUND | OFF | nessun ciclo dedicato | base |
| 3 | VS FINISH | OFF | nessun ciclo dedicato | base |
| 4 | SURVIVAL MODE | ON | nessun ciclo dedicato | base |
| 5 | MATCH POINT (CPU) | 2 | nessun ciclo dedicato | base |
| 6 | MATCH POINT (VS) | 2 | nessun ciclo dedicato | base |
| 7 | CUT CROSS STREET | OFF | nessun ciclo dedicato | base |
| 8 | DISPLAY TYPE | C.R.T. | nessun ciclo dedicato | base |
| 9 | MASTER VOLUME | 3 | nessun ciclo dedicato | base |

### Manx TT Superbike - DX/Twin (`manxtt`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/manxtt/generated/screenshots/manxtt-base-base.png>)
- **Screenshot della campagna:** 60

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 2 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 3 | CABINET TYPE | DELUXE | nessun ciclo dedicato | base |
| 4 | LINK TYPE | Not Link | nessun ciclo dedicato | base |
| 5 | BIKE COLOR (No.) | RED (No.1) | nessun ciclo dedicato | base |
| 6 | RACE MODE | RACE | nessun ciclo dedicato | base |
| 7 | LAXEY NUMBER OF LAP | 2 | nessun ciclo dedicato | base |
| 8 | LAXEY GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 9 | LAXEY REVISE MODE | 2/3 | nessun ciclo dedicato | base |
| 10 | TT NUMBER OF LAP | 2 | nessun ciclo dedicato | base |
| 11 | TT GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 12 | TT REVISE MODE | 2/3 | nessun ciclo dedicato | base |
| 13 | START SWITCH OP. | ON | nessun ciclo dedicato | base |

### Motor Raid - Twin (`motoraid`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/motoraid/generated/screenshots/motoraid-base-base.png>)
- **Screenshot della campagna:** 25

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 2 | RACE MODE | STANDARD | nessun ciclo dedicato | base |
| 3 | ENEMY LEVEL | NORMAL | nessun ciclo dedicato | base |
| 4 | ENGINE VOLUME | STANDARD | nessun ciclo dedicato | base |
| 5 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 6 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 7 | NETWORK TYPE | STAND ALONE | nessun ciclo dedicato | base |
| 8 | CABINET ID | 1 | nessun ciclo dedicato | base |

### Over Rev (`overrev`)

- **Menu:** Foundational / Optional Game Setting
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/overrev/generated/screenshots/overrev-base-base.png>)
- **Screenshot della campagna:** 36

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 2 | HARDWARE TYPE | NORMAL (2in1) | nessun ciclo dedicato | base |
| 3 | LINK MAX | NOT LINK | nessun ciclo dedicato | base |
| 4 | LINK TYPE | MASTER CarNo.1 | nessun ciclo dedicato | base |
| 5 | TIME DIFFICULTY | 3 | nessun ciclo dedicato | base |
| 6 | STEER KICK BACK | 3 | nessun ciclo dedicato | base |
| 7 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 8 | DEMO SOUND | ON | nessun ciclo dedicato | base |
| 9 | MAXIMUM LAP | NORMAL MODE | nessun ciclo dedicato | base |

### Pilot Kids (`pltkids`)

- **Menu:** Game Configuring
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/pltkids/generated/screenshots/pltkids-base-base.png>)
- **Screenshot della campagna:** 37

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | Fighters | 3 | nessun ciclo dedicato | base |
| 2 | Difficult | Normal | nessun ciclo dedicato | base |
| 3 | Demo Sound | Off | nessun ciclo dedicato | base |
| 4 | Coin Slot | Same | nessun ciclo dedicato | base |
| 5 | Coin Mode | Normal | nessun ciclo dedicato | base |
| 6 | Coin-1 | 1 Coin = 1 Credit | nessun ciclo dedicato | base |
| 7 | Coin-2 | 1 Coin = 1 Credit | nessun ciclo dedicato | base |
| 8 | Continue | On | nessun ciclo dedicato | base |

### Royal Ascot II (`rascot2`)

**Stato:** caso speciale acquisito. La diagnostica mostra l'attesa SEGANET e indica `Push Shot1`; Button 1 avvia correttamente il titolo senza SegaNetCom. Il successivo comando Test torna alla diagnostica SEGANET e non espone un normale menu Game Settings. Non risultano quindi righe Game Settings ordinarie da catalogare nell'emulazione corrente.

**Evidenza:** [diagnostica SEGANET](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/game-settings-gap-fill-20260920/rascot2/screenshots/rascot2-service-short-service-short.png>), [avvio locale dopo Button 1](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/game-settings-gap-fill-20260920/rascot2/screenshots/rascot2-local-start-test-menu-after-shot1.png>), [ritorno della diagnostica con Test](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/game-settings-gap-fill-20260920/rascot2/screenshots/rascot2-local-start-test-menu-local-start-test-menu.png>)

### Rail Chase 2 (`rchase2`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/rchase2/generated/screenshots/rchase2-base-base.png>)
- **Screenshot della campagna:** 22

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 2 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 3 | GAME DIFFICULTY | 4/8 | nessun ciclo dedicato | base |
| 4 | SHIFTING DIFFICULTY | 50 sec EVERY | nessun ciclo dedicato | base |

### Sonic Championship (`schamp`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/schamp/generated/screenshots/schamp-base-base.png>)
- **Screenshot della campagna:** 60

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | MATCH COUNT (1P) | 2 | nessun ciclo dedicato | base |
| 2 | MATCH COUNT (VS) | 2 | nessun ciclo dedicato | base |
| 3 | ENEMY RANK | NORMAL | nessun ciclo dedicato | base |
| 4 | TIME | 30 | nessun ciclo dedicato | base |
| 5 | ENERGY (1P) | NORMAL | nessun ciclo dedicato | base |
| 6 | ENERGY (VS) | NORMAL | nessun ciclo dedicato | base |
| 7 | BARRIER | 5 | nessun ciclo dedicato | base |
| 8 | BARRIER RESET | OFF | nessun ciclo dedicato | base |
| 9 | AUTOMATIC | ON | nessun ciclo dedicato | base |
| 10 | HYPER MODE | ON | nessun ciclo dedicato | base |
| 11 | DAMAGE | NORMAL | nessun ciclo dedicato | base |
| 12 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 13 | CONTINUE | ON | nessun ciclo dedicato | base |
| 14 | COUNTRY | USA | nessun ciclo dedicato | base |
| 15 | DISPLAY TYPE | C.R.T. | nessun ciclo dedicato | base |
| 16 | VS FINISH | OFF | nessun ciclo dedicato | base |

### Sega Water Ski (`segawski`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/segawski/generated/screenshots/segawski-base-base.png>)
- **Screenshot della campagna:** 8

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 2 | ADVERTISE SOUND | OFF | nessun ciclo dedicato | base |

### Super GT 24h (`sgt24h`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/sgt24h/generated/screenshots/sgt24h-base-base.png>)
- **Screenshot della campagna:** 33

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | LINK TYPE | NOT LINK | nessun ciclo dedicato | base |
| 2 | LINK MAX | NOT LINK | nessun ciclo dedicato | base |
| 3 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 4 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 5 | CPU CAR LEVEL | 2 | nessun ciclo dedicato | base |
| 6 | STEERING FORCE | 2 | nessun ciclo dedicato | base |
| 7 | RACE MODE | NORMAL | nessun ciclo dedicato | base |
| 8 | GAME BGM | ON | nessun ciclo dedicato | base |
| 9 | DEMO SOUND | ON | nessun ciclo dedicato | base |
| 10 | I/O TYPE | A | nessun ciclo dedicato | base |

### Sega Ski Super G (`skisuprg`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/skisuprg/generated/screenshots/skisuprg-base-base.png>)
- **Screenshot della campagna:** 105

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 2 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 3 | NETWORK TYPE | NO LINK | nessun ciclo dedicato | base |
| 4 | CABINET ID | 1 | nessun ciclo dedicato | base |
| 5 | DRIVE BOARD POWER | 2 | nessun ciclo dedicato | base |
| 6 | LIVE DISPLAY | ON | nessun ciclo dedicato | base |
| 7 | VICTORIA DISPLAY | ON | nessun ciclo dedicato | base |
| 8 | INITIAL TIME | 10 | nessun ciclo dedicato | base |
| 9 | STAGE TIME (WHITE FOREST) | 10 | nessun ciclo dedicato | base |
| 10 | STAGE TIME (NIGHT VALLEY) | 10 | nessun ciclo dedicato | base |
| 11 | STAGE TIME (WILD KING) | 10 | nessun ciclo dedicato | base |

### Sky Target (`skytargt`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/skytargt/generated/screenshots/skytargt-base-base.png>)
- **Screenshot della campagna:** 12

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 2 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 3 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 4 | CABINET TYPE | STANDARD | nessun ciclo dedicato | base |

### Sega Rally Championship - Twin/DX (`srallyc`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/srallyc/generated/screenshots/srallyc-base-base.png>)
- **Screenshot della campagna:** 22

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | OFF, ON | base + ciclo |
| 2 | COUNTRY | JPN | EXPORT, JPN, USA | base + ciclo |
| 3 | CABINET TYPE | TWIN | DELUXE, TWIN | base + ciclo |
| 4 | LINK TYPE | NOTLINK | CAR 1, CAR 2, CAR 3, CAR 4, NOTLINK, RELAY | base + ciclo |
| 5 | GAME DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | base + ciclo |
| 6 | GAME MODE | NORMAL | LONG, LONGEST, NORMAL, SHORT | base + ciclo |

### Sega Touring Car Championship (`stcc`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/stcc/generated/screenshots/stcc-base-base.png>)
- **Screenshot della campagna:** 39

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 2 | URL ADDRESS | ON | nessun ciclo dedicato | base |
| 3 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 4 | CABINET TYPE | TWIN | nessun ciclo dedicato | base |
| 5 | LINK TYPE | STAND ALONE | nessun ciclo dedicato | base |
| 6 | DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 7 | GAME MODE | NORMAL | nessun ciclo dedicato | base |
| 8 | DEFAULT CAR | RANDOM | nessun ciclo dedicato | base |
| 9 | NAME ENTRY | BEFORE-3 | nessun ciclo dedicato | base |
| 10 | DEFAULT VIEW | BIRD'S | nessun ciclo dedicato | base |

### Top Skater (`topskatr`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/topskatr/generated/screenshots/topskatr-base-base.png>)
- **Screenshot della campagna:** 11

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 2 | GAME DIFFICULTY | 4/8 | nessun ciclo dedicato | base |

### Virtua Cop (`vcop`)

- **Menu:** Game System
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/vcop/generated/screenshots/vcop-base-base.png>)
- **Screenshot della campagna:** 27

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | OFF, ON | base + ciclo |
| 2 | COUNTRY | JPN | EXP, JPN, USA | base + ciclo |
| 3 | CABINET | DX | DX, SP U/R, U/R | base + ciclo |
| 4 | DIFFICULTY | NORMAL | EASIEST, EASY, HARD, HARDEST, MEDIUM EASY, MEDIUM HARD, NORMAL, VERY EASY, VERY HARD | base + ciclo |
| 5 | LIFE | 6 | 1, 2, 3, 4, 5, 6, 7, 8, 9 | base + ciclo |
| 6 | HUMAN TYPE | NORMAL | — | visible-only |
| 7 | RELOAD TYPE | NORMAL | — | visible-only |

### Virtua Cop 2 (`vcop2`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/vcop2/generated/screenshots/vcop2-base-base.png>)
- **Screenshot della campagna:** 27

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 2 | COUNTRY | JPN | nessun ciclo dedicato | base |
| 3 | CABINET | DX | nessun ciclo dedicato | base |
| 4 | DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 5 | LIFE | 4 | nessun ciclo dedicato | base |

### Virtua Fighter 2 (`vf2`)

- **Menu:** Game Assignment
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/vf2/generated/screenshots/vf2-base-base.png>)
- **Screenshot della campagna:** 30

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | MATCH COUNT (1P) | 2 | nessun ciclo dedicato | base |
| 2 | MATCH COUNT (VS) | 2 | nessun ciclo dedicato | base |
| 3 | DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 4 | ENERGY MAX (1P) | 168 | — | visible-only |
| 5 | ENERGY MAX (VS) | 288 | — | visible-only |
| 6 | STAGE WIDTH | 1580 | — | visible-only |
| 7 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 8 | CONTINUE | ON | nessun ciclo dedicato | base |
| 9 | DRINK | OK | nessun ciclo dedicato | base |
| 10 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 11 | DISPLAY TYPE | PROJECTOR | nessun ciclo dedicato | base |
| 12 | VS FINISH | OFF | nessun ciclo dedicato | base |
| 13 | RANKING MODE | OFF | nessun ciclo dedicato | base |
| 14 | VERSION | NORMAL | nessun ciclo dedicato | base |

### Cyber Troopers Virtual-On - Twin (`von`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/von/generated/screenshots/von-base-base.png>)
- **Screenshot della campagna:** 117

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | PLAY TIME 1P (STAGE 1~5) | 90 SECS | nessun ciclo dedicato | base |
| 2 | PLAY TIME 1P (PENALTY) | 90 SECS | nessun ciclo dedicato | base |
| 3 | PLAY TIME 1P (STAGE 6~8) | 90 SECS | nessun ciclo dedicato | base |
| 4 | PLAY TIME 1P (LAST STG) | 90 SECS | nessun ciclo dedicato | base |
| 5 | PLAY TIME VERSUS | 90 SECS | nessun ciclo dedicato | base |
| 6 | MATCH COUNT 1P (STAGE 1~5) | 1 | nessun ciclo dedicato | base |
| 7 | MATCH COUNT 1P (PENALTY) | 1 | nessun ciclo dedicato | base |
| 8 | MATCH COUNT 1P (STAGE 6~8) | 1 | nessun ciclo dedicato | base |
| 9 | MATCH COUNT VERSUS | 1 | nessun ciclo dedicato | base |
| 10 | NETWORK LINK ATTRIBUTE | NO LINK | nessun ciclo dedicato | base |
| 11 | WINNING BY DECISION | ON | nessun ciclo dedicato | base |
| 12 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 13 | ADVERTISE SOUND | LOUD | nessun ciclo dedicato | base |
| 14 | CONTINUE | ON | nessun ciclo dedicato | base |
| 15 | REPLAY AND POSING MODE | REPLAY & POSING | nessun ciclo dedicato | base |
| 16 | RANKING MODE | ON | nessun ciclo dedicato | base |
| 17 | VERSUS ALWAYS FINISH | OFF | nessun ciclo dedicato | base |
| 18 | DISPLAY BRIGHTNESS | 0 | nessun ciclo dedicato | base |

### Virtua Striker (`vstriker`)

- **Menu:** Game System
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/vstriker/generated/screenshots/vstriker-base-base.png>)
- **Screenshot della campagna:** 37

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | nessun ciclo dedicato | base |
| 2 | COUNTRY | JPN | nessun ciclo dedicato | base |
| 3 | MONITOR | CRT | nessun ciclo dedicato | base |
| 4 | DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 5 | TIME SET | 2:00 | nessun ciclo dedicato | base |
| 6 | V GOAL SYSTEM | OFF | nessun ciclo dedicato | base |
| 7 | V GOAL TIME SET | 0:15 | nessun ciclo dedicato | base |
| 8 | PK SYSTEM | OFF | nessun ciclo dedicato | base |
| 9 | PK MEMBER SET | 3 | nessun ciclo dedicato | base |
| 10 | BILLBOARD | ON | nessun ciclo dedicato | base |
| 11 | ONE MATCH MODE | OFF | nessun ciclo dedicato | base |

### Wave Runner (`waverunr`)

- **Menu:** Game Assignments
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/waverunr/generated/screenshots/waverunr-base-base.png>)
- **Screenshot della campagna:** 22

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | nessun ciclo dedicato | base |
| 2 | RACE MODE | NORMAL | nessun ciclo dedicato | base |
| 3 | HANDICAP | ON | nessun ciclo dedicato | base |
| 4 | ADVERTISE SOUND | OFF | nessun ciclo dedicato | base |
| 5 | COUNTRY | JAPAN | nessun ciclo dedicato | base |
| 6 | NETWORK TYPE | STAND ALONE | nessun ciclo dedicato | base |
| 7 | CABINET ID | 1 | nessun ciclo dedicato | base |

### Zero Gunner (`zerogun`)

- **Menu:** Configuring
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/zerogun/generated/screenshots/zerogun-base-base.png>)
- **Screenshot della campagna:** 35

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | Credit Mode | Same | nessun ciclo dedicato | base |
| 2 | Continue Mode | Normal Mode | nessun ciclo dedicato | base |
| 3 | Coin Slot 1 | 1 coin / 1 credit | nessun ciclo dedicato | base |
| 4 | Coin Slot 2 | 1 coin / 1 credit | nessun ciclo dedicato | base |
| 5 | Demo Sound | ON | nessun ciclo dedicato | base |
| 6 | Difficulty | NORMAL | nessun ciclo dedicato | base |
| 7 | Fighters | 3 fighters | nessun ciclo dedicato | base |
| 8 | Extend Points | 600000 | nessun ciclo dedicato | base |
| 9 | Ranking Data | Do Initialize | nessun ciclo dedicato | base |

### Zero Gunner (`zeroguna`)

- **Menu:** Configuring
- **Schermata base:** [apri screenshot](</Users/andrea/Documents/RetroArch/sm2-nvram-analysis/zeroguna/generated/screenshots/zeroguna-base-base.png>)
- **Screenshot della campagna:** 35

| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |
| ---: | --- | --- | --- | --- |
| 1 | Credit Mode | Same | nessun ciclo dedicato | base |
| 2 | Continue Mode | Normal Mode | nessun ciclo dedicato | base |
| 3 | Coin Slot 1 | 1 coin / 1 credit | nessun ciclo dedicato | base |
| 4 | Coin Slot 2 | 1 coin / 1 credit | nessun ciclo dedicato | base |
| 5 | Demo Sound | ON | nessun ciclo dedicato | base |
| 6 | Difficulty | NORMAL | nessun ciclo dedicato | base |
| 7 | Fighters | 3 fighters | nessun ciclo dedicato | base |
| 8 | Extend Points | 600000 | nessun ciclo dedicato | base |
| 9 | Ranking Data | Do Initialize | nessun ciclo dedicato | base |

## Cloni

Ogni tabella seguente è la trascrizione strutturata dello screenshot del clone. Quando
l'insieme delle righe coincide con il parent, il catalogo lo espande comunque per rendere
immediate le differenze. I valori possibili restano attribuiti alla campagna parent finché
non esiste una campagna ciclica dedicata al clone.

### Daytona USA (`daytona93`)

- **Parent:** `daytona`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/daytona93/screenshots/daytona93-native-menu-game-system.png>)

- **Differenze strutturali:** rimosse: link-id, car-number, game-mode, rival-arrow.
- **Nota:** Reduced four-row menu.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `daytona` |
| 2 | COUNTRY | JPN | EXPORT, JPN, USA | campagna parent `daytona` |
| 3 | CABINET | DELUXE | DELUXE, UPRIGHT | ciclo clone |
| 4 | DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | campagna parent `daytona` |

### Daytona USA (To The MAXX) (`daytonam`)

- **Parent:** `daytona`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/daytonam/screenshots/daytonam-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | LINK ID | MASTER | MASTER, SINGLE, SLAVE | campagna parent `daytona` |
| 2 | CAR NUMBER | 1 | 1, 2, 3, 4, 5, 6, 7, 8 | campagna parent `daytona` |
| 3 | CABINET | TWIN | DELUXE, TWIN, UPRIGHT | campagna parent `daytona` |
| 4 | COUNTRY | JPN | EXPORT, JPN, USA | campagna parent `daytona` |
| 5 | DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | campagna parent `daytona` |
| 6 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `daytona` |
| 7 | GAME MODE | NORMAL | ENDURANCE, GRAND PRIX, NORMAL | campagna parent `daytona` |
| 8 | RIVAL ARROW | ON | OFF, ON | campagna parent `daytona` |

### Daytona USA (`daytonas`)

- **Parent:** `daytona`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/daytonas/screenshots/daytonas-native-menu-game-system.png>)

- **Differenze strutturali:** aggiunte: promote-saturn.
- **Nota:** Adds PROMOTE SATURN; dedicated cycles show OFF, COMING SOON and AVAILABLE NOW.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | LINK ID | MASTER | MASTER, SINGLE, SLAVE | campagna parent `daytona` |
| 2 | CAR NUMBER | 1 | 1, 2, 3, 4, 5, 6, 7, 8 | campagna parent `daytona` |
| 3 | CABINET | UPRIGHT | DELUXE, TWIN, SPECIAL, UPRIGHT | ciclo clone |
| 4 | COUNTRY | USA | EXPORT, JPN, USA | campagna parent `daytona` |
| 5 | DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | campagna parent `daytona` |
| 6 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `daytona` |
| 7 | GAME MODE | NORMAL | ENDURANCE, GRAND PRIX, NORMAL | campagna parent `daytona` |
| 8 | RIVAL ARROW | ON | OFF, ON | campagna parent `daytona` |
| 9 | PROMOTE SATURN | COMING SOON | OFF, COMING SOON, AVAILABLE NOW | ciclo clone |

### Daytona USA Special Edition (`daytonase`)

- **Parent:** `daytona`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/daytonase/screenshots/daytonase-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.
- **Nota:** Same row set; CABINET defaults to SPECIAL.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | LINK ID | MASTER | MASTER, SINGLE, SLAVE | campagna parent `daytona` |
| 2 | CAR NUMBER | 1 | 1, 2, 3, 4, 5, 6, 7, 8 | campagna parent `daytona` |
| 3 | CABINET | SPECIAL | DELUXE, TWIN, UPRIGHT | campagna parent `daytona` |
| 4 | COUNTRY | JPN | EXPORT, JPN, USA | campagna parent `daytona` |
| 5 | DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | campagna parent `daytona` |
| 6 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `daytona` |
| 7 | GAME MODE | NORMAL | ENDURANCE, GRAND PRIX, NORMAL | campagna parent `daytona` |
| 8 | RIVAL ARROW | ON | OFF, ON | campagna parent `daytona` |

### Dead or Alive (`doaa`)

- **Parent:** `doa`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/doaa/screenshots/doaa-menu-structure-game-mode-settings.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | VJCOM SET COUNT | 2 | 2, 3, 4, 5 | campagna parent `doa` |
| 2 | VJMAN SET COUNT | 2 | 2, 3, 4, 5 | campagna parent `doa` |
| 3 | VJCOM DIFFICULTY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 4 | VJCOM ENERGY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 5 | VJMAN ENERGY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 6 | DEMO SOUND | ON | OFF, ON | campagna parent `doa` |
| 7 | NATION | JAPAN | EXPORT, JAPAN, USA | campagna parent `doa` |
| 8 | CONTINUE | ON | OFF, ON | campagna parent `doa` |
| 9 | V.S FINISH | OFF | 1, 10, 2, 3, 4, 5, 6, 7, 8, 9, OFF | campagna parent `doa` |
| 10 | BURST MODE | OFF | OFF, ON | campagna parent `doa` |

### Dead or Alive (`doaab`)

- **Parent:** `doa`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/doaab/screenshots/doaab-menu-structure-game-mode-settings.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | VJCOM SET COUNT | 2 | 2, 3, 4, 5 | campagna parent `doa` |
| 2 | VJMAN SET COUNT | 2 | 2, 3, 4, 5 | campagna parent `doa` |
| 3 | VJCOM DIFFICULTY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 4 | VJCOM ENERGY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 5 | VJMAN ENERGY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 6 | DEMO SOUND | ON | OFF, ON | campagna parent `doa` |
| 7 | NATION | JAPAN | EXPORT, JAPAN, USA | campagna parent `doa` |
| 8 | CONTINUE | ON | OFF, ON | campagna parent `doa` |
| 9 | V.S FINISH | OFF | 1, 10, 2, 3, 4, 5, 6, 7, 8, 9, OFF | campagna parent `doa` |
| 10 | BURST MODE | OFF | OFF, ON | campagna parent `doa` |

### Dead or Alive (`doaae`)

- **Parent:** `doa`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/doaae/screenshots/doaae-menu-structure-game-mode-settings.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | VJCOM SET COUNT | 2 | 2, 3, 4, 5 | campagna parent `doa` |
| 2 | VJMAN SET COUNT | 2 | 2, 3, 4, 5 | campagna parent `doa` |
| 3 | VJCOM DIFFICULTY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 4 | VJCOM ENERGY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 5 | VJMAN ENERGY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 6 | DEMO SOUND | ON | OFF, ON | campagna parent `doa` |
| 7 | NATION | EXPORT | EXPORT, JAPAN, USA | campagna parent `doa` |
| 8 | CONTINUE | ON | OFF, ON | campagna parent `doa` |
| 9 | V.S FINISH | OFF | 1, 10, 2, 3, 4, 5, 6, 7, 8, 9, OFF | campagna parent `doa` |
| 10 | BURST MODE | OFF | OFF, ON | campagna parent `doa` |

### Dead or Alive (`doab`)

- **Parent:** `doa`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/doab/screenshots/doab-menu-structure-game-mode-settings.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | VJCOM SET COUNT | 2 | 2, 3, 4, 5 | campagna parent `doa` |
| 2 | VJMAN SET COUNT | 2 | 2, 3, 4, 5 | campagna parent `doa` |
| 3 | VJCOM DIFFICULTY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 4 | VJCOM ENERGY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 5 | VJMAN ENERGY | NORMAL | EASY, HARD, NORMAL, VERY HARD | campagna parent `doa` |
| 6 | DEMO SOUND | ON | OFF, ON | campagna parent `doa` |
| 7 | NATION | JAPAN | EXPORT, JAPAN, USA | campagna parent `doa` |
| 8 | CONTINUE | ON | OFF, ON | campagna parent `doa` |
| 9 | V.S FINISH | OFF | 1, 10, 2, 3, 4, 5, 6, 7, 8, 9, OFF | campagna parent `doa` |
| 10 | BURST MODE | OFF | OFF, ON | campagna parent `doa` |

### Dynamite Cop (`dynamcopb`)

- **Parent:** `dynamcop`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/dynamcopb/screenshots/dynamcopb-menu-structure-game-assignments.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | GAME DIFFICULTY | 3 | non acquisiti | nessun ciclo |
| 3 | LIFE AMOUNT | 104 | non acquisiti | nessun ciclo |
| 4 | VIOLENCE MODE | ON | non acquisiti | nessun ciclo |

### Dynamite Cop (`dynamcopc`)

- **Parent:** `dynamcop`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/dynamcopc/screenshots/dynamcopc-menu-structure-game-assignments.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | GAME DIFFICULTY | 3 | non acquisiti | nessun ciclo |
| 3 | LIFE AMOUNT | 104 | non acquisiti | nessun ciclo |
| 4 | VIOLENCE MODE | ON | non acquisiti | nessun ciclo |

### Dynamite Deka 2 (`dyndeka2`)

- **Parent:** `dynamcop`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/dyndeka2/screenshots/dyndeka2-menu-structure-game-assignments.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.
- **Nota:** HP PASSWORD is visible but informational.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | GAME DIFFICULTY | 3 | non acquisiti | nessun ciclo |
| 3 | LIFE AMOUNT | 104 | non acquisiti | nessun ciclo |
| 4 | VIOLENCE MODE | ON | non acquisiti | nessun ciclo |
| 5 | HP PASSWORD | NO PASSWORD | — | informational |

### Dynamite Deka 2 (`dyndeka2b`)

- **Parent:** `dynamcop`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/dyndeka2b/screenshots/dyndeka2b-menu-structure-game-assignments.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.
- **Nota:** HP PASSWORD is visible but informational.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | GAME DIFFICULTY | 3 | non acquisiti | nessun ciclo |
| 3 | LIFE AMOUNT | 104 | non acquisiti | nessun ciclo |
| 4 | VIOLENCE MODE | ON | non acquisiti | nessun ciclo |
| 5 | HP PASSWORD | NO PASSWORD | — | informational |

### Fighting Vipers (`fvipersa`)

- **Parent:** `fvipers`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/fvipersa/screenshots/fvipersa-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | MATCH COUNT (1P) | 2 | 2, 3, 4, 5 | campagna parent `fvipers` |
| 2 | MATCH COUNT (VS) | 2 | 2, 3, 4, 5 | campagna parent `fvipers` |
| 3 | DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | campagna parent `fvipers` |
| 4 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `fvipers` |
| 5 | CONTINUE | ON | OFF, ON | campagna parent `fvipers` |
| 6 | COUNTRY | JAPAN | EXPORT, JAPAN, USA | campagna parent `fvipers` |
| 7 | DISPLAY TYPE | PROJECTOR | CRT, PROJECTOR | campagna parent `fvipers` |
| 8 | VS FINISH | OFF | OFF, ON | campagna parent `fvipers` |
| 9 | RANKING MODE | OFF | OFF, ON | campagna parent `fvipers` |

### Fighting Vipers (`fvipersb`)

- **Parent:** `fvipers`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/fvipersb/screenshots/fvipersb-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | MATCH COUNT (1P) | 2 | 2, 3, 4, 5 | campagna parent `fvipers` |
| 2 | MATCH COUNT (VS) | 2 | 2, 3, 4, 5 | campagna parent `fvipers` |
| 3 | DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | campagna parent `fvipers` |
| 4 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `fvipers` |
| 5 | CONTINUE | ON | OFF, ON | campagna parent `fvipers` |
| 6 | COUNTRY | JAPAN | EXPORT, JAPAN, USA | campagna parent `fvipers` |
| 7 | DISPLAY TYPE | PROJECTOR | CRT, PROJECTOR | campagna parent `fvipers` |
| 8 | VS FINISH | OFF | OFF, ON | campagna parent `fvipers` |
| 9 | RANKING MODE | OFF | OFF, ON | campagna parent `fvipers` |

### The House of the Dead (`hotdo`)

- **Parent:** `hotd`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/hotdo/screenshots/hotdo-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | MEDIUM EASY, MEDIUM HARD, NORMAL, VERY EASY, VERY HARD | campagna parent `hotd` |
| 2 | LIFE SETTING | INITIAL 3 / MAX LIFE 5 | INITIAL 1 MAX 3, INITIAL 1 MAX 4, INITIAL 1 MAX 5, INITIAL 2 MAX 3, INITIAL 2 MAX 4, INITIAL 2 MAX 5, INITIAL 3 MAX 3, INITIAL 3 MAX 4, INITIAL 3 MAX 5, INITIAL 4 MAX 4, INITIAL 4 MAX 5, INITIAL 5 MAX 5 | campagna parent `hotd` |
| 3 | BLOOD COLOR | GREEN | BLUE, GREEN, PURPLE, RED | campagna parent `hotd` |
| 4 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `hotd` |
| 5 | COUNTRY | JAPAN | EXPORT, JAPAN, USA | campagna parent `hotd` |

### The House of the Dead (`hotdp`)

- **Parent:** `hotd`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/hotdp/screenshots/hotdp-menu-structure-settings-main.png>)

- **Differenze strutturali:** aggiunte: gun-blowback, cabinet-type.
- **Nota:** Dedicated clone campaign; adds GUN BLOWBACK and CABINET TYPE.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | MEDIUM EASY, MEDIUM HARD, NORMAL, VERY EASY, VERY HARD | campagna parent `hotd` |
| 2 | LIFE SETTING | INITIAL 3 / MAX LIFE 5 | INITIAL 1 MAX 3, INITIAL 1 MAX 4, INITIAL 1 MAX 5, INITIAL 2 MAX 3, INITIAL 2 MAX 4, INITIAL 2 MAX 5, INITIAL 3 MAX 3, INITIAL 3 MAX 4, INITIAL 3 MAX 5, INITIAL 4 MAX 4, INITIAL 4 MAX 5, INITIAL 5 MAX 5 | campagna parent `hotd` |
| 3 | BLOOD COLOR | RED | RED, GREEN | ciclo clone |
| 4 | GUN BLOWBACK | ON | non acquisiti | nessun ciclo |
| 5 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `hotd` |
| 6 | COUNTRY | JAPAN | EXPORT, JAPAN, USA | campagna parent `hotd` |
| 7 | CABINET TYPE | STANDARD | non acquisiti | nessun ciclo |

### INDY 500 Deluxe (`indy500d`)

- **Parent:** `indy500`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/indy500d/screenshots/indy500d-menu-structure-settings-main.png>)

- **Differenze strutturali:** rimosse: engine-volume, default-view.
- **Nota:** ENGINE VOLUME and DEFAULT VIEW are absent.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 2 | RACE MODE | NORMAL | non acquisiti | nessun ciclo |
| 3 | HANDICAP | ON | non acquisiti | nessun ciclo |
| 4 | ADVERTISE SOUND | OFF | non acquisiti | nessun ciclo |
| 5 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 6 | CABINET TYPE | DELUXE | non acquisiti | nessun ciclo |
| 7 | NETWORK TYPE | STAND ALONE | non acquisiti | nessun ciclo |
| 8 | CABINET ID | 1 | non acquisiti | nessun ciclo |

### INDY 500 Twin (`indy500to`)

- **Parent:** `indy500`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/indy500to/screenshots/indy500to-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 2 | RACE MODE | NORMAL | non acquisiti | nessun ciclo |
| 3 | HANDICAP | ON | non acquisiti | nessun ciclo |
| 4 | ADVERTISE SOUND | OFF | non acquisiti | nessun ciclo |
| 5 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 6 | CABINET TYPE | TWIN | non acquisiti | nessun ciclo |
| 7 | NETWORK TYPE | STAND ALONE | non acquisiti | nessun ciclo |
| 8 | CABINET ID | 1 | non acquisiti | nessun ciclo |
| 9 | ENGINE VOLUME | 3 | non acquisiti | nessun ciclo |
| 10 | DEFAULT VIEW | 4 | non acquisiti | nessun ciclo |

### Last Bronx: Tokyo Bangaichi (`lastbrnxj`)

- **Parent:** `lastbrnx`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/lastbrnxj/screenshots/lastbrnxj-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | 1 NORMAL | non acquisiti | nessun ciclo |
| 2 | ADVERTISE SOUND | OFF | non acquisiti | nessun ciclo |
| 3 | VS FINISH | OFF | non acquisiti | nessun ciclo |
| 4 | SURVIVAL MODE | ON | non acquisiti | nessun ciclo |
| 5 | MATCH POINT (CPU) | 2 | non acquisiti | nessun ciclo |
| 6 | MATCH POINT (VS) | 2 | non acquisiti | nessun ciclo |
| 7 | CUT CROSS STREET | OFF | non acquisiti | nessun ciclo |
| 8 | DISPLAY TYPE | C.R.T. | non acquisiti | nessun ciclo |
| 9 | MASTER VOLUME | 3 | non acquisiti | nessun ciclo |

### Last Bronx (`lastbrnxu`)

- **Parent:** `lastbrnx`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/lastbrnxu/screenshots/lastbrnxu-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | 1 NORMAL | non acquisiti | nessun ciclo |
| 2 | ADVERTISE SOUND | OFF | non acquisiti | nessun ciclo |
| 3 | VS FINISH | OFF | non acquisiti | nessun ciclo |
| 4 | SURVIVAL MODE | ON | non acquisiti | nessun ciclo |
| 5 | MATCH POINT (CPU) | 2 | non acquisiti | nessun ciclo |
| 6 | MATCH POINT (VS) | 2 | non acquisiti | nessun ciclo |
| 7 | CUT CROSS STREET | OFF | non acquisiti | nessun ciclo |
| 8 | DISPLAY TYPE | C.R.T. | non acquisiti | nessun ciclo |
| 9 | MASTER VOLUME | 3 | non acquisiti | nessun ciclo |

### Manx TT Superbike - DX/Twin (`manxttc`)

- **Parent:** `manxtt`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/manxttc/screenshots/manxttc-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 3 | CABINET TYPE | TWIN | non acquisiti | nessun ciclo |
| 4 | LINK TYPE | Not Link | non acquisiti | nessun ciclo |
| 5 | BIKE COLOR (No.) | RED (No.1) | non acquisiti | nessun ciclo |
| 6 | RACE MODE | RACE | non acquisiti | nessun ciclo |
| 7 | LAXEY NUMBER OF LAP | 2 | non acquisiti | nessun ciclo |
| 8 | LAXEY GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 9 | LAXEY REVISE MODE | 2/3 | non acquisiti | nessun ciclo |
| 10 | TT NUMBER OF LAP | 2 | non acquisiti | nessun ciclo |
| 11 | TT GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 12 | TT REVISE MODE | 2/3 | non acquisiti | nessun ciclo |
| 13 | START SWITCH OP. | OFF | non acquisiti | nessun ciclo |

### Manx TT Superbike - DX (`manxttdx`)

- **Parent:** `manxtt`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/manxttdx/screenshots/manxttdx-menu-structure-settings-main.png>)

- **Differenze strutturali:** rimosse: cabinet-type, link-type, laxey-revise-mode, tt-revise-mode.
- **Nota:** CABINET TYPE, LINK TYPE and both REVISE MODE rows are absent.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 3 | BIKE COLOR (No.) | RED (No.1) | non acquisiti | nessun ciclo |
| 4 | RACE MODE | RACE | non acquisiti | nessun ciclo |
| 5 | LAXEY NUMBER OF LAP | 2 | non acquisiti | nessun ciclo |
| 6 | LAXEY GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 7 | TT NUMBER OF LAP | 2 | non acquisiti | nessun ciclo |
| 8 | TT GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 9 | START SWITCH OP. | OFF | non acquisiti | nessun ciclo |

### Motor Raid - Twin/DX (`motoraiddx`)

- **Parent:** `motoraid`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/motoraiddx/screenshots/motoraiddx-menu-structure-settings-main.png>)

- **Differenze strutturali:** aggiunte: cabinet-type.
- **Nota:** Adds CABINET TYPE; ENGINE VOLUME is OUT OF USE for Deluxe.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 2 | RACE MODE | STANDARD | non acquisiti | nessun ciclo |
| 3 | ENEMY LEVEL | NORMAL | non acquisiti | nessun ciclo |
| 4 | ENGINE VOLUME | OUT OF USE | non acquisiti | nessun ciclo |
| 5 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 6 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 7 | NETWORK TYPE | STAND ALONE | non acquisiti | nessun ciclo |
| 8 | CABINET ID | 1 | non acquisiti | nessun ciclo |
| 9 | CABINET TYPE | DELUXE | DELUXE, TWIN | ciclo clone |

### Over Rev (`overrevb`)

- **Parent:** `overrev`
- **Schermate acquisite:** [schermata 1](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/game-settings-gap-fill-20260920/overrevb/screenshots/overrevb-foundational-settings-foundational-settings.png>), [schermata 2](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/game-settings-gap-fill-20260920/overrevb/screenshots/overrevb-optional-game-settings-optional-game-settings.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.
- **Nota:** Acquisiti entrambi i sottomenu; l'insieme delle righe coincide con il parent.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | COUNTRY | U.S.A. | non acquisiti | nessun ciclo |
| 2 | HARDWARE TYPE | NORMAL (2in1) | non acquisiti | nessun ciclo |
| 3 | LINK MAX | NOT LINK | non acquisiti | nessun ciclo |
| 4 | LINK TYPE | MASTER CarNo.1 | non acquisiti | nessun ciclo |
| 5 | TIME DIFFICULTY | 3 | non acquisiti | nessun ciclo |
| 6 | STEER KICK BACK | 3 | non acquisiti | nessun ciclo |
| 7 | GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 8 | DEMO SOUND | ON | non acquisiti | nessun ciclo |
| 9 | MAXIMUM LAP | NORMAL MODE | non acquisiti | nessun ciclo |

### Over Rev (`overrevba`)

- **Parent:** `overrev`
- **Schermate acquisite:** [schermata 1](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/game-settings-gap-fill-20260920/overrevba/screenshots/overrevba-foundational-settings-foundational-settings.png>), [schermata 2](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/game-settings-gap-fill-20260920/overrevba/screenshots/overrevba-optional-game-settings-optional-game-settings.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.
- **Nota:** Acquisiti entrambi i sottomenu; l'insieme delle righe coincide con il parent.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | COUNTRY | U.S.A. | non acquisiti | nessun ciclo |
| 2 | HARDWARE TYPE | NORMAL (2in1) | non acquisiti | nessun ciclo |
| 3 | LINK MAX | NOT LINK | non acquisiti | nessun ciclo |
| 4 | LINK TYPE | MASTER CarNo.1 | non acquisiti | nessun ciclo |
| 5 | TIME DIFFICULTY | 3 | non acquisiti | nessun ciclo |
| 6 | STEER KICK BACK | 3 | non acquisiti | nessun ciclo |
| 7 | GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 8 | DEMO SOUND | ON | non acquisiti | nessun ciclo |
| 9 | MAXIMUM LAP | NORMAL MODE | non acquisiti | nessun ciclo |

### Pilot Kids (`pltkidsa`)

- **Parent:** `pltkids`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/pltkidsa/screenshots/pltkidsa-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | Fighters | 3 | non acquisiti | nessun ciclo |
| 2 | Difficult | Normal | non acquisiti | nessun ciclo |
| 3 | Demo Sound | Off | non acquisiti | nessun ciclo |
| 4 | Coin Slot | Same | non acquisiti | nessun ciclo |
| 5 | Coin Mode | Normal | non acquisiti | nessun ciclo |
| 6 | Coin-1 | 1 Coin = 1 Credit | non acquisiti | nessun ciclo |
| 7 | Coin-2 | 1 Coin = 1 Credit | non acquisiti | nessun ciclo |
| 8 | Continue | On | non acquisiti | nessun ciclo |

### Rail Chase 2 (`rchase2a`)

- **Parent:** `rchase2`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/rchase2a/screenshots/rchase2a-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 3 | GAME DIFFICULTY | 4/8 | non acquisiti | nessun ciclo |
| 4 | SHIFTING DIFFICULTY | 50 sec EVERY | non acquisiti | nessun ciclo |

### Sonic the Fighters (`sfight`)

- **Parent:** `schamp`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/sfight/screenshots/sfight-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | MATCH COUNT (1P) | 2 | non acquisiti | nessun ciclo |
| 2 | MATCH COUNT (VS) | 2 | non acquisiti | nessun ciclo |
| 3 | ENEMY RANK | NORMAL | non acquisiti | nessun ciclo |
| 4 | TIME | 30 | non acquisiti | nessun ciclo |
| 5 | ENERGY (1P) | NORMAL | non acquisiti | nessun ciclo |
| 6 | ENERGY (VS) | NORMAL | non acquisiti | nessun ciclo |
| 7 | BARRIER | 5 | non acquisiti | nessun ciclo |
| 8 | BARRIER RESET | OFF | non acquisiti | nessun ciclo |
| 9 | AUTOMATIC | OFF | non acquisiti | nessun ciclo |
| 10 | HYPER MODE | ON | non acquisiti | nessun ciclo |
| 11 | DAMAGE | NORMAL | non acquisiti | nessun ciclo |
| 12 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 13 | CONTINUE | ON | non acquisiti | nessun ciclo |
| 14 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 15 | DISPLAY TYPE | C.R.T. | non acquisiti | nessun ciclo |
| 16 | VS FINISH | OFF | non acquisiti | nessun ciclo |

### Sega Rally Championship - Twin/DX (`srallycb`)

- **Parent:** `srallyc`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/srallycb/screenshots/srallycb-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `srallyc` |
| 2 | COUNTRY | JPN | EXPORT, JPN, USA | campagna parent `srallyc` |
| 3 | CABINET TYPE | TWIN | DELUXE, TWIN | campagna parent `srallyc` |
| 4 | LINK TYPE | NOTLINK | CAR 1, CAR 2, CAR 3, CAR 4, NOTLINK, RELAY | campagna parent `srallyc` |
| 5 | GAME DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | campagna parent `srallyc` |
| 6 | GAME MODE | NORMAL | LONG, LONGEST, NORMAL, SHORT | campagna parent `srallyc` |

### Sega Rally Championship - Twin/DX (`srallycc`)

- **Parent:** `srallyc`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/srallycc/screenshots/srallycc-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `srallyc` |
| 2 | COUNTRY | JPN | EXPORT, JPN, USA | campagna parent `srallyc` |
| 3 | CABINET TYPE | TWIN | DELUXE, TWIN | campagna parent `srallyc` |
| 4 | LINK TYPE | NOTLINK | CAR 1, CAR 2, CAR 3, CAR 4, NOTLINK, RELAY | campagna parent `srallyc` |
| 5 | GAME DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | campagna parent `srallyc` |
| 6 | GAME MODE | NORMAL | LONG, LONGEST, NORMAL, SHORT | campagna parent `srallyc` |

### Sega Rally Championship - DX (`srallycdx`)

- **Parent:** `srallyc`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/srallycdx/screenshots/srallycdx-menu-structure-settings-main.png>)

- **Differenze strutturali:** rimosse: cabinet-type, link-type.
- **Nota:** DX menu has only four rows; CABINET TYPE and LINK TYPE are absent.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `srallyc` |
| 2 | COUNTRY | JPN | EXPORT, JPN, USA | campagna parent `srallyc` |
| 3 | GAME DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | campagna parent `srallyc` |
| 4 | GAME MODE | NORMAL | LONG, LONGEST, NORMAL, SHORT | campagna parent `srallyc` |

### Sega Rally Championship - DX (`srallycdxa`)

- **Parent:** `srallyc`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/srallycdxa/screenshots/srallycdxa-menu-structure-settings-main.png>)

- **Differenze strutturali:** rimosse: cabinet-type, link-type.
- **Nota:** DX menu has only four rows; a dedicated value campaign is archived.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `srallyc` |
| 2 | COUNTRY | JPN | EXPORT, JPN, USA | campagna parent `srallyc` |
| 3 | GAME DIFFICULTY | NORMAL | EASY, HARD, HARDEST, NORMAL | campagna parent `srallyc` |
| 4 | GAME MODE | NORMAL | LONG, LONGEST, NORMAL, SHORT | campagna parent `srallyc` |

### Sega Touring Car Championship (`stcca`)

- **Parent:** `stcc`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/stcca/screenshots/stcca-country-usa-country-usa.png>)

- **Differenze strutturali:** rimosse: default-view.
- **Nota:** DEFAULT VIEW is absent. The archived screen intentionally shows COUNTRY=USA and is not a native-default capture.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | URL ADDRESS | ON | non acquisiti | nessun ciclo |
| 3 | COUNTRY | USA (selected capture; native default not established) | non acquisiti | nessun ciclo |
| 4 | CABINET TYPE | TWIN | non acquisiti | nessun ciclo |
| 5 | LINK TYPE | STAND ALONE | non acquisiti | nessun ciclo |
| 6 | DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 7 | GAME MODE | NORMAL | non acquisiti | nessun ciclo |
| 8 | DEFAULT CAR | RANDOM | non acquisiti | nessun ciclo |
| 9 | NAME ENTRY | BEFORE-3 | non acquisiti | nessun ciclo |

### Sega Touring Car Championship (`stccb`)

- **Parent:** `stcc`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/stccb/screenshots/stccb-native-menu-game-assignments.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | URL ADDRESS | ON | non acquisiti | nessun ciclo |
| 3 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 4 | CABINET TYPE | TWIN | non acquisiti | nessun ciclo |
| 5 | LINK TYPE | STAND ALONE | non acquisiti | nessun ciclo |
| 6 | DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 7 | GAME MODE | NORMAL | non acquisiti | nessun ciclo |
| 8 | DEFAULT CAR | RANDOM | non acquisiti | nessun ciclo |
| 9 | NAME ENTRY | BEFORE-3 | non acquisiti | nessun ciclo |
| 10 | DEFAULT VIEW | DRIVER'S | non acquisiti | nessun ciclo |

### Sega Touring Car Championship (`stcco`)

- **Parent:** `stcc`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/stcco/screenshots/stcco-country-usa-country-usa.png>)

- **Differenze strutturali:** rimosse: default-view.
- **Nota:** DEFAULT VIEW is absent. The archived screen intentionally shows COUNTRY=USA and is not a native-default capture.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | URL ADDRESS | ON | non acquisiti | nessun ciclo |
| 3 | COUNTRY | USA (selected capture; native default not established) | non acquisiti | nessun ciclo |
| 4 | CABINET TYPE | TWIN | non acquisiti | nessun ciclo |
| 5 | LINK TYPE | STAND ALONE | non acquisiti | nessun ciclo |
| 6 | DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 7 | GAME MODE | NORMAL | non acquisiti | nessun ciclo |
| 8 | DEFAULT CAR | RANDOM | non acquisiti | nessun ciclo |
| 9 | NAME ENTRY | AFTER-3 | non acquisiti | nessun ciclo |

### Top Skater (`topskatrj`)

- **Parent:** `topskatr`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/topskatrj/screenshots/topskatrj-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | GAME DIFFICULTY | 4/8 | non acquisiti | nessun ciclo |

### Top Skater (`topskatru`)

- **Parent:** `topskatr`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/topskatru/screenshots/topskatru-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | GAME DIFFICULTY | 4/8 | non acquisiti | nessun ciclo |

### Top Skater (`topskatruo`)

- **Parent:** `topskatr`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/topskatruo/screenshots/topskatruo-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | GAME DIFFICULTY | 4/8 | non acquisiti | nessun ciclo |

### Virtua Cop (`vcopa`)

- **Parent:** `vcop`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/vcopa/screenshots/vcopa-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | OFF, ON | campagna parent `vcop` |
| 2 | COUNTRY | JPN | EXP, JPN, USA | campagna parent `vcop` |
| 3 | CABINET | DX | DX, SP U/R, U/R | campagna parent `vcop` |
| 4 | DIFFICULTY | NORMAL | EASIEST, EASY, HARD, HARDEST, MEDIUM EASY, MEDIUM HARD, NORMAL, VERY EASY, VERY HARD | campagna parent `vcop` |
| 5 | LIFE | 6 | 1, 2, 3, 4, 5, 6, 7, 8, 9 | campagna parent `vcop` |
| 6 | HUMAN TYPE | NORMAL | — | visible-only |
| 7 | RELOAD TYPE | NORMAL | — | visible-only |

### Virtua Fighter 2 (`vf2a`)

- **Parent:** `vf2`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/vf2a/screenshots/vf2a-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | MATCH COUNT (1P) | 2 | non acquisiti | nessun ciclo |
| 2 | MATCH COUNT (VS) | 2 | non acquisiti | nessun ciclo |
| 3 | DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 4 | ENERGY MAX (1P) | 168 | — | visible-only |
| 5 | ENERGY MAX (VS) | 288 | — | visible-only |
| 6 | STAGE WIDTH | 1580 | — | visible-only |
| 7 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 8 | CONTINUE | ON | non acquisiti | nessun ciclo |
| 9 | DRINK | OK | non acquisiti | nessun ciclo |
| 10 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 11 | DISPLAY TYPE | PROJECTOR | non acquisiti | nessun ciclo |
| 12 | VS FINISH | OFF | non acquisiti | nessun ciclo |
| 13 | RANKING MODE | OFF | non acquisiti | nessun ciclo |
| 14 | VERSION | NORMAL | non acquisiti | nessun ciclo |

### Virtua Fighter 2 (`vf2b`)

- **Parent:** `vf2`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/vf2b/screenshots/vf2b-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | MATCH COUNT (1P) | 2 | non acquisiti | nessun ciclo |
| 2 | MATCH COUNT (VS) | 2 | non acquisiti | nessun ciclo |
| 3 | DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 4 | ENERGY MAX (1P) | 168 | — | visible-only |
| 5 | ENERGY MAX (VS) | 288 | — | visible-only |
| 6 | STAGE WIDTH | 1580 | — | visible-only |
| 7 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 8 | CONTINUE | ON | non acquisiti | nessun ciclo |
| 9 | DRINK | OK | non acquisiti | nessun ciclo |
| 10 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 11 | DISPLAY TYPE | PROJECTOR | non acquisiti | nessun ciclo |
| 12 | VS FINISH | OFF | non acquisiti | nessun ciclo |
| 13 | RANKING MODE | OFF | non acquisiti | nessun ciclo |
| 14 | VERSION | NORMAL | non acquisiti | nessun ciclo |

### Virtua Fighter 2 (`vf2o`)

- **Parent:** `vf2`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/vf2o/screenshots/vf2o-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | MATCH COUNT (1P) | 2 | non acquisiti | nessun ciclo |
| 2 | MATCH COUNT (VS) | 2 | non acquisiti | nessun ciclo |
| 3 | DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 4 | ENERGY MAX (1P) | 168 | — | visible-only |
| 5 | ENERGY MAX (VS) | 288 | — | visible-only |
| 6 | STAGE WIDTH | 1580 | — | visible-only |
| 7 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 8 | CONTINUE | ON | non acquisiti | nessun ciclo |
| 9 | DRINK | OK | non acquisiti | nessun ciclo |
| 10 | COUNTRY | JAPAN | non acquisiti | nessun ciclo |
| 11 | DISPLAY TYPE | PROJECTOR | non acquisiti | nessun ciclo |
| 12 | VS FINISH | OFF | non acquisiti | nessun ciclo |
| 13 | RANKING MODE | OFF | non acquisiti | nessun ciclo |
| 14 | VERSION | NORMAL | non acquisiti | nessun ciclo |

### Cyber Troopers Virtual-On - Twin (`vonj`)

- **Parent:** `von`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/vonj/screenshots/vonj-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | PLAY TIME 1P (STAGE 1~5) | 90 SECS | non acquisiti | nessun ciclo |
| 2 | PLAY TIME 1P (PENALTY) | 90 SECS | non acquisiti | nessun ciclo |
| 3 | PLAY TIME 1P (STAGE 6~8) | 90 SECS | non acquisiti | nessun ciclo |
| 4 | PLAY TIME 1P (LAST STG) | 90 SECS | non acquisiti | nessun ciclo |
| 5 | PLAY TIME VERSUS | 90 SECS | non acquisiti | nessun ciclo |
| 6 | MATCH COUNT 1P (STAGE 1~5) | 1 | non acquisiti | nessun ciclo |
| 7 | MATCH COUNT 1P (PENALTY) | 1 | non acquisiti | nessun ciclo |
| 8 | MATCH COUNT 1P (STAGE 6~8) | 1 | non acquisiti | nessun ciclo |
| 9 | MATCH COUNT VERSUS | 1 | non acquisiti | nessun ciclo |
| 10 | NETWORK LINK ATTRIBUTE | NO LINK | non acquisiti | nessun ciclo |
| 11 | WINNING BY DECISION | ON | non acquisiti | nessun ciclo |
| 12 | GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 13 | ADVERTISE SOUND | LOUD | non acquisiti | nessun ciclo |
| 14 | CONTINUE | ON | non acquisiti | nessun ciclo |
| 15 | REPLAY AND POSING MODE | REPLAY & POSING | non acquisiti | nessun ciclo |
| 16 | RANKING MODE | ON | non acquisiti | nessun ciclo |
| 17 | VERSUS ALWAYS FINISH | OFF | non acquisiti | nessun ciclo |
| 18 | DISPLAY BRIGHTNESS | 0 | non acquisiti | nessun ciclo |

### Cyber Troopers Virtual-On - Relay (`vonr`)

- **Parent:** `von`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/game-settings-gap-fill-20260920/vonr/screenshots/vonr-menu-navigation-game-assignments.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.
- **Nota:** Game Assignments diventa accessibile dopo il timeout della rete Relay; righe e default coincidono con il parent.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | PLAY TIME 1P (STAGE 1~5) | 90 SECS | non acquisiti | nessun ciclo |
| 2 | PLAY TIME 1P (PENALTY) | 90 SECS | non acquisiti | nessun ciclo |
| 3 | PLAY TIME 1P (STAGE 6~8) | 90 SECS | non acquisiti | nessun ciclo |
| 4 | PLAY TIME 1P (LAST STG) | 90 SECS | non acquisiti | nessun ciclo |
| 5 | PLAY TIME VERSUS | 90 SECS | non acquisiti | nessun ciclo |
| 6 | MATCH COUNT 1P (STAGE 1~5) | 1 | non acquisiti | nessun ciclo |
| 7 | MATCH COUNT 1P (PENALTY) | 1 | non acquisiti | nessun ciclo |
| 8 | MATCH COUNT 1P (STAGE 6~8) | 1 | non acquisiti | nessun ciclo |
| 9 | MATCH COUNT VERSUS | 1 | non acquisiti | nessun ciclo |
| 10 | NETWORK LINK ATTRIBUTE | NO LINK | non acquisiti | nessun ciclo |
| 11 | WINNING BY DECISION | ON | non acquisiti | nessun ciclo |
| 12 | GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 13 | ADVERTISE SOUND | LOUD | non acquisiti | nessun ciclo |
| 14 | CONTINUE | ON | non acquisiti | nessun ciclo |
| 15 | REPLAY AND POSING MODE | REPLAY & POSING | non acquisiti | nessun ciclo |
| 16 | RANKING MODE | ON | non acquisiti | nessun ciclo |
| 17 | VERSUS ALWAYS FINISH | OFF | non acquisiti | nessun ciclo |
| 18 | DISPLAY BRIGHTNESS | 0 | non acquisiti | nessun ciclo |

### Cyber Troopers Virtual-On - Twin (`vonu`)

- **Parent:** `von`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/vonu/screenshots/vonu-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | PLAY TIME 1P (STAGE 1~5) | 90 SECS | non acquisiti | nessun ciclo |
| 2 | PLAY TIME 1P (PENALTY) | 90 SECS | non acquisiti | nessun ciclo |
| 3 | PLAY TIME 1P (STAGE 6~8) | 90 SECS | non acquisiti | nessun ciclo |
| 4 | PLAY TIME 1P (LAST STG) | 90 SECS | non acquisiti | nessun ciclo |
| 5 | PLAY TIME VERSUS | 90 SECS | non acquisiti | nessun ciclo |
| 6 | MATCH COUNT 1P (STAGE 1~5) | 1 | non acquisiti | nessun ciclo |
| 7 | MATCH COUNT 1P (PENALTY) | 1 | non acquisiti | nessun ciclo |
| 8 | MATCH COUNT 1P (STAGE 6~8) | 1 | non acquisiti | nessun ciclo |
| 9 | MATCH COUNT VERSUS | 1 | non acquisiti | nessun ciclo |
| 10 | NETWORK LINK ATTRIBUTE | NO LINK | non acquisiti | nessun ciclo |
| 11 | WINNING BY DECISION | ON | non acquisiti | nessun ciclo |
| 12 | GAME DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 13 | ADVERTISE SOUND | LOUD | non acquisiti | nessun ciclo |
| 14 | CONTINUE | ON | non acquisiti | nessun ciclo |
| 15 | REPLAY AND POSING MODE | REPLAY & POSING | non acquisiti | nessun ciclo |
| 16 | RANKING MODE | ON | non acquisiti | nessun ciclo |
| 17 | VERSUS ALWAYS FINISH | OFF | non acquisiti | nessun ciclo |
| 18 | DISPLAY BRIGHTNESS | 0 | non acquisiti | nessun ciclo |

### Virtua Striker (`vstrikero`)

- **Parent:** `vstriker`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/vstrikero/screenshots/vstrikero-menu-structure-settings-main.png>)

- **Differenze strutturali:** rimosse: one-match-mode.
- **Nota:** ONE MATCH MODE is absent.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | ADVERTISE SOUND | ON | non acquisiti | nessun ciclo |
| 2 | COUNTRY | JPN | non acquisiti | nessun ciclo |
| 3 | MONITOR | CRT | non acquisiti | nessun ciclo |
| 4 | DIFFICULTY | NORMAL | non acquisiti | nessun ciclo |
| 5 | TIME SET | 2:00 | non acquisiti | nessun ciclo |
| 6 | V GOAL SYSTEM | OFF | non acquisiti | nessun ciclo |
| 7 | V GOAL TIME SET | 0:15 | non acquisiti | nessun ciclo |
| 8 | PK SYSTEM | OFF | non acquisiti | nessun ciclo |
| 9 | PK MEMBER SET | 3 | non acquisiti | nessun ciclo |
| 10 | BILLBOARD | ON | non acquisiti | nessun ciclo |

### Zero Gunner (`zerogunaj`)

- **Parent:** `zeroguna`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/zerogunaj/screenshots/zerogunaj-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | Credit Mode | Same | non acquisiti | nessun ciclo |
| 2 | Continue Mode | Normal Mode | non acquisiti | nessun ciclo |
| 3 | Coin Slot 1 | 1 coin / 1 credit | non acquisiti | nessun ciclo |
| 4 | Coin Slot 2 | 1 coin / 1 credit | non acquisiti | nessun ciclo |
| 5 | Demo Sound | ON | non acquisiti | nessun ciclo |
| 6 | Difficulty | NORMAL | non acquisiti | nessun ciclo |
| 7 | Fighters | 3 fighters | non acquisiti | nessun ciclo |
| 8 | Extend Points | 600000 | non acquisiti | nessun ciclo |
| 9 | Ranking Data | Do Initialize | non acquisiti | nessun ciclo |

### Zero Gunner (`zerogunj`)

- **Parent:** `zerogun`
- **Schermate acquisite:** [apri screenshot](</Users/andrea/dev/sm2-emu-libretro/build-libretro-gpu/validation/clone-service-menu-archive-20260915/zerogunj/screenshots/zerogunj-menu-structure-settings-main.png>)

- **Differenze strutturali:** nessuna riga aggiunta o rimossa.

| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |
| ---: | --- | --- | --- | --- |
| 1 | Credit Mode | Same | non acquisiti | nessun ciclo |
| 2 | Continue Mode | Normal Mode | non acquisiti | nessun ciclo |
| 3 | Coin Slot 1 | 1 coin / 1 credit | non acquisiti | nessun ciclo |
| 4 | Coin Slot 2 | 1 coin / 1 credit | non acquisiti | nessun ciclo |
| 5 | Demo Sound | ON | non acquisiti | nessun ciclo |
| 6 | Difficulty | NORMAL | non acquisiti | nessun ciclo |
| 7 | Fighters | 3 fighters | non acquisiti | nessun ciclo |
| 8 | Extend Points | 600000 | non acquisiti | nessun ciclo |
| 9 | Ranking Data | Do Initialize | non acquisiti | nessun ciclo |

## Lacune residue

Nessuna. Tutti i parent e i cloni presenti in `games.xml` hanno una trascrizione strutturata oppure, nel caso speciale `rascot2`, un esito diagnostico conclusivo senza Game Settings ordinari.
