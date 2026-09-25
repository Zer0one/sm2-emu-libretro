# Confronto NVRAM predefinita dei cloni Model 2

## Ambito

La prima fase avvia ogni clone da una directory di salvataggio vuota per
900 frame emulati senza input, acquisisce la backup RAM nativa da 16 KiB e la
EEPROM da 128 byte e confronta entrambi i file byte per byte con una nuova
acquisizione del relativo parent. Non esamina le voci del Service Menu e non
deduce offset specifici dei cloni. Una differenza indica quindi la necessità di
una successiva analisi dedicata, senza attribuire ancora un significato ai byte.

- Revisione metadata del progetto: `fc541deb961f46e6db06e3e94ba424b5f498b1c6`
- Eseguibile: standalone macOS del repository corrente (`sm2-emu 0.9.7`)
- Cloni acquisiti: **48/48**
- Identici al parent: **21**
- Differenze stabili dal parent: **27**
- Acquisizioni fallite: **0**
- Cloni `preliminary` acquisiti: **16/16**

I 22 parent sono stati acquisiti una seconda volta nelle stesse condizioni.
Tutte le ripetizioni sono risultate identiche byte per byte: nessuna differenza
clone/parent osservata è spiegata da variabilità tra due avvii equivalenti.

## Esito sintetico

Compatibilità iniziale con il campione parent, perché entrambe le memorie sono
identiche: `daytonam`, `fvipersa`, `fvipersb`, `hotdo`, `lastbrnxj`, `lastbrnxu`, `overrevb`, `overrevba`, `pltkidsa`, `rchase2a`, `srallycb`, `srallycc`, `topskatrj`, `topskatru`, `topskatruo`, `vcopa`, `vonj`, `vonr`, `vonu`, `zerogunaj`, `zerogunj`.

Dispongono ora di un campione dedicato verificato con le opzioni del parent:
`daytonase`, `doaa`, `doaab`, `doaae`, `doab`, `dynamcopb`, `dynamcopc`,
`indy500to`, `manxttc`, `manxttdx`, `sfight`, `srallycdx`, `vf2b`.

Quattordici cloni dispongono di un campione e di un catalogo NVRAM specifici
ricavati dal proprio Service Menu: `daytona93`, `daytonas`, `dyndeka2`,
`dyndeka2b`, `hotdp`, `indy500d`, `motoraiddx`, `stcca`, `stccb`, `stcco`,
`vf2a`, `vf2o`, `vstrikero` e `srallycdxa`.

Tutti i cloni sono coperti da un template dedicato o dall’eredità parent
verificata.

## Risultati completi

| Clone | Parent | Preliminary | Byte differenti backup RAM | Byte differenti EEPROM | Differisce dal parent |
|---|---|---:|---:|---:|---|
| `daytona93` | `daytona` | Sì | 239 | 80 | Sì |
| `daytonam` | `daytona` | No | 0 | 0 | No |
| `daytonas` | `daytona` | Sì | 12 | 6 | Sì |
| `daytonase` | `daytona` | Sì | 8 | 4 | Sì |
| `doaa` | `doa` | Sì | 1 | 0 | Sì |
| `doaab` | `doa` | Sì | 1 | 4 | Sì |
| `doaae` | `doa` | Sì | 1 | 4 | Sì |
| `doab` | `doa` | Sì | 1 | 0 | Sì |
| `dynamcopb` | `dynamcop` | No | 3 | 0 | Sì |
| `dynamcopc` | `dynamcop` | No | 3 | 9 | Sì |
| `dyndeka2` | `dynamcop` | No | 0 | 1 | Sì |
| `dyndeka2b` | `dynamcop` | No | 3 | 1 | Sì |
| `fvipersa` | `fvipers` | No | 0 | 0 | No |
| `fvipersb` | `fvipers` | No | 0 | 0 | No |
| `hotdo` | `hotd` | No | 0 | 0 | No |
| `hotdp` | `hotd` | No | 239 | 62 | Sì |
| `indy500d` | `indy500` | No | 827 | 68 | Sì |
| `indy500to` | `indy500` | No | 0 | 30 | Sì |
| `lastbrnxj` | `lastbrnx` | Sì | 0 | 0 | No |
| `lastbrnxu` | `lastbrnx` | Sì | 0 | 0 | No |
| `manxttc` | `manxtt` | Sì | 0 | 4 | Sì |
| `manxttdx` | `manxtt` | Sì | 520 | 88 | Sì |
| `motoraiddx` | `motoraid` | No | 0 | 7 | Sì |
| `overrevb` | `overrev` | No | 0 | 0 | No |
| `overrevba` | `overrev` | No | 0 | 0 | No |
| `pltkidsa` | `pltkids` | No | 0 | 0 | No |
| `rchase2a` | `rchase2` | No | 0 | 0 | No |
| `sfight` | `schamp` | Sì | 5 | 0 | Sì |
| `srallycb` | `srallyc` | No | 0 | 0 | No |
| `srallycc` | `srallyc` | No | 0 | 0 | No |
| `srallycdx` | `srallyc` | No | 0 | 6 | Sì |
| `srallycdxa` | `srallyc` | No | 5548 | 22 | Sì |
| `stcca` | `stcc` | No | 90 | 17 | Sì |
| `stccb` | `stcc` | No | 90 | 18 | Sì |
| `stcco` | `stcc` | No | 912 | 18 | Sì |
| `topskatrj` | `topskatr` | Sì | 0 | 0 | No |
| `topskatru` | `topskatr` | Sì | 0 | 0 | No |
| `topskatruo` | `topskatr` | Sì | 0 | 0 | No |
| `vcopa` | `vcop` | Sì | 0 | 0 | No |
| `vf2a` | `vf2` | No | 3 | 0 | Sì |
| `vf2b` | `vf2` | No | 1 | 0 | Sì |
| `vf2o` | `vf2` | No | 3 | 0 | Sì |
| `vonj` | `von` | No | 0 | 0 | No |
| `vonr` | `von` | No | 0 | 0 | No |
| `vonu` | `von` | No | 0 | 0 | No |
| `vstrikero` | `vstriker` | No | 138 | 0 | Sì |
| `zerogunaj` | `zeroguna` | No | 0 | 0 | No |
| `zerogunj` | `zerogun` | No | 0 | 0 | No |

