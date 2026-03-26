#!/bin/bash

# Sprawdź czy podano numer powtórzenia jako argument (np. ./run_experiments.sh 1)
EXP_NUM=${1:-1}
PROGRAM="./pi"

# --- NOWA KONFIGURACJA DOPASOWANA DO ARESA ---
SMALL=5000000              # ok. 0.06s na 1 rdzeniu
MEDIUM=550000000           # ok. 7s na 1 rdzeniu
LARGE=60000000000          # ok. 12 min na 1 rdzeniu -> ok. 1 min na 12 rdzeniach

SMALL_PER_CORE=416000
MEDIUM_PER_CORE=45000000
LARGE_PER_CORE=5000000000  # to jest Twoje bazowe 5 mld punktów

echo "Rozpoczynam eksperyment nr: $EXP_NUM"

# Funkcja pomocnicza do uruchamiania i zapisywania
run_and_save() {
    local TYPE=$1    # strong / weak
    local SIZE=$2    # small / medium / big
    local CORES=$3
    local POINTS=$4
    
    # Tworzenie ścieżki folderu: np. results/strong/big
    local DIR="results/$TYPE/$SIZE"
    mkdir -p "$DIR"
    
    local OUT_FILE="$DIR/result-${CORES}-${EXP_NUM}.csv"
    
    # Nagłówek dla pojedynczego pliku
    echo "Type,Size,Cores,Points,Time,Pi" > "$OUT_FILE"
    
    # Uruchomienie MPI
    RES=$(mpirun -np $CORES $PROGRAM $POINTS)
    
    # Wyciąganie danych
    TIME=$(echo "$RES" | grep "Całkowity czas" | awk '{print $4}')
    PI=$(echo "$RES" | grep "Przybliżenie PI" | awk '{print $3}')
    
    # Zapis do pliku
    echo "$TYPE,$SIZE,$CORES,$POINTS,$TIME,$PI" >> "$OUT_FILE"
    echo "Zapisano: $OUT_FILE (Czas: $TIME s)"
}

# --- PĘTLA ROZMIARÓW ---
for SIZE_LABEL in "small" "medium" "big"
do
    if [ "$SIZE_LABEL" == "small" ]; then 
        S_TOTAL=$SMALL; W_PER=$SMALL_PER_CORE
    elif [ "$SIZE_LABEL" == "medium" ]; then 
        S_TOTAL=$MEDIUM; W_PER=$MEDIUM_PER_CORE
    else 
        S_TOTAL=$LARGE; W_PER=$LARGE_PER_CORE
    fi

    # --- PĘTLA RDZENI 1-12 ---
    for cores in {1..12}
    do
        # Skalowanie SILNE
        run_and_save "strong" "$SIZE_LABEL" "$cores" "$S_TOTAL"
        
        # Skalowanie SŁABE
        POINTS_WEAK=$((W_PER * cores))
        run_and_save "weak" "$SIZE_LABEL" "$cores" "$POINTS_WEAK"
    done
done

echo "Koniec powtórzenia nr $EXP_NUM."
