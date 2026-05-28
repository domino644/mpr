from collections import defaultdict
import matplotlib.pyplot as plt


GEN, SPLIT, SORT, WRITE, TOTAL = 4, 5, 6, 7, 8
THREADS = 1
ITERS = 10
LW = 0.7
TYPE_NAMES = {
    GEN: "Losowanie liczb",
    SPLIT: "Rozdział do kubełków",
    SORT: "Sortowanie kubełków",
    WRITE: "Przepisywanie do tablicy początkowej",
    TOTAL: "Całość algorytmu",
}

TYPE_NAMES2 = {GEN: "gen", SPLIT: "split", SORT: "sort", WRITE: "write", TOTAL: "total"}


# Iteration,Threads,Size,Buckets,Gen_Time,Split_Time,Sort_Time,Write_Time,Total_Time
# 1,1,1000000000,10000000,5.765560,54.016945,83.897065,3.802469,147.553144
# data[(threads, type)] = []
def parse_line(res_data, line):
    d = line.split(",")
    threads = int(d[THREADS])
    for i in range(GEN, TOTAL + 1):
        res_data[(threads, i)].append(float(d[i]))


def parse_file(filename):
    data = defaultdict(list)

    with open(filename) as f:
        lines = f.readlines()

    for i in range(1, len(lines)):
        if not len(lines[i]):
            continue
        parse_line(data, lines[i])

    calc_avg(data)

    return data


def calc_avg(data):
    for key, values in data.items():
        if len(values) != ITERS:
            print("WRONG ITERS")
        avg = sum(values) / len(values)
        data[key] = avg


def plot_four_times(data_list, show=False):
    types = [GEN, SPLIT, SORT, WRITE]

    fig, axes = plt.subplots(2, 2, figsize=(12, 8))
    axes = axes.flatten()

    for ax, t in zip(axes, types):
        for data, label in data_list:
            threads_values = sorted({k[0] for k in data.keys() if k[1] == t})
            times = [data[(thr, t)] for thr in threads_values]

            ax.plot(threads_values, times, marker="o", linewidth=LW, label=label)

        ax.set_title(TYPE_NAMES[t])
        ax.set_xlabel("Liczba wątków")
        ax.set_ylabel("Czas [s]")
        ax.grid()
        ax.legend()

    plt.tight_layout()
    plt.savefig("lab5/results_extra/plots/times_four.png")
    if show:
        plt.show()


def plot_one_times(data_list, t, show=False):
    fig, ax = plt.subplots(figsize=(8, 5))

    for data, label in data_list:
        threads_values = sorted({k[0] for k in data.keys() if k[1] == t})
        times = [data[(thr, t)] for thr in threads_values]

        ax.plot(threads_values, times, marker="o", linewidth=LW, label=label)

    ax.set_title(TYPE_NAMES[t])
    ax.set_xlabel("Liczba wątków")
    ax.set_ylabel("Czas [s]")
    ax.grid()
    ax.legend()

    plt.tight_layout()
    plt.savefig(f"lab5/results_extra/plots/time_{TYPE_NAMES2[t]}.png")
    if show:
        plt.show()


def plot_four_speedup(data_list, data_seq, show=False):
    types = [GEN, SPLIT, SORT, WRITE]

    fig, axes = plt.subplots(2, 2, figsize=(12, 8))
    axes = axes.flatten()

    for ax, t in zip(axes, types):
        for data, label in data_list:
            threads_values = sorted({k[0] for k in data.keys() if k[1] == t})
            seq_time = data_seq.get((1, t))
            speedup = [seq_time / data[(thr, t)] for thr in threads_values]

            ax.plot(threads_values, speedup, marker="o", linewidth=LW, label=label)

        ax.plot(
            threads_values, threads_values, linestyle="--", label="Ideal", color="black"
        )
        ax.set_title(TYPE_NAMES[t])
        ax.set_xlabel("Liczba wątków")
        ax.set_ylabel("Przyspieszenie")
        ax.grid()
        ax.legend()

    plt.tight_layout()
    plt.savefig("lab5/results_extra/plots/speedup_four.png")
    if show:
        plt.show()


def plot_one_speedup(data_list, data_seq, t, show=False):
    fig, ax = plt.subplots(figsize=(8, 5))

    for data, label in data_list:
        threads_values = sorted({k[0] for k in data.keys() if k[1] == t})
        seq_time = data_seq.get((1, t))
        speedup = [seq_time / data[(thr, t)] for thr in threads_values]

        ax.plot(threads_values, speedup, marker="o", linewidth=LW, label=label)

    ax.plot(
        threads_values, threads_values, linestyle="--", label="Ideal", color="black"
    )
    ax.set_title(TYPE_NAMES[t])
    ax.set_xlabel("Liczba wątków")
    ax.set_ylabel("Przyspieszenie")
    ax.grid()
    ax.legend()

    plt.tight_layout()
    plt.savefig(f"lab5/results_extra/plots/speedup_{TYPE_NAMES2[t]}.png")
    if show:
        plt.show()


data2 = parse_file("lab5/results_extra/data/results_extra_variant2.csv")
data2aligned = parse_file("lab5/results_extra/data/results_extra_variant2_aligned.csv")
data4 = parse_file("lab5/results_extra/data/results_extra_variant4.csv")
data4aligned = parse_file("lab5/results_extra/data/results_extra_variant4_aligned.csv")

data_par = [
    (data2, "Wariant 2"),
    (data2aligned, "Wariant 2 Machine Sympathy"),
    (data4, "Wariant 4"),
    (data4aligned, "Wariant 4 Machine Sympathy"),
]

show = False
plot_four_times(data_par, show)
plot_one_times(data_par, TOTAL, show)

print("DONE")