“Differisce dal parent: No” significa che entrambe le memorie acquisite sono
identiche al parent. “Sì” registra la differenza osservata nella prima fase;
lo stato dell’analisi successiva è riportato nelle sezioni seguenti.

## Verifica di derivabilità dai campi noti

Per i 27 cloni differenti è stata eseguita una seconda verifica senza entrare
nel Service Menu. Per ogni clone il controllo parte dai due campioni del parent,
riconosce soltanto valori già presenti nel catalogo NVRAM del parent, applica le
relative patch tramite lo stesso codice usato dal core e rigenera checksum e
copie speculari. Il clone è considerato derivabile soltanto se il risultato
coincide byte per byte con entrambe le memorie acquisite.

- Cloni differenti ricostruiti interamente con soli campi noti: **0/27**.
- Cloni con almeno una variazione nota, ma anche differenze residue: **11/27**.
- Cloni senza variazioni riconducibili ai campi parent attualmente noti:
  **16/27**.

| Clone | Variazioni note riconosciute | Differenze residue backup RAM | Differenze residue EEPROM |
|---|---|---:|---:|
| `daytona93` | Link ID: Single; Cabinet: Deluxe; Rival Arrow: Off | 235 | 78 |
| `daytonas` | Country: USA | 10 | 5 |
| `daytonase` | Cabinet: Upright | 2 | 1 |
| `indy500d` | Country: Japan | 827 | 66 |
| `indy500to` | Country: Japan; Engine Volume: 2 | 0 | 26 |
| `manxttc` | Cabinet Type: Twin | 0 | 3 |
| `sfight` | Automatic: Off; Country: Japan | 1 | 0 |
| `srallycdx` | Cabinet Type: Deluxe | 0 | 5 |
| `srallycdxa` | Cabinet Type: Deluxe | 5548 | 21 |
| `stccb` | Default View: Driver's | 90 | 17 |
| `vstrikero` | One Match Mode: On | 136 | 0 |

I 16 cloni senza una variazione riconosciuta sono: `doaa`, `doaab`, `doaae`,
`doab`, `dynamcopb`, `dynamcopc`, `dyndeka2`, `dyndeka2b`, `hotdp`, `manxttdx`,
`motoraiddx`, `stcca`, `stcco`, `vf2a`, `vf2b`, `vf2o`.

Questa verifica da sola non abilita campioni derivati. I 21 cloni già identici
al parent sono compatibili con l’eredità diretta; per cinque cloni differenti è
stata successivamente verificata l’alternativa descritta sotto.

## Campioni clone usati con i parametri del parent

