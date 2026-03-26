#!/usr/bin/env python
from mpi4py import MPI
from time import sleep

comm = MPI.COMM_WORLD
rank = comm.Get_rank()

if rank == 0:
    while True:
      comm.issend("ping", dest=1)
      rec_data = comm.recv(source=1)
      print(rec_data, flush=True)
      sleep(1)
elif rank == 1:
    while True:
      data = comm.recv(source=0)
      print(data, flush=True)
      comm.issend("pong", dest=0)
      sleep(2)
else:
    print("Expected only two nodes")
