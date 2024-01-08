#include "prodcons.h"
#define PC_buffer_len 10

// Funzione per estrarre la stringa dal buffer
char *get(buffer *b) {
  char *s = NULL;
  xpthread_mutex_lock(&b->pmutex_buf, QUI);
  // Attende fino a quando il buffer è vuoto
  while (b->i == 0)
    xpthread_cond_wait(&b->cond_data_items, &b->pmutex_buf, QUI); // Attende finchè il buffer è pieno
  // Verifica se il buffer è stato segnato come vuoto
  if (b->i == -1)
  {
    xpthread_mutex_unlock(&(b->pmutex_buf), QUI);
    return NULL;  // Buffer segnato come vuoto
  }
  // Accesso al buffer
  s = b->buf[b->ri]; // Preleva la stringa dal buffer
  b->ri = (b->ri + 1) % PC_buffer_len;  // Aggiorna l'indice di lettura
  b->i -= 1;  // Aggiorna il numero di elementi nel buffer
  xpthread_cond_signal(&b->cond_free_slots, QUI); // Segnala ai threads produttori.
  xpthread_mutex_unlock(&b->pmutex_buf, QUI);
  return s;
}

// Funzione per inserire la stringa nel buffer
int put(buffer *b, char *s) {
  xpthread_mutex_lock(&b->pmutex_buf, QUI);
  while (b->i == PC_buffer_len)
    xpthread_cond_wait(&b->cond_free_slots, &b->pmutex_buf, QUI); // Attende finchè il buffer è pieno
  b->buf[b->wi] = s;         // Scrive la stringa nel buffer
  b->wi = (b->wi + 1) % PC_buffer_len;  // Aggiorna l'indice di scrittura
  b->i += 1;
  xpthread_cond_signal(&b->cond_data_items, QUI); // Segnala ai threads consumatori.
  xpthread_mutex_unlock(&b->pmutex_buf, QUI);
  return 0;
}

// Distrugge il buffer circolare
int buffer_destroy(buffer *b) {
  // Distruggo i semafori e mutex
  xpthread_cond_destroy(&b->cond_free_slots, QUI);
  xpthread_cond_destroy(&b->cond_data_items, QUI);
  xpthread_mutex_destroy(&b->pmutex_buf, QUI);
  return 0;
}
