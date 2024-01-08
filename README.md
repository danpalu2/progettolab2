# Progetto di Laboratorio 2

Il progetto consiste nella realizzazione di un programma multithreading chiamato archivio.c che gestisce la memorizzazione di stringhe in una tabella hash. Il programma archivio viene lanciato da un server, alla quale si collegano due client. I due client si collegano al server tramite socket, e inviano al server le righe di un file di testo. Il server scrive tali righe in due pipe distinte (a seconda di quale client sta inviando i dati). Infine, il server lancia il programma archivo. Il programma archivio è composto da:

- Thread gestore dei segnali;
- Thread capo scrittore;
- Thread capo lettore;
- Thread scrittori;
- Thread lettori.

I thread capi, leggono i dati dalle rispettive pipe e scrivono i dati su un buffer circolare condiviso al fine di passare i dati ai propri thread scrittori/lettori. I thread scrittori non fanno altro che scrivere sulla tabella hash, mentre i thread lettori contano le occorrenze di determinate parole all'interno della tabella. Il progetto contiene i tre file di prova (file1, file2, file3) e la sua esecuzione genera altri tre file:

- File `lettori.log` cotenente le parole cercate all'interno della tabella e il numero di occorrenze.
- File `server.log` cotenente il tipo di connessione e il numero totale di byte scritti nelle pipe.
- File `valgrind-XXX.log` cotenente i dati prodotti dall'esecuzione di valgrind sul programma.

## Server.py

Il file è scritto in python, comunica con i due client e avvia archivio. Il server è incaricato di creare le pipe `capolet` e `caposc` se non sono già esistenti e tutto ciò che avviene nel server viene scritto sul file `server.log`. Per essere avviato richiede dei parametri tramite il modulo argparse, attraverso il quale è possibile anche chiedere aiuto eseguendo il comando:

```shell
./server.py -h
```

Il server è in ascolto continuamente e quando avviene la connessione con uno dei due client, per prima cosa capisce il tipo di connessione tramite la lettura di un carattere inviato dal client. A seconda del tipo di connessione scriverà sulla pipe esatta aperta in precedenza. 

La funzione per scrivere all'interno delle pipe è `gestisci_connessione(conn, fd_pipe)`. In un ciclo continuo il server richiedrà prima la lunghezza della sequenza codificabile in 2 byte e nei cicli successivi scriverà sul file di log i dati relativi la connessione. In presenza di una connessione di tipo A, il server al secondo ciclo scriverà il numero di byte scritti nella pipe `capolet`, mentre in presenza di una connessione di tipo B, il server si chiederà prima se la lunghezza della sequenza ricevuta è uguale a 0 e successivamente scriverà il numero totale di byte scritti nella pipe `caposc` e invierà al client di tipo B il numero di sequenze ricevute. Ad ogni ciclo viene effettuata la scrittura sulla pipe alla quale accediamo in mutua esclusione e dove scriviamo prima la lunghezza in byte e poi la sequenza ricevuta.

Il server rimane in attesa di connessioni finchè non riceverà una Keyboard interrupt, generata dal segnale SIGINT. In questo caso le pipe vengono chiuse e rimosse dalla memoria. Infine, viene inviato il segnale SIGTERM al programma archivio.

## Client1.c

Il file è scritto in C, accetta sulla linea di comando il nome di un file di testo e invia al server, una alla volta, le linee del file di testo con una connessione di tipo A per ogni liena. Il client dopo aver aperto il `file1` in lettura legge da esso tramite la `getline` in un ciclo while e chiama la funzione `connetti(buffer, seq_length)`. La funzione chiamata non fa altro che collegarsi al server e inviare in ordine:

- Tipo di connessione;
- Lunghezza della sequenza;
- Sequenza letta dal file.

## Client2.c

Il file è scritto in python, accetta sulla linea di comando il nome di uno o più file di testo. Per ogni file di testo passato sulla linea di comando crea un thread che si collega al server e invia una alla volta le linee del file con una connessione di tipo B (una per ogni thread creato). Il file accetta gli argomenti tramite il modulo argparse. Ogni thread creato tiene traccia del numero di sequenze inviate al server e apre il file in lettura. Viene attivata una singola connessione per file dove vengono inviate tutte le linee di testo. Anche il client2 invia al server i dati nell'ordine:

- Tipo di connessione;
- Lunghezza della sequenza;
- Sequenza letta dal file.

