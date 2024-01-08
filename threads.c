#define Max_sequence_length 2048
#include "threads.h"
#include "xerrori.h"

// Funzione eseguita dai thread lettori
void *leggi(void *arg) {
  thread_r *r = (thread_r *)arg; // Crea un oggetto di tipo thread_r
  char *s;
  int tot;
  // Ciclo di lettura dal buffer condiviso
  while ((s = get(r->buf)) != NULL) {
    tot = conta(s); // Richiama la funzione conta() presente in hash_table.c
    // Scrittura su lettori.log
    xpthread_mutex_lock(r->f_mutex, QUI); // Accesso in mutua esclusione al file
    fprintf(r->f, "%s %d\n", s, tot);
    xpthread_mutex_unlock(r->f_mutex, QUI);
  }
  pthread_exit(NULL); // Terminazione del thread lettore
}

// Funzione eseguita dai thread scrittori
void *scrivi(void *arg) {
  thread_w *w = (thread_w *)arg; //// Crea un oggetto di tipo thread_w
  char *s;
  // Lettura della stringa dal buffer condiviso
  while ((s = get(w->buf)) != NULL) {
    aggiungi(s); // Richiama la funzione aggiungi() presente in hash_table.c
  }
  pthread_exit(NULL); // Terminazione del thread scrittore
}

// Funzione per il thread capo lettore.
void *master_threads(void *arg) {
  master *m = (master *)arg;
  // Apertura delle pipe in lettura
  int fd;
  if (m->fd_pipe == 0) {
    // Apertura delle named pipe in lettura
    fd = open("capolet", O_RDONLY);
    if (fd < 0)
      termina("Error opening named pipe");
  } else {
    fd = open("caposc", O_RDONLY);
    if (fd < 0)
      termina("Error opening named pipe");
  }
  char readb[Max_sequence_length];
  int e;
  char *save;
  int count = 0;
  char *tempo[100000];
  // Ciclo di lettura dalla pipe
  while ((e = read(fd, readb, sizeof(uint16_t))) > 0) {
    if (e < (int)sizeof(uint16_t))
      break;
    u_int16_t size;
    memcpy(&size, readb, sizeof(uint16_t));
    size = ntohs(size);
    if (size > Max_sequence_length)
      break;
    e = read(fd, readb, size);
    if (e <= 0)
      break;
    // Tokenizzazione stringa
    readb[size] = '\0';
    char *tmp;
    char *token = strtok_r(readb, ".,:; \n\r\t", &save);
    while (token != NULL) {
      // Accesso al buffer
      tmp = strdup(token);
      tempo[count] = tmp;
      count++;
      put(&m->buf, tmp);
      token = strtok_r(NULL, ".,:; \n\r\t", &save);
    }
  }
  // Svuoto la memoria
  for (int i = 0; i < count; i++) {
    free(tempo[i]);
  }
  if (e == -1) xtermina("Error reading pipe", QUI);
  // Segnalo ai thread che devono terminare
  xpthread_mutex_lock(&(m->buf.pmutex_buf), QUI);
  for (int i = 0; i < m->t; i++) { 
    m->buf.i = -1;
  } 
  xpthread_cond_broadcast(&(m->buf.cond_data_items), QUI);
  xpthread_mutex_unlock(&(m->buf.pmutex_buf), QUI);

  // Chiusura della pipe
  xclose(fd, QUI);
  pthread_exit(NULL);
}
