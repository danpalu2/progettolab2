#include "hashtable.h"
#include "xerrori.h"

// Puntatore alla testa della lista
ENTRY *linked_list = NULL;

// Oggetto tipo hashtable
hash_table ht;

// Crea tabella hash
void createTable(size_t dim) {
    ht.i = hcreate(dim);
  // Inizializzo dati della struct hash_table 
    if (xpthread_mutex_init(&(ht.hash_lock), NULL, QUI) != 0)
        xtermina("Error during mutex initialization", QUI);
    if (xpthread_cond_init(&(ht.cond_r), NULL, QUI) != 0)
      xtermina("Error during reader condition initialization", QUI);
    if (xpthread_cond_init(&(ht.cond_w), NULL, QUI) != 0)
      xtermina("Error during writers initialization", QUI);
    ht.num_entry = 0;
    ht.num_readers = 0;
    ht.num_writers = 0;
}

// Crea un oggetto di tipo entry con chiave s e valore n
ENTRY *crea_entry(char *s, int n) {
  ENTRY *e = malloc(sizeof(ENTRY));
  if(e==NULL) termina("errore malloc entry 1");
  e->key = strdup(s); // salva copia di s
  e->data = malloc(sizeof(coppia));
  if(e->key==NULL || e->data==NULL)
    termina("errore malloc entry 2");
  // inizializzo coppia
  coppia *c = (coppia *) e->data; // cast obbligatorio
  c->valore  = n;
  c->next = NULL;
  return e;
}

void aggiungi (char *s) {
  if (s == NULL) return;
  ENTRY *e = crea_entry(s, 1);
  xpthread_mutex_lock(&(ht.hash_lock), QUI);
  // Schema che da precedenza ai thread scrittori
  while (ht.num_writers > 0)
  {
    xpthread_cond_wait(&(ht.cond_w), &(ht.hash_lock), QUI);  // Finchè ci sono scrittori rilascio la lock e aspetto
  }
  ht.num_writers++;
  ENTRY *r = hsearch(*e, FIND);  // Tramite FIND cerco nella tabella  
  if(r==NULL) {
    r = hsearch(*e,ENTER);  // Tramite ENTER inserisco nella tabella
    // Creo un oggetto di tipo coppia e lo collego alla linked list
    coppia *c = (coppia *) e->data;
    c->next = linked_list;
    linked_list = e;
    if(r==NULL) termina("Error or full table");
    ht.num_entry++;
  }
  else {
    //la stringa è gia' presente incremento il valore
    assert(strcmp(e->key,r->key)==0);
    coppia *c = (coppia *) r->data;
    c->valore++;
    // Libero la memoria
    free(e->key);
    free(e->data);
    free(e);
  }
  ht.num_writers--;
  // Sveglio prima eventuali thread scrittori in attesa
  xpthread_cond_signal(&ht.cond_w, QUI);
  // Sveglia eventuali thread lettori in attesa
  xpthread_cond_broadcast(&ht.cond_r, QUI);
  xpthread_mutex_unlock(&ht.hash_lock, QUI);
}

int conta(char *s) {
  int x = 0;
  ENTRY *e = crea_entry(s, 0); 
  // Cerco in mutua esclusione nella tabella
  xpthread_mutex_lock(&ht.hash_lock, QUI);
  while (ht.num_writers > 0)
  {
      xpthread_cond_wait(&(ht.cond_w), &(ht.hash_lock), QUI);
  }
  ht.num_readers++;
  ENTRY *r = hsearch(*e, FIND);
  // Se lo trovo prendo il valore
  if (r != NULL) {
    coppia *c = (coppia *) r->data;
    x = c->valore;
  }
  free(e->key);
  free(e->data);
  free(e);
  ht.num_readers--;
  // Sveglia eventuali thread scrittori in attesa
  xpthread_cond_signal(&(ht.cond_w), QUI);
  xpthread_mutex_unlock(&ht.hash_lock, QUI);
  return x;
}

// Restituisce il numero di elementi presenti nella tabella hash
int size() {
  return ht.num_entry;
}

// Distrugge la tabella hash, liberando la memoria associata
void destroy() {
  ENTRY *temp = NULL;
  // Ciclo per cancellare dalla memoria i dati presenti in tabella
  while (linked_list != NULL) {
    coppia *c = (coppia *)linked_list->data;
    temp = c->next;
    free(c);
    free(linked_list->key);
    free(linked_list);
    linked_list = temp;
  }
  // Distruggo mutex e condition variables associati alla struct hash_table
  if (xpthread_mutex_destroy(&ht.hash_lock, QUI) != 0)
    xtermina("Error in mutex destruction", QUI);
  if (xpthread_cond_destroy(&ht.cond_r, QUI) && xpthread_cond_destroy(&ht.cond_w, QUI))
    xtermina("Error in cond destruction", QUI);
  hdestroy();
}
