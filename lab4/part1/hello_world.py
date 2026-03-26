#!/usr/bin/env python
from mpi4py import MPI
import socket

comm = MPI.COMM_WORLD

print("hello world")
print(f"my rank is: {comm.rank}, at node {socket.gethostname()}")
