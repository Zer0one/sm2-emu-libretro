# Campagne NVRAM dei cloni

I 27 cloni validati con template dedicato hanno una campagna permanente
`libretro_nvram_samples.<set>.toml`. I risultati restano nel workspace sotto
`build-libretro-gpu/validation/nvram-campaigns`; ROM e risultati non vanno in Git.

La tabella TOML opzionale `[core_options]` permette di disattivare esplicitamente
Automatic Initial NVRAM Setup e NVRAM Settings, conservando i default nativi
nelle acquisizioni. Le campagne precedenti senza tale tabella mantengono il
comportamento originario. Ogni variazione riparte dal baseline del clone.
I suffissi `step-N` indicano N pressioni dal default nativo, senza presumerne
il valore. `cycle-NNN` identifica una ricetta della campagna parent: anche qui
il nome non promette il valore del parent. Le campagne includono i parametri
selezionabili del menu, anche se non sono esposti come Core Options.

## Uso

```sh
python3 scripts/libretro_nvram_samples.py scripts/libretro_nvram_samples.indy500d.toml --dry-run
python3 scripts/libretro_nvram_samples.py scripts/libretro_nvram_samples.indy500d.toml
```

Il catalogo ricavato dagli screenshot e i 51 YAML diagnostici si controllano
con:

```sh
ruby scripts/audit-diagnostic-yaml-catalog.rb
```

Il controllo confronta righe visibili, ordine, default dichiarati e valori
acquisiti. Offset, codifiche e algoritmi d’integrità restano responsabilità dei
singoli YAML e dei controlli Save RAM.

## Sequenze e verifica

Tutte le 27 campagne hanno superato il controllo preliminare del generatore;
è stata verificata anche la disattivazione esplicita dei setup del core.
Non è stata ripetuta l'estrazione: le sequenze derivano dai menu e dalle
acquisizioni archiviati. I cicli completi dei nuovi file non sono tutti stati
rieseguiti dopo il consolidamento.

I binding sono R3=Test e L3=Service; Daytona usa START per selezionare.
HP Password ed Engine Volume Out of Use vengono saltati. VF2 attraversa
INITIALIZE senza selezionarlo. Le campagne terminano uscendo dai menu.
`motoraiddx` conserva l'attesa prudenziale per il prompt Driveboard Error /
Press Test e il doppio Test. In modalità interattiva attendere il prompt
prima delle due pressioni.

## Catalogo

| Set | Campioni configurati |
|---|---:|
| `daytona93` | 12 |
| `daytonas` | 33 |
| `daytonase` | 29 |
| `doaa` | 41 |
| `doaab` | 41 |
| `doaae` | 41 |
| `doab` | 41 |
| `dynamcopb` | 46 |
| `dynamcopc` | 46 |
| `dyndeka2` | 46 |
| `dyndeka2b` | 46 |
| `hotdp` | 29 |
| `indy500d` | 28 |
| `indy500to` | 35 |
| `manxttc` | 60 |
| `manxttdx` | 60 |
| `motoraiddx` | 25 |
| `sfight` | 60 |
| `srallycdx` | 22 |
| `srallycdxa` | 14 |
| `stcca` | 37 |
| `stccb` | 39 |
| `stcco` | 37 |
| `vf2a` | 30 |
| `vf2b` | 30 |
| `vf2o` | 30 |
| `vstrikero` | 11 |

Tutti i cloni dispongono ora di una campagna dedicata o di un’eredità parent
verificata. Per `hotdp` i 29 campioni permanenti coprono i valori distinti; due
acquisizioni aggiuntive archiviate confermano il ritorno ciclico Red/Green di
Blood Color.

## Acquisizioni mirate per il catalogo Game Settings

Le seguenti campagne brevi completano la struttura dei menu senza duplicare i
cicli dei valori già acquisiti per i parent:

| Configurazione | Scopo |
|---|---|
| `libretro_nvram_samples.overrevb.toml` | Foundational Setting e Optional Game Setting del clone |
| `libretro_nvram_samples.overrevba.toml` | Foundational Setting e Optional Game Setting del clone |
| `libretro_nvram_samples.vonr.toml` | Test Menu e Game Assignments dopo il timeout della rete Relay |
| `libretro_nvram_samples.rascot2.toml` | Diagnostica SegaNet, avvio locale con Button 1 e ritorno della diagnostica con Test |

I risultati verificati sono archiviati sotto
`build-libretro-gpu/validation/game-settings-gap-fill-20260920`. `vonr` richiede
che il comando Test venga inviato soltanto dopo l’inizializzazione Relay;
`rascot2` non espone un normale menu Game Settings nel percorso acquisito.