I campioni nativi di `daytonase`, `indy500to`, `manxttc`, `sfight` e
`srallycdx` superano il validatore del rispettivo parent. Tutti i **149 valori**
complessivamente esposti dai cinque cataloghi parent sono stati applicati uno
alla volta al campione clone; ogni scrittura ha mantenuto validi layout,
checksum e mirror.

I cinque campioni sono stati quindi integrati come template dedicati. Il primo
avvio pulito in RetroArch Nightly 1.22.2 ha applicato il setup automatico prima
del primo frame; il secondo ha importato il `.srm` esistente senza rieseguire il
setup. Tutte le dieci sessioni sono terminate con exit code 0, audio stereo non
silenzioso, contenitore da 16.576 byte valido e gioco o gara reale visibile.

| Clone | Default finali rilevanti dopo il secondo avvio | Esito |
|---|---|---|
| `daytonase` | Link ID Single, Cabinet Deluxe, Country USA | Avvio, gara e persistenza validi |
| `indy500to` | Country USA, Network Type Stand Alone, Engine Volume 2 | Avvio, gara e persistenza validi |
| `manxttc` | Country USA, Cabinet Type Twin, Link Type Not Link | Avvio, gara e persistenza validi |
| `sfight` | Country USA, Automatic Off, Display Type C.R.T. | Avvio, combattimento e persistenza validi |
| `srallycdx` | Country USA, Cabinet Type Deluxe, Link Type Notlink | Avvio, gara e persistenza validi |

Il core consente l’eredità del template parent soltanto ai 21 cloni con campioni
byte-identici. Gli altri cloni abilitati usano sempre un template specifico; i
sei casi ancora da analizzare non ricevono implicitamente il campione parent.

## Analisi dei 22 cloni inizialmente rimanenti

Ogni campione residuo è stato sottoposto al validatore del parent. Quando il
layout è accettato, tutti i valori del catalogo parent sono stati scritti uno
alla volta sul campione clone e l’integrità è stata ricontrollata dopo ogni
scrittura.

### Cloni con catalogo specifico

La verifica diretta dei Service Menu ha mostrato che la compatibilità puramente
strutturale col parent non bastava a descriverne la semantica. Gli otto set elencati usano
ora un template e un catalogo specifici.

| Clone | Parent | Particolarità | Valori verificati |
|---|---|---|---:|
| `daytona93` | `daytona` | Menu ridotto a 4 voci; Cabinet espone solo Deluxe/Upright | 11 valori specifici |
| `daytonas` | `daytona` | Cabinet a 4 valori e nuova voce Promote Saturn | 32 valori specifici |
| `dyndeka2` | `dynamcop` | HP Password è informativo e non selezionabile | 12 valori specifici |
| `dyndeka2b` | `dynamcop` | HP Password è informativo e non selezionabile | 12 valori specifici |
| `motoraiddx` | `motoraid` | Engine Volume è informativo; Cabinet Type espone Deluxe/Twin | 24 valori specifici |
| `stcca` | `stcc` | Country usa il layout clone; Default View è assente | 32 valori specifici |
| `stccb` | `stcc` | Country usa due byte spostati e un flag regionale aggiuntivo | 34 valori specifici |
| `stcco` | `stcc` | Country usa il layout clone; Default View è assente | 32 valori specifici |

`Promote Saturn` è un parametro autonomo con il ciclo `Coming Soon`,
`Available Now`, `Off`. `daytona93` non espone Link ID, Car Number, Game Mode o
Rival Arrow. I tre cloni STCC usano la codifica Country specifica; soltanto
`stccb` conserva anche Default View. Le mappe complete e le prove sono in
`data/diagnostic-menus/daytona93.yaml`, `daytonas.yaml`, `stcca.yaml`,
`stccb.yaml` e `stcco.yaml`.

### Layout parent compatibile, differenze esterne ai campi noti

Per questi otto cloni il campione dedicato conserva dati specifici del set,
mentre tutti i parametri del parent possono essere applicati senza adattare
layout o algoritmo d’integrità. Hanno seguito lo stesso percorso già verificato
sui primi cinque cloni: tutti i
template sono integrati e provati con due avvii consecutivi.

| Famiglia parent | Cloni | Valori parent verificati per clone |
|---|---|---:|
| `doa` | `doaa`, `doaab`, `doaae`, `doab` | 21/21 |
| `dynamcop` | `dynamcopb`, `dynamcopc` | 12/12 |
| `manxtt` | `manxttdx` | 29/29 |
| `vf2` | `vf2b` | 19/19 |

