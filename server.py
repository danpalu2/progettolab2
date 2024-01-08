#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import struct, socket, signal
import logging, concurrent.futures, threading
import os, argparse, subprocess

# Host e Porta
HOST = "127.0.0.1" 
PORT = 56215 

# Variabili globali delle pipe
Scpipe = "caposc"
Letpipe = "capolet"

lock = threading.Lock()
Max_sequence_length = 2048
# Configurazione del file di log
logging.basicConfig(filename='server.log', level=logging.INFO, 
                    datefmt='%d/%m/%y %H:%M:%S', 
                    format='%(asctime)s - %(levelname)s - %(message)s')

# Crea le FIFO capolet e caposc se non esistono
if not os.path.exists(Scpipe):
    os.mkfifo(Scpipe)
if not os.path.exists(Letpipe):
    os.mkfifo(Letpipe)

# Funzione per ricevere tutti i byte inviati durante la connessione.
def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, 1024))
    if len(chunk) == 0:
      raise RuntimeError("socket connection broken")
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
  return chunks

# Gestisco una singola connessione con un client
def gestisci_connessione(conn, fd_pipe): 
  # Utilizzando with permetto di chiudere la connessione automaticamente.
  with conn:
    nSeq = 0
    nB = 0
    while True:  
      data = conn.recv(2); # Sequenza lunga al massimo 2048 bytes. 
      # Connessione di tipo A
      if not data:
        logging.info(f"Connection type: A. Bytes written into the pipe: {nB}")
        break
      # Leggo la lunghezza della sequenza
      seqLength  = struct.unpack("!h",data)[0]
      assert seqLength < Max_sequence_length, "The sequence is too long."
      # Connessione di tipo B
      if(seqLength == 0):
        nSeq += 1
        logging.info(f"Connection type: B. Bytes written into the pipe: {nB}")
        conn.sendall(struct.pack("!i",nSeq)) 
        break
      # Leggo la sequenza
      seq = recv_all(conn, seqLength)
      # Incremento il numero di sequenze e byte ricevuti
      nB += (2 + seqLength)
      nSeq += 1 
      # Scrivo la sequenza ricevuta all'interno della fifo.
      try:
        lock.acquire()
        os.write(fd_pipe, data)
        os.write(fd_pipe, seq)
        lock.release()
      except OSError as e:
        logging.error(f"Error while writing to pipe: {e}")

def main(max_threads):
  assert max_threads>0, "The number of threads for the pool must be greater than 0."
  # Apro le FIFO in scrittura
  fd_capolet = os.open(Letpipe, os.O_WRONLY)
  fd_caposc = os.open(Scpipe, os.O_WRONLY)
  # Creo il server socket
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    try:  
      # Riutilizzo la stessa porta
      s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
      s.bind((HOST, PORT))
      s.listen()
      # Crea un Thread Pool per gestire le connessioni
      with concurrent.futures.ThreadPoolExecutor(max_workers=max_threads) as executor:
        logging.info("Server started.")
        logging.info("Server waiting for connection...")
        while True:
          conn, addr = s.accept()
          connType = conn.recv(1).decode()
          # Capisco il tipo di connessione
          if connType == 'a':
            executor.submit(gestisci_connessione, conn, fd_capolet)
          elif connType == 'b':
            executor.submit(gestisci_connessione, conn, fd_caposc)
          else:  
            logging.info("Connection type not recognized")
    except KeyboardInterrupt:
      logging.info("Server closed")
      # Chiudo le FIFO
      os.close(fd_capolet)
      os.close(fd_caposc)
      # Rimuovo le FIFO dalla memoria
      os.unlink(Letpipe)
      os.unlink(Scpipe)
      # Termina programma
      p.send_signal(signal.SIGTERM)
      # Termino il server socket
      s.shutdown(socket.SHUT_RDWR)
      s.close()

# Definizione del modulo argparse
if __name__ == '__main__':
  parser = argparse.ArgumentParser(description="Server",formatter_class=argparse.RawTextHelpFormatter)
  parser.add_argument("max_threads", help="Maximum number of thread.", type=int)
  parser.add_argument('-r', help='Number of reader threads.', type = int, default=3)  
  parser.add_argument('-w', help='Number of writer threads.', type = int, default=3)  
  parser.add_argument('-v', action='store_true', help='Use valgrind to run archivio.') 
  args = parser.parse_args()

  if args.r < 3: 
    args.r = 3
  if args.w < 3:
    args.w = 3
  
  val = "valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all --log-file=valgrind-%p.log "
  exe = "./archivio -r "+str(args.r)+ " -w "+str(args.w)
  
  if args.v:
    cmd = val + exe
    tkn = cmd.split()
    p = subprocess.Popen(tkn)
  else:
    # esegui main come processo in background
    tkn = exe.split()
    p = subprocess.Popen(tkn)
  
  main(args.max_threads)
