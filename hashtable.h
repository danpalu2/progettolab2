#ifndef HASH_TABLE
#define HASH_TABLE
#include "xerrori.h"

typedef struct {
  int i;
  int num_entry;        // Numero di stringhe distinte caricate nell'hash table
  int num_readers;      // Numero di lettori attivi
  int num_writers;      // Numero di scrittori attivi
  pthread_cond_t cond_r; // Condition variable per lettori
  pthread_cond_t cond_w; // Condition variable per scrittori
  pthread_mutex_t hash_lock; // Mutex per accesso alla tabella
} hash_table;

typedef struct {
  int valore;  // numero di occorrenze della stringa
  ENTRY *next; // puntatore al prossimo elemento
} coppia;

void createTable(size_t dim);
ENTRY *crea_entry(char *s, int n);
void aggiungi(char *s);
int conta(char *s);
int size();
void destroy();
void read_lock(hash_table *x);
void read_unlock(hash_table *x);
void write_lock(hash_table *x);
void write_unlock(hash_table *x);

#endif // HASH_TABLE