Una volta terminato di inviare le linee del file, riceve dal server, il numero di sequenze ricevute e lo confronta con il numero di sequenze inviate.

## Archivio.c

Il file è scritto in C, riceve sulla linea di comando due interi che indicano il numero w di thread scrittori (che eseguono solo l'operazione aggiungi), e il numero r di thread lettori (che eseguono solo l'operazione conta). E' composto da due funzioni:

- `signal_handler`;
- `main`.

La funzione `signal_handler` è il copro del thread gestore dei segnali. Dopo aver settato la maschera dei segnali il thread entra in un ciclo continuo dove attende l'arrivo di un segnale per poterlo gestire. In particolare il thread deve gestire tre segnali:

- Segnale SIGINT: Stampa su stderr il numero di stringhe distinte contenuto all'interno della tabella hash;
- Segnale SIGSUR1: Distrugge e ricrea la tabella hash, azzerandone il contenuto e liberando la memoria.
- Segnale SIGTERM: Il thread blocca la gestione di altri segnali, aspetta la terminazione dei threads capo lettore e capo scrittore, stampa su stdout il numero di stringhe distinte contenuto nella tabella, distrugge quest'ultima e termina.

All'interno del `main` chiama la funzione per la creazione della tabella hash contenuta nel file `hashtable.c`. Successivamente crea e apre il file lettori.log, setta la maschera dei segnali e crea il thread gestore dei segnali. Crea i buffer dei lettori e degli scrittori e inizializza le condition variables e i mutex necessari alla struct `buffer` presente nel file `prodcons.h`. Crea l'oggetto di tipo `buffer` per il capo lettore, l'oggetto di tipo `master` (presente in `threads.h`) che indica il thread capo lettore e assegna ai suoi campi i dati necessari all'inizializzazione. Successivamente crea l'oggetto di tipo `thread_r`, indicante i thread lettori, la quale inizializzazione dei campi avviene all'interno di un ciclo for (un ciclo per il numero dei thread lettori passato da riga comando). Le stesse operazioni valgono anche per il thread capo scrittore e per i thread scrittori. Una volta terminato il thread gestore dei segnali, il processo attende anche la terminazione di tutti i thread scrittori e lettori, chiude il file e dealloca la memoria necessaria ai buffer lettore e scrittore. Infine, termina il programma.

## Threads.c

Il file è scritto in C e contiene tre funzioni per l'esecuzione dei threads capo scrittore/lettore, lettori e scrittori. Ogni thread fa riferimento ad una propria struct (contenuta in `threads.h`):

- `master` per i threads capi;
- `thread_w` per i threads scrittori
- `thread_r` per i threads lettori

La funzione `leggi` (eseguita da ogni thread lettore) non fa altro che leggere dal buffer condiviso le stringhe, richiamare la funzione `conta` per leggere il numero di occorrenze delle stringhe lette presenti nella tabella hash. Successivamente accede in mutua esclusione sul file `lettori.log` e scrive il risultato. La lettura dal buffer è fatta in un ciclo while e andrà avanti finchè la stringa letta sarà diversa da `NULL`.

La funzione `scrivi` (eseguita da ogni thread scrittore) legge le strighe dal buffer condiviso e per ognuna di esse richiama la funzione `aggiungi` per scriverle all'interno della tabella hash. La lettura dal buffer è fatta in un ciclo while e andrà avanti finchè la stringa letta sarà diversa da `NULL`.

La funzione `master_threads` viene eseguita dai threads capo scrittore e capo lettore. I due thread apriranno in lettura rispettivamente la pipe `caposc` e la pipe `capolet`. Successivamente entreranno all'interno del ciclo di lettura dalla rispettiva pipe. Leggeranno prima la dimensione della sequenza e successivamente la sequenza stessa. All'interno del ciclo di lettura effettueranno la tokenizzazione della sequenza tramite l'utilizzo della funzione `strtok_r`, rientrante e thread safety rispetto a `strtok`. I token creati verranno salvati all'interno di un array di supporto per la deallocazione della memoria e successivamente scritti all'interno dei rispettivi buffer tramite la funzione `put`. I thread capo lettore e capo scrittore rimmarranno in attesa sul comando di lettura della pipe finchè queste non vengono chiuse in scrittura. Successivamente segnaleranno la terminazione ai rispettivi thread scrittori e lettori effettuando una broadcast sulla condition variable dove i thread sono in attesa. Infine, i thread capi chiudono le pipe e terminano.

