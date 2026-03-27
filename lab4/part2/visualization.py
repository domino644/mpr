import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

BASE_DIR = "results"
SCALING_TYPES = ["strong", "weak"]
SIZES = ["small", "medium", "big"]
CORES_RANGE = range(1, 13)


def load_data():
    all_data = []
    for stype in SCALING_TYPES:
        for size in SIZES:
            path = os.path.join(BASE_DIR, stype, size)
            if not os.path.exists(path):
                continue

            for filename in os.listdir(path):
                if filename.endswith(".csv"):
                    df = pd.read_csv(os.path.join(path, filename))
                    all_data.append(df)

    return pd.concat(all_data, ignore_index=True)


def calculate_metrics(df):
    stats = (
        df.groupby(["Type", "Size", "Cores"])
        .agg({"Time": ["mean", "std"], "Points": "first"})
        .reset_index()
    )

    stats.columns = ["Type", "Size", "Cores", "Time_mean", "Time_std", "Points"]

    # Obliczanie metryk
    metrics_list = []
    for (stype, size), group in stats.groupby(["Type", "Size"]):
        group = group.sort_values("Cores")
        t1 = group[group["Cores"] == 1]["Time_mean"].values[0]

        if stype == "strong":
            group["Speedup"] = t1 / group["Time_mean"]
            group["Efficiency"] = group["Speedup"] / group["Cores"]
            group["Ideal_Speedup"] = group["Cores"]
        else:
            group["Speedup"] = (group["Cores"] * t1) / group["Time_mean"]
            group["Efficiency"] = group["Speedup"] / group["Cores"]
            group["Ideal_Speedup"] = group["Cores"]

        n = group["Cores"]
        s = group["Speedup"]
        group["Serial_Fraction"] = ((1 / s) - (1 / n)) / (1 - (1 / n))
        group.loc[group["Cores"] == 1, "Serial_Fraction"] = np.nan

        metrics_list.append(group)

    return pd.concat(metrics_list)


def plot_metrics(data):
    metrics_to_plot = [
        ("Time_mean", "Czas wykonania [s]", "Skalowanie czasu (log)", True),
        ("Speedup", "Przyspieszenie", "Przyspieszenie", False),
        ("Efficiency", "Efektywność", "Efektywność", False),
        (
            "Serial_Fraction",
            "Część sekwencyjna",
            "Część sekwencyjna",
            False,
        ),
    ]

    for stype in SCALING_TYPES:
        stype_data = data[data["Type"] == stype]

        for col, ylabel, title, use_log in metrics_to_plot:
            plt.figure(figsize=(10, 6))

            errorbar_legend_added = False

            for size in SIZES:
                subset = stype_data[stype_data["Size"] == size].sort_values("Cores")
                if subset.empty:
                    continue

                plt.scatter(
                    subset["Cores"],
                    subset[col],
                    label=f"Rozmiar: {size}",
                    s=80,
                    zorder=3,
                )
                plt.plot(
                    subset["Cores"],
                    subset[col],
                    alpha=0.4,
                    linestyle="--",
                    linewidth=2,
                    zorder=2,
                )
                if col == "Time_mean":
                    current_label = (
                        "Odchylenie standardowe" if not errorbar_legend_added else None
                    )
                    plt.errorbar(
                        subset["Cores"],
                        subset[col],
                        yerr=subset["Time_std"],
                        fmt="none",
                        ecolor="red",
                        elinewidth=2,
                        capsize=5,
                        capthick=1.5,
                        alpha=0.9,
                        zorder=5,
                        label=current_label,
                    )
                    errorbar_legend_added = True

            if col == "Speedup":
                plt.plot(
                    CORES_RANGE,
                    CORES_RANGE,
                    color="red",
                    label="Ideał (Liniowe)",
                    alpha=0.6,
                    linestyle="-",
                    zorder=1,
                )
            elif col == "Efficiency":
                plt.axhline(
                    1.0, color="red", label="Ideał (100%)", alpha=0.6, linestyle="-"
                )

            plt.title(f"{title} - Skalowanie {stype.capitalize()}", fontsize=14)
            plt.xlabel("Liczba procesorów (rdzeni)", fontsize=12)
            plt.ylabel(ylabel, fontsize=12)
            plt.xticks(list(CORES_RANGE))
            if use_log:
                plt.yscale("log")

            plt.legend(fontsize=10)
            plt.grid(True, which="both", ls="-", alpha=0.3)
            plt.tight_layout()

            filename = f"{stype}_{col}.png"
            plt.savefig(filename, dpi=300)
            plt.close()
            print(f"Zapisano: {filename}")


try:
    raw_df = load_data()
    processed_df = calculate_metrics(raw_df)
    processed_df.to_csv("processed_data.csv", index=False)
    plot_metrics(processed_df)
    print("Sukces! Sprawdź pliki PNG.")
except Exception as e:
    print(f"Błąd: {e}. Upewnij się, że folder 'results' zawiera dane.")
