#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import socket, os, struct
import argparse, concurrent.futures
Max_sequence_length = 2048

# Host e Porta
HOST = "127.0.0.1" 
PORT = 56215 

# Funzione per ricevere tutti i byte inviati durante la connessione.
def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, 1024))
    if len(chunk) == 0:
      raise RuntimeError("Socket connection broken.")
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
    assert bytes_recd == len(chunks)
  return chunks

# Invio dati al server
def send_seq(socket, data):
  length = len(data)
  assert length <= Max_sequence_length, "Data length exceeds maximum in client2"
  length_bytes = struct.pack("!h", length)
  socket.sendall(length_bytes) 
  socket.sendall(data)

# Crea una connessione per ogni file
def connetti(file):
  try:
      nSeq = 0
      with open(file, 'r') as f:
          # Creo il socket
          with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
              s.connect((HOST, PORT))
              # Invio tipo connessione al server
              b = 'b'  
              s.sendall(b.encode())
              # Invio le linee del file al server
              for line in f:
                  send_seq(s, line.encode())
                  nSeq += 1
              # Invia una sequenza di lunghezza 0 per indicare la fine
              send_seq(s, b"")
              nSeq += 1
              # Confronto numero sequenze inviate con ricevute 
              data = recv_all(s, 4);
              nSeq_rec = struct.unpack("!i", data)[0]
              assert nSeq == nSeq_rec, "Number of sequences sent and received don't match"
              # Chiude il socket
              s.shutdown(socket.SHUT_RDWR)
  except FileNotFoundError:
      print("Invalid file: ", file)

def main(files):
  # Creo un thread per ogni file
  with concurrent.futures.ThreadPoolExecutor() as executor:
    thread_list = []
    for f in files:
      t = executor.submit(connetti, f)
      thread_list.append(t)
    # Attendo che tutti i thread abbiano finito
    for t in thread_list:
      t.result()

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description="Client2",formatter_class=argparse.RawTextHelpFormatter)
  parser.add_argument("files", nargs="+", help="Enter files names")
  args = parser.parse_args()
  
  main(args.files)