### Layout differente con struttura già individuata o scarto ridotto

Questi tre campioni vengono rifiutati dal validatore parent e non possono
ancora riceverne le opzioni. Per `indy500d` è già nota la diversa dimensione
della banca; per le due revisioni VF2 lo scarto è limitato a tre byte. Esiste
quindi un punto di partenza concreto per un adattatore specifico.

| Clone | Parent | Evidenza principale |
|---|---|---|
| `indy500d` | `indy500` | Banca EEPROM specchiata da 44 byte invece di 36 e dati specifici Deluxe |
| `vf2a` | `vf2` | Tre byte differenti, inclusi lunghezza/intestazione del blocco nominato |
| `vf2o` | `vf2` | Tre byte differenti, inclusi lunghezza/intestazione del blocco nominato |

### Layout differente con divergenze estese

Questi tre casi richiedono l’analisi completa di struttura, integrità e offset
prima di esporre le opzioni parent.

| Clone | Parent | Byte differenti backup RAM / EEPROM |
|---|---|---:|
| `hotdp` | `hotd` | 239 / 62 |
| `srallycdxa` | `srallyc` | 5548 / 22 |
| `vstrikero` | `vstriker` | 138 / 0 |

Il riepilogo operativo finale è quindi: **42/48 cloni supportati dal setup
automatico** — 21 tramite template parent byte-identico e 21 tramite template
dedicato — e **6 cloni che richiedono ancora un adattamento specifico del
layout o dell’integrità**. Sui 21 template dedicati sono stati verificati 494
valori NVRAM e 42 avvii RetroArch complessivi, due per clone, senza errori di
caricamento o persistenza.

## VF2 Revision A e Original — 18 settembre 2026

`vf2a` e `vf2o` riutilizzano le otto opzioni VF2 con cataloghi e campioni
iniziali dedicati. Il blocco dei parametri a 0x3340 (29 byte), la codifica e il
CRC-16/CCITT a 0x3302 sono identici al parent; i default nativi dei parametri
coincidono, salvo Country=Export applicato secondo la regola concordata.
Gli header conservano rispettivamente 0x13 e 0x12 a 0x3306. Il campo a 0x3318,
pur diverso nel campione base, varia durante la partita e non è una firma
fissa. I controlli automatici coprono ogni valore, CRC, conservazione dei byte
specifici e import/export del contenitore SRAM.

La copertura sale a **44/48 cloni** (23 template dedicati e 21 ereditati).
`hotdp` è un prototipo ed è escluso dall'implementazione NVRAM Settings e setup
dedicato. Restano **tre cloni da analizzare**: `indy500d`, `srallycdxa`,
`vstrikero`.

Prove reali del gruppo VF2: quattro avvii RetroArch macOS, due per clone,
con Country=USA, Drink=NG, Difficulty=Hardest e Display Type=C.R.T.
Entrambi raggiungono una partita; il secondo avvio, con NVRAM Settings
Disabled, conserva esattamente il blocco dei parametri e un CRC valido senza
riapplicare setup o opzioni. Log, screenshot e confronto SRAM sono archiviati
in `build-libretro-gpu/validation/vf2-revisions-20260918/summary.json`.

## Indy 500 Deluxe — 18 settembre 2026

Il confronto con gli screenshot dell'ultima campagna mostra otto voci nel
Service Menu, contro dieci del parent: Engine Volume e Default View sono
assenti. Le sette opzioni selezionate mantengono la codifica parent, con mappa
specifica Deluxe: Difficulty 0x19, Race Mode 0x18, Advertise Sound 0x27,
Country 0x16, Cabinet Type 0x17, Network Type 0x28 e Cabinet ID 0x29.
La copia speculare è a +44 byte. CRC Sega init 0xdebdec00, senza byte finale,
sui 42 byte dopo il checksum di ciascuna banca. Sono stati acquisiti il
baseline e sette campioni con una variazione ciascuno. I byte interni estranei
alle opzioni non vengono sovrascritti usando la mappa parent.

Il template conserva Cabinet Type=Deluxe e gli altri default nativi; il setup
iniziale applica Country=Export e Network Type=Stand Alone. I test automatici
verificano tutti i valori delle sette opzioni, l'integrità e il rifiuto della
banca parent da 36 byte. Copertura aggiornata: **45/48 cloni**, con 24 template
dedicati e 21 ereditati. `hotdp` resta escluso perché prototipo; restano da
analizzare `srallycdxa` e `vstrikero`.

