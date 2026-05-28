#!/usr/bin/env python
from mpi4py import MPI
import argparse
import csv
import socket

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

parser = argparse.ArgumentParser(description="MPI Benchmark to CSV")
parser.add_argument(
    "-t", "--type", choices=["buffered", "sync"], required=True, help="Typ komunikacji"
)
parser.add_argument(
    "-o", "--output", required=True, help="Nazwa pliku wyjściowego .csv"
)
parser.add_argument(
    "-n", "--N", type=int, default=1000, help="Liczba powtórzeń dla każdego rozmiaru"
)
args = parser.parse_args()

msg_sizes = [
    1,
    2,
    4,
    8,
    16,
    32,
    64,
    128,
    256,
    512,
    1024,
    4096,
    16384,
    65536,
    262144,
    1048576,
    4194304,
    8388608,
]

results = []

if size < 2:
    if rank == 0:
        print("Błąd: Wymagane co najmniej 2 procesy.")
        exit()

print(f"Rank {rank} is running on {socket.gethostname()}")

for s in msg_sizes:
    message = bytearray(s)

    if args.type == "buffered":
        buffer_size = s + MPI.BSEND_OVERHEAD + 2048
        buff = bytearray(buffer_size)
        MPI.Attach_buffer(buff)

    comm.Barrier()
    times = []

    if rank == 0:
        for _ in range(args.N):
            start = MPI.Wtime()
            if args.type == "buffered":
                comm.Bsend(message, dest=1)
            else:
                comm.Ssend(message, dest=1)

            comm.Recv(message, source=1)
            end = (MPI.Wtime() - start) / 2
            times.append(end)

        avg_time = sum(times) / args.N
        latency_ms = avg_time * 1000
        bandwidth = (s * 8) / (avg_time * 1e6) if avg_time > 0 else 0

        results.append(
            {
                "size_bytes": s,
                "latency_ms": round(latency_ms, 6),
                "bandwidth_mbit_s": round(bandwidth, 4),
            }
        )
        print(
            f"Rozmiar: {s:>8} B | Latency: {latency_ms:8.4f} ms | BW: {bandwidth:10.2f} Mbit/s"
        )

    elif rank == 1:
        for _ in range(args.N):
            comm.Recv(message, source=0)
            if args.type == "buffered":
                comm.Bsend(message, dest=0)
            else:
                comm.Ssend(message, dest=0)

    if args.type == "buffered":
        MPI.Detach_buffer()

if rank == 0:
    keys = results[0].keys()
    with open(args.output, "w", newline="") as f:
        dict_writer = csv.DictWriter(f, fieldnames=keys)
        dict_writer.writeheader()
        dict_writer.writerows(results)
    print(f"\nWyniki zapisano do: {args.output}")
