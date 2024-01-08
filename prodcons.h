#ifndef PRODCONS_H
#define PRODCONS_H
#include "xerrori.h"

typedef struct {
  char **buf;
  int i; // Variabile che indica gli elementi nel buffer.
  int wi;  // Indice di scrittura nel buffer.
  int ri;  // Indice di lettura nel buffer.
  pthread_mutex_t pmutex_buf; // Mutex per ottenere accesso esclusivo al buffer.
  pthread_cond_t cond_free_slots; // Condition variable per segnalare ai threads produttori che c'è posto libero nel buffer.
  pthread_cond_t
      cond_data_items; // Condition variable per segnalare ai threads consumatori che ci sono dati da consumare.
} buffer;

char *get(buffer *b);
int put(buffer *b, char *s);
int buffer_destroy(buffer *b);

#endif // PRODCONS_H
