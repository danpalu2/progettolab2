#ifndef THREADS_H
#define THREADS_H

#include "hashtable.h" // Permette ai threads di chiamare funzioni di hashing.
#include "prodcons.h" // Permette ai threads di aver accesso al buffer condiviso.

typedef struct {
  buffer *buf;  // Buffer di accesso ai thread scrittori
} thread_w;

typedef struct {
  buffer *buf; // Buffer di accesso ai thread lettori
  pthread_mutex_t *f_mutex; // Mutex per accedere al file lettori.log
  FILE *f;
} thread_r;

// Struttura dati per i capi
typedef struct {
  buffer buf;
  int fd_pipe; // File descriptor che indica la pipe dalla quale leggere i dati
int t; // Numero thread lettori o scrittori
} master;

void *leggi(void *arg);
void *scrivi(void *arg);
void *master_threads(void *arg);

#endif // THREADS_H
