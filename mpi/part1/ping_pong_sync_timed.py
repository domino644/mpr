#!/usr/bin/env python
from mpi4py import MPI
from time import sleep
from sys import getsizeof

comm = MPI.COMM_WORLD
rank = comm.Get_rank()

print(f"{MPI.Get_processor_name()}")

N = 10000
i = 0
times = []
message = bytearray(10000)

comm.Barrier()

if rank == 0:
    while i < N:
      start = MPI.Wtime()
      comm.ssend(message, dest=1)
      rec_data = comm.recv(source=1)
      end = (MPI.Wtime() - start) / 2
      times.append(end)
      i += 1
elif rank == 1:
    while i < N:
      data = comm.recv(source=0)
      comm.ssend(message, dest=0)
      i += 1
else:
    print("Expected only two nodes")

if rank == 0:
	print("Communication type: synchronous")
	print(f"Number of messages: {N}")
	print(f"Size of message: {message.__sizeof__()}")
	print(f"Average time for 1 communicate: {sum(times) / N}")
