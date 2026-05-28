#!/bin/bash
#SBATCH --job-name=pi_full_exp
#SBATCH --output=exp_%j.out
#SBATCH --partition=plgrid
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=12
#SBATCH --time=02:00:00

module load openmpi

EXP_NUM=$1
if [ -z "$EXP_NUM" ]; then
    echo "Błąd: Nie podano numeru eksperymentu!"
    echo "Użycie: sbatch job.sh <nr_exp>"
    exit 1
fi

echo "Uruchamiam powtórzenie nr: $EXP_NUM"

./run_experiments.sh $EXP_NUM
