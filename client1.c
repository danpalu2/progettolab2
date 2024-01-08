#include "xerrori.h"

// host e port a cui connettersi
#define HOST "127.0.0.1"
#define PORT 56215
#define MAX_SEQUENCE_LENGTH 2048

// Scrive "n" bytes in un file descriptor
ssize_t writen(int fd, void *ptr, size_t n) {
  size_t nleft;
  ssize_t nwritten;
  nleft = n;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) < 0) {
      if (nleft == n)
        return -1; /* error, return -1 */
      else
        break; /* error, return amount written so far */
    } else if (nwritten == 0)
      break;
    nleft -= nwritten;
    ptr += nwritten;
  }
  return (n - nleft); /* return >= 0 */
}

void connetti(char *buff, int seq_length) {
  size_t e;
  uint16_t len;
  char a = 'a';
  // Operazioni per la gestione del socket
  int fd_skt = 0; // File descriptor associato al socket
  struct sockaddr_in serv_addr;
  // Creo il socket
  fd_skt = xsocket(AF_INET, SOCK_STREAM, 0, QUI);
  // Assegno l'indirizzo
  serv_addr.sin_family = AF_INET;
  // Converto il numero della porta in network order
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = inet_addr(HOST);
  // Apro la connessione
  xconnect(fd_skt, &serv_addr, sizeof(serv_addr), QUI);
  // Invio tipo connessione al server
  e = writen(fd_skt, &a, sizeof(a));
  if (e != sizeof(a))
    xtermina("Write error", QUI);
  // Invio lunghezza della sequenza
  len = htons(seq_length);
  e = writen(fd_skt, &len, sizeof(len));
  if (e != sizeof(len))
    xtermina("Write error", QUI);
  // Invio la sequenza
  e = writen(fd_skt, buff, seq_length);
  if (e != seq_length)
    xtermina("Write error", QUI);
  // Chiusura del socket
  xclose(fd_skt, QUI);
}

int main(int argc, char const *argv[]) {
  if (argc != 2) {
    printf("Enter file name next to ./client1.out\n");
    exit(1);
  }
  // Operazioni per la gestione del file
  FILE *f = xfopen(argv[1], "r", QUI);
  size_t buffer_length = 10;
  int seq_length;
  char *buffer = malloc(buffer_length*sizeof(char));
  // Ciclo di lettura dal file
  while ((seq_length = getline(&buffer, &buffer_length, f)) > 0) {
    assert(seq_length <= MAX_SEQUENCE_LENGTH &&
           "The line exceeds the maximum in client1");
    connetti(buffer, seq_length);
  }
  free(buffer);
  // Chiusura del file
  if (fclose(f) != 0)
    xtermina("File closing error in client1", QUI);
  return 0;
}
