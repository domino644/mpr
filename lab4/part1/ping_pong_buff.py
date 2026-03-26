#!/usr/bin/env python
from mpi4py import MPI
from time import sleep

comm = MPI.COMM_WORLD
rank = comm.Get_rank()

buff = bytearray(1024)
MPI.Attach_buffer(buff)

if rank == 0:
    while True:
      comm.bsend("ping", dest=1)
      rec_data = comm.recv(source=1)
      print(rec_data, flush=True)
      sleep(1)
elif rank == 1:
    while True:
      data = comm.recv(source=0)
      print(data, flush=True)
      comm.bsend("pong", dest=0)
      sleep(2)
else:
    print("Expected only two nodes")


MPI.Detach_buffer()
