# Assignment1_ARP
 First assignment for ARP at UniGe

- funzioni con Prima maiuscola
- cmake per costruire la roba
- snake case

# Davide
- Drone display
- Input

## parameter file
- scala input
- massa drone
- valori attrito
- altre costanti del drone


# Valentina
- Server
- Watchdog

# Simulazione di un esecuzione
- Premo la freccia destra
- Il gestore dell'input lo manda al drone
- Il drone calcola la forza e il resto e manda la posizione attuale al server
  - Notare che il drone in ogni caso manda aggiornamenti al server
- Il server prende queste info e fa vedere il risultato
# idea di come gestire l'update della mappa
Se si gestisce lo scambio di messaggi attraverso le pipe come nel week8 homework tutto torna meglio
Perche' tramite le pipe i processi segnalano quando e' pronta la roba da leggere e poi tramite
la shared memory avviene il vero e proprio scambio di messaggi. Cosi' siamo anche a mezzo del secondo
assignment.
