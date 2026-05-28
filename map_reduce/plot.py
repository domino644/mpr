import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

df = pd.read_csv("map_reduce/results.csv")

# Ustawienie kolejności sortowania na osi X
df["Size"] = pd.Categorical(
    df["Size"], categories=["1GB", "10GB", "20GB"], ordered=True
)

# Mapowanie nazw na bardziej czytelne dla legendy
node_mapping = {
    "Primary": "Sekwencyjny",
    "xlarge": "Hadoop (xlarge)",
    "2xlarge": "Hadoop (2xlarge)",
}
df["Konfiguracja"] = df["Node"].map(node_mapping)

# Ustawienie stylu wykresów
sns.set_theme(style="whitegrid", palette="Set2")

# ==========================================
# WYKRES 1: Średni czas wykonania (Słupkowy z wariancją)
# ==========================================
plt.figure(figsize=(10, 6))
# errorbar='sd' pokaże odchylenie standardowe na słupkach, capsize dodaje poprzeczki
ax1 = sns.barplot(
    data=df, x="Size", y="Time", hue="Konfiguracja", errorbar="sd", capsize=0.1
)
plt.title(
    "Średni czas wykonania programu (z uwzględnieniem wariancji)", fontsize=14, pad=15
)
plt.xlabel("Rozmiar problemu (N)", fontsize=12)
plt.ylabel("Czas [ms]", fontsize=12)
plt.legend(title="Konfiguracja")
plt.tight_layout()
plt.savefig("hist.png")

# ==========================================
# WYKRES 2: Wzrost czasu wykonania (Liniowy)
# ==========================================
plt.figure(figsize=(10, 6))
# style="Konfiguracja" + dashes=True zrobi przerywane linie, markers=True doda kropki
ax2 = sns.lineplot(
    data=df,
    x="Size",
    y="Time",
    hue="Konfiguracja",
    style="Konfiguracja",
    markers=["o", "s", "D"],
    dashes=[(2, 2), (2, 2), (2, 2)],
    linewidth=2.5,
    markersize=8,
)
plt.title("Wzrost czasu w zależności od rozmiaru danych", fontsize=14, pad=15)
plt.xlabel("Rozmiar problemu (N)", fontsize=12)
plt.ylabel("Czas [ms]", fontsize=12)
plt.legend(title="Konfiguracja")
plt.tight_layout()
plt.savefig("linear.png")

# ==========================================
# OBLICZENIA I WYKRES 3: Przyspieszenie bezwzględne (Liniowy)
# ==========================================
# Wyliczamy średnie z poszczególnych przebiegów
mean_df = df.groupby(["Size", "Konfiguracja"])["Time"].mean().reset_index()

# Wyciągamy czasy sekwencyjne dla referencji (Sk(N))
seq_times = mean_df[mean_df["Konfiguracja"] == "Sekwencyjny"].set_index("Size")["Time"]


# Funkcja licząca wzór S(N,p) = Sk(N) / T(N,p)
def calculate_speedup(row):
    if row["Konfiguracja"] == "Sekwencyjny":
        return 1.0  # Przyspieszenie sekwencyjnego względem samego siebie to 1
    return seq_times[row["Size"]] / row["Time"]


mean_df["Speedup"] = mean_df.apply(calculate_speedup, axis=1)

# Filtrujemy tylko wersje zrównoleglone do wyświetlenia na wykresie
speedup_df = mean_df[mean_df["Konfiguracja"] != "Sekwencyjny"]

plt.figure(figsize=(10, 6))

# Zmienione na lineplot z przerywaną linią (dashes=True) i znacznikami na punktach (markers=True)
ax3 = sns.lineplot(
    data=speedup_df,
    x="Size",
    y="Speedup",
    hue="Konfiguracja",
    style="Konfiguracja",
    markers=["o", "s"],
    dashes=[(2, 2), (2, 2)],
    linewidth=2.5,
    markersize=10,
)

plt.title(
    "Przyspieszenie bezwzględne $S(N, p)$ względem wersji sekwencyjnej",
    fontsize=14,
    pad=15,
)
plt.xlabel("Rozmiar problemu (N)", fontsize=12)
plt.ylabel("Przyspieszenie S(N, p)", fontsize=12)

# Dodajemy czerwoną linię pokazującą barierę przyspieszenia (gdzie czas Hadoop = czas sekwencyjny)
plt.axhline(
    y=1.0,
    color="red",
    linestyle="--",
    linewidth=1.5,
    alpha=0.7,
    label="Próg opłacalności (S=1)",
)

# Wymuszamy, by oś Y zaczynała się od zera i kończyła nieco nad czerwoną linią
plt.ylim(0, 1.2)

plt.legend()
plt.tight_layout()
plt.savefig("speedup.png")