## Prodcons.c

Il file è scritto in C e contiene l'implementazione del buffer, circolare e di dimensione limitata, e le operazioni su quest'ultimo. Lo schema di accesso al buffer è realizato tramite l'utilizzo di mutex e condition variables.  Il file è composto da tre funzioni:

- La funzione `get`: Viene chiamata dai thread lettori e scrittori e permette di leggere le stringhe dal buffer. Si efettua inizialmente una lock per la regione critica, successivamente in un ciclo while ci si mette in attesa su una condition variable finchè il numero di elementi nel buffer è uguale a 0 e quindi si rilascia la lock. Se sono presenti stringhe all'interno del buffer, la stringa verrà preleveta e verrà effettuata una signal sulla condition variable che segnala la possibilità di scrivere altre stringhe all'interno del buffer. Se il numero di elementi presenti nel buffer è uguale a -1, significa che i thread lettori devono terminare e quindi verrà restituito `NULL`
-  La funzione `put`: Viene chiamata dai thread capo lettore e capo scrittore e permette di scrivere le stringhe all'interno del buffer. Si efettua inizialmente una lock per la regione critica, successivamente in un ciclo while ci si mette in attesa su una condition variable finchè il numero di elementi nel buffer è uguale alla massima capacità e quindi si rilascia la lock. Se c'è posto libero all'interno del buffer, la stringa verrà scritta e verrà effettuata una signal sulla condition variable che segnala la possibilità di leggere dal buffer.
-  La funzione `buffer_destroy`: Viene chiamata dal programma `archivio` e consente la distruzione delle condition variables e dei mutex utilizzati dai buffer al fine di liberare la memoria.

## Hashtable.c

Il file è scritto in C e contiene l'implementazione della tabella hash e le funzioni per la creazione e la distruzione della stessa, e per le operazioni `aggiungi` e `conta`. Il file utilizza un oggetto di tipo hash_table la cui struct è contenuta nel file `hash_table.h`, e una variabile globale chiamata `linked_list` utilizzata per la deallocazione degli elementi all'interno della tabella. L'accesso alla tabella è implementato secondo lo schema che favorisce gli scrittori utilizzando delle condition variables. Il file è composto da quattro funzioni:

- `createTable`: Inizilizza i mutex e le condition variables della struct `hash_table` oltrechè al numero di elementi della tabella, numero di lettori e scrittori attivi. La funzione viene chiamata dal programma `archivio`.
- `aggiungi`: La funzione viene chiamata dai thread scrittori. Inzialmente si acquisisce la lock sulla sezione critica e all'interno di un ciclo while si aspetta finchè il numero di scrittori attivi è maggiore di 0, l'attesa viene fatta su una condition variable e viene rilasciata la lock. Se non ci sono scrittori attivi si entra nella sezione critica e si effettua l'operazione di ricerca sulla tabella tramite la funzione `hsearch` e passandole la modalità `FIND`. Se la ricerca avrà successo, la funzione `aggiungi` non farà altro che incrementare il numero di occorrenze dell'elemento all'interno della tabella. Se la ricerca invece fallisce, verrà richiamata nuovamente la funzione `hsearch` ma passandole la modalità `ENTER`. Alla fine verrà effettuata una signal per svegliare eventuali scrittori in attesa e successivamente una broadcast per i lettori in attesa.
- `conta`: La funzione viene chiamata dai thread lettori. Inzialmente si acquisisce la lock sulla sezione critica e all'interno di un ciclo while si aspetta finchè il numero di scrittori attivi è maggiore di 0, l'attesa viene fatta su una condition variable e viene rilasciata la lock. Se non ci sono scrittori attivi si entra nella sezione critica e si effettua l'operazione di ricerca sulla tabella tramite la funzione `hsearch` e passandole la modalità `FIND`. Se la ricerca avrà successo, verrà restituito il numero di occerrenze dell'elemento all'interno della tabella hash.  Alla fine verrà effettuata una signal per svegliare eventuali scrittori in attesa.
- `destroy`: La funzione viene chiamata dal thread gestore dei segnali e permette di liberare la memoria, deallocando tutti gli elementi presenti all'interno della tabella e distruggendo mutex e condition variables relativi alla struct `hash_table`.