Le due prove RetroArch macOS con Difficulty=Hard, Race Mode=Long, Advertise
Sound=On, Country=USA, Cabinet=Deluxe, Network=Stand Alone e Cabinet ID=2
sono terminate correttamente. Il secondo avvio con NVRAM Settings Disabled
conserva i sette valori, CRC e mirror; le EEPROM risultano byte-identiche.
Il gioco raggiunge l'attract mode con grafica regolare. La prova copre avvio
e persistenza, non il comportamento in gara di ciascuna opzione.
Evidenze in `build-libretro-gpu/validation/indy500d-layout-20260918/summary.json`.

## Virtua Striker (older) — 19 settembre 2026

Il Service Menu del clone contiene dieci voci modificabili e non espone
`ONE MATCH MODE`, presente nel parent. Il confronto isolato ha confermato nove
opzioni selezionate: Advertise Sound, Country, Monitor, Difficulty, V Goal
System, V Goal Time Set, PK System, PK Member Set e Billboard. `TIME SET` resta
al default nativo 2:00 e non viene esposto come Core Option.

Le codifiche e gli offset coincidono con il parent, eccetto Advertise Sound,
che usa 0x17/0x97 anziché 0x19/0x99. Il formato conserva due banche identiche da
128 byte; 0x08-0x09 resta il marcatore fisso `0a 00` e non è un CRC variabile.
Il setup mantiene i default nativi del clone, applicando soltanto la regola
concordata Country=Export. La precedente lettura di `ONE MATCH MODE: On` ottenuta
applicando la mappa parent era quindi un falso positivo, risolto osservando il
menu del clone.

Copertura aggiornata: **46/48 cloni**, con 25 template dedicati e 21 ereditati.
`hotdp` resta escluso perché prototipo; soltanto `srallycdxa` richiede ancora
analisi.

## Sega Rally Championship Deluxe revision A — 19 settembre 2026

Il Service Menu contiene quattro sole voci modificabili: Advertise Sound,
Country, Game Difficulty e Game Mode. Cabinet Type e Link Type del parent non
sono presenti. La campagna completa di 14 campioni conferma per le quattro voci
gli stessi offset e codici del parent: 0x08, 0x09, 0x0c e 0x0d.

Il formato del clone è però specifico: EEPROM dichiarata da 0x2c byte e
CRC-16/CCITT con iniziale 0xffff e risultato invertito sui 0x2a byte da 0x02.
La backup RAM differisce sostanzialmente dal parent e viene quindi conservata in
un template dedicato. Il setup applica Country=Export e mantiene i default nativi
restanti; i byte 0x0a e 0x0b delle voci assenti non vengono modificati.

Due avvii RetroArch reali hanno verificato setup, avvio in gara e persistenza di
Advertise Sound=Off, Country=USA, Difficulty=Hardest e Game Mode=Longest. Il
secondo avvio ha caricato lo stesso `.srm` con NVRAM Settings disabilitato senza
riapplicare le opzioni. Copertura a questo punto della campagna: **47/48 cloni**, con 26 template
dedicati e 21 ereditati. Restava il solo `hotdp`.

## The House of the Dead prototype — 19 settembre 2026

Il prototipo usa un layout proprio: due banche EEPROM speculari da 24 byte a
0x08 e 0x20, protette dal CRC Sega con iniziale 0xdebdeb00 e byte zero finale.
La campagna ha acquisito 29 combinazioni distinte e due campioni aggiuntivi per
confermare il ritorno ciclico di Blood Color, che nel prototipo offre soltanto
Red e Green.

Sono esposte le quattro voci concordate: Game Difficulty, Blood Color,
Advertise Sound e Country. Life Setting, Gun Blowback e Cabinet Type sono
documentate ma non diventano Core Options. Il template conserva i default
nativi del prototipo e il setup automatico applica soltanto la regola generale
Country=Export. Due avvii RetroArch hanno verificato un set completo di valori e
la successiva persistenza byte-identica con Initial NVRAM Setup e NVRAM Settings
disabilitati. La copertura della campagna è quindi **48/48 cloni**, con 27
template dedicati e 21 ereditati.

## Campagne riproducibili

Le campagne dei 27 cloni con template dedicato sono ora conservate in
`scripts/libretro_nvram_samples.<set>.toml`; catalogo, uso e limiti della
verifica sono in `scripts/NVRAM_CAMPAIGNS.md`. La tabella `[core_options]`
disattiva esplicitamente i setup del core durante queste acquisizioni native.
