#include "hashtable.h" // Permette di usare le funzioni che operano sulla tabella hash.
#include "xerrori.h" // Permette di usare le funzioni per la gestione degli errori.
#include "prodcons.h" // Permette di usare le funzioni per la gestione del buffer.
#include "threads.h" // Permette di usare le funzioni dei threads lettori e scrittori.
#define Num_elem 1000000
#define PC_buffer_len 10

// Puntatori ai thread capo scrittore e capo lettore
pthread_t cpWrite;
pthread_t cpRead;

// Programma per il thread gestore dei segnali
void *signal_handler(void *arg) {
  // Si mette in attesa di tutti i segnali
  sigset_t mask;
  sigfillset(&mask);
  int s;
  // Attende l'arrivo di un segnale
  while (true) {
    int e = sigwait(&mask, &s);
    if (e != 0)
      xtermina("Sigwait error", QUI);
    printf("Handler thread woken up by signal %d\n", s);
    // Gestione segnale SIGINT
    if (s == SIGINT) {
      fprintf(stderr, "Number of distinct strings stored in the table %d\n", size());
    }
    // Gestione segnale SIGSUR1
    if (s == SIGUSR1) {
      destroy();               // Distrugge la tabella hash
      createTable(Num_elem);  // Crea la tabella hash
    }
    // Gestione segnale SIGTERM
    if (s == SIGTERM) {
      pthread_sigmask(SIG_BLOCK, &mask,
                      NULL); // Blocca la gestione di altri segnali
      // Aspetto la fine dei thread capo scrittore e lettore
      xpthread_join(cpWrite, NULL, QUI);
      xpthread_join(cpRead, NULL, QUI);
      fprintf(stdout, "Number of distinct strings stored in the table %d\n", size());
      destroy();  // Distrugge la tabella hash
      pthread_exit(NULL); // Termina il programma
    }
  }
}

int main(int argc, char *argv[]) {
  // Controllo dei parametri
  if (argc < 5) {
    printf("Missing parameters");
    exit(1);
  }
  int num_readers = atoi(argv[2]);
  int num_writers = atoi(argv[4]);
  if (num_writers < 3 || num_readers < 3)
    xtermina("Error in the parameters", QUI);

  // Creo tabella hash
  createTable(Num_elem);

  //  Creo file lettori.log
  FILE *f = xfopen("lettori.log", "w+", QUI);

  // Creo maschera per i segnali
  sigset_t mask;
  sigfillset(&mask);
  pthread_sigmask(SIG_BLOCK, &mask, NULL);
  // Creo processo gestore dei segnali
  pthread_t signal_t;   
  xpthread_create(&signal_t, NULL, signal_handler, NULL, QUI);
  
  // Creo buffer lettori e scrittori
  char *buff_r[10];
  char *buff_w[10];
  // Inizializzo i buffer a NULL
  for (int i = 0; i < PC_buffer_len; i++) {
      buff_r[i] = NULL;
      buff_w[i] = NULL;
  }
  
  // Inizializzo semafori e mutex relativi a prodcons.h
  pthread_cond_t cond_free_slots1, cond_free_slots2;
  pthread_cond_t cond_data_items1, cond_data_items2;
  pthread_mutex_t pmutex_bufr;
  pthread_mutex_t pmutex_bufw;
  xpthread_cond_init(&cond_free_slots1, NULL, QUI);
  xpthread_cond_init(&cond_free_slots2, NULL, QUI);
  xpthread_cond_init(&cond_data_items1, NULL, QUI);
  xpthread_cond_init(&cond_data_items2, NULL, QUI);
  xpthread_mutex_init(&pmutex_bufr, NULL, QUI);
  xpthread_mutex_init(&pmutex_bufw, NULL, QUI);

  // Variabili di accesso al buffer per capo lettore
  buffer b_lettore;
  b_lettore.buf = buff_r;
  b_lettore.i = 0;
  b_lettore.ri = 0;
  b_lettore.wi = 0;
  b_lettore.cond_data_items = cond_data_items1;
  b_lettore.cond_free_slots = cond_free_slots1;
  b_lettore.pmutex_buf = pmutex_bufr;
  
  // Thread capo lettore
  master tc_lettore;
  tc_lettore.buf = b_lettore;
  tc_lettore.fd_pipe = 0;
  tc_lettore.t = num_readers;
  if ((xpthread_create(&cpRead, NULL, master_threads, &tc_lettore, QUI)) != 0)
    termina("pthread_create Reader failed");
  
  // Thread lettori
  thread_r reader_threads[num_readers];
  pthread_t t_r[num_readers];
  pthread_mutex_t f_mutex;
  xpthread_mutex_init(&f_mutex, NULL, QUI);
  // Creo tutti i lettori
  for (int i = 0; i < num_readers; i++) {
    reader_threads[i].buf = &tc_lettore.buf;
    reader_threads[i].f = f;
    reader_threads[i].f_mutex = &f_mutex;
    // Avvio l'iesimo thread lettore
    if ((xpthread_create(&t_r[i], NULL, leggi, &reader_threads[i], QUI)) != 0)
      termina("pthread_create 'leggi' failed");
  }

  // Variabili di accesso al buffer per capo scrittore
  buffer b_scrittore;
  b_scrittore.buf = buff_w;
  b_scrittore.i = 0;
  b_scrittore.ri = 0;
  b_scrittore.wi = 0;
  b_scrittore.cond_data_items = cond_data_items2;
  b_scrittore.cond_free_slots = cond_free_slots2;
  b_scrittore.pmutex_buf = pmutex_bufw;
  
  // Thread capo scrittore
  master tc_scrittore;
  tc_scrittore.buf = b_scrittore;
  tc_scrittore.fd_pipe = 1;
  tc_scrittore.t = num_writers;
  if ((xpthread_create(&cpWrite, NULL, master_threads, &tc_scrittore, QUI)) != 0)
    termina("pthread_create Reader failed");
  
  // Thread scrittori
  thread_w writer_threads[num_writers];
  pthread_t t_w[num_writers];
  // Creo tutti gli scrittori
  for (int i = 0; i < num_writers; i++) {
     writer_threads[i].buf = &tc_scrittore.buf;
    // Avvio l'iesimo thread scrittore
    if ((xpthread_create(&t_w[i], NULL, scrivi, &writer_threads[i], QUI)) != 0)
      termina("pthread_create 'scrivi' failed");
  }

  // Terminazione del thread gestore
  xpthread_join(signal_t, NULL, QUI);
  
  // Terminazione thread scrittori e lettori
  for (int i = 0; i < num_writers; i++) xpthread_join(t_w[i], NULL, QUI); 
  for (int i = 0; i < num_readers; i++) xpthread_join(t_r[i], NULL, QUI);

  // Chiusura file
  fclose(f);
  
  // Liberazione della memoria
  xpthread_mutex_destroy(&f_mutex, QUI);
  // Liberazione della memoria alla fine del programma
  buffer_destroy(&tc_lettore.buf);
  buffer_destroy(&tc_scrittore.buf);
  pthread_exit(NULL);
}
