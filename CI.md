# Build automatiche Libretro

Il workflow `.github/workflows/libretro-ci.yml` compila da `main`, nelle pull
request e su avvio manuale quattro core: Linux x86_64, Windows x86_64,
macOS Apple Silicon e macOS Intel. Entrambi i renderer, software e Vulkan,
sono inclusi. Non servono ROM nei runner e nessuna ROM viene distribuita.

Ogni job esegue i controlli degli input e verifica caricamento della libreria,
25 funzioni ABI, dipendenze e tre cicli init/deinit senza contenuto. Questi
controlli non sostituiscono una prova con giochi e GPU reali.

Gli artifact privati restano disponibili per 14 giorni. Contengono il core,
`sm2_libretro.info`, `system/sm2-emu/games.xml`, licenze, revisione sorgente e
SHA256SUMS. Copiare il core nella directory core del frontend, il file `.info`
nella directory informazioni e `sm2-emu/games.xml` nella directory system.
Per una prova usare prima directory separate per configurazione e salvataggi.

Linux usa Ubuntu 24.04 (glibc 2.39) e incorpora i runtime GCC/C++: la
compatibilità con distribuzioni precedenti non è garantita. Windows usa MinGW64
con runtime statici. macOS richiede almeno macOS 13. La GPU richiede Vulkan 1.3
con le feature verificate dal core; su macOS vedere `GPU.md` nel repository
per la selezione di una versione compatibile di MoltenVK.

I profili completi dei controlli e i savestate non sono ancora implementati.
