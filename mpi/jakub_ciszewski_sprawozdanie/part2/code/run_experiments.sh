#!/bin/bash
EXP_NUM=${1:-1}
PROGRAM="./pi"

SMALL=5000000
MEDIUM=550000000
LARGE=60000000000
SMALL_PER_CORE=416000
MEDIUM_PER_CORE=45000000
LARGE_PER_CORE=5000000000

echo "Rozpoczynam eksperyment nr: $EXP_NUM"

run_and_save() {
    local TYPE=$1
    local SIZE=$2
    local CORES=$3
    local POINTS=$4
    
    local DIR="results/$TYPE/$SIZE"
    mkdir -p "$DIR"
    
    local OUT_FILE="$DIR/result-${CORES}-${EXP_NUM}.csv"
    
    echo "Type,Size,Cores,Points,Time,Pi" > "$OUT_FILE"
    
    RES=$(mpirun -np $CORES $PROGRAM $POINTS)
    
    TIME=$(echo "$RES" | grep "Całkowity czas" | awk '{print $4}')
    PI=$(echo "$RES" | grep "Przybliżenie PI" | awk '{print $3}')
    
    echo "$TYPE,$SIZE,$CORES,$POINTS,$TIME,$PI" >> "$OUT_FILE"
    echo "Zapisano: $OUT_FILE (Czas: $TIME s)"
}

for SIZE_LABEL in "small" "medium" "big"
do
    if [ "$SIZE_LABEL" == "small" ]; then 
        S_TOTAL=$SMALL; W_PER=$SMALL_PER_CORE
    elif [ "$SIZE_LABEL" == "medium" ]; then 
        S_TOTAL=$MEDIUM; W_PER=$MEDIUM_PER_CORE
    else 
        S_TOTAL=$LARGE; W_PER=$LARGE_PER_CORE
    fi

    for cores in {1..12}
    do
        run_and_save "strong" "$SIZE_LABEL" "$cores" "$S_TOTAL"
        
        POINTS_WEAK=$((W_PER * cores))
        run_and_save "weak" "$SIZE_LABEL" "$cores" "$POINTS_WEAK"
    done
done

echo "Koniec powtórzenia nr $EXP_NUM."
