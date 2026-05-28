#!/bin/bash
#SBATCH --time=3:00:00
#SBATCH --output=result_extra-%a.out
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH --partition=plgrid
#SBATCH --account=plgmpr26-cpu
#SBATCH --array=1-10

module load gcc/13.2.0

VARIANT2_FILENAME=bucket_sort2_tab_quicksort
VARIANT2_ALIGNED_FILENAME=bucket_sort2_tab_aligned
VARIANT4_FILENAME=bucket_sort4_tab_quicksort
VARIANT4_ALIGNED_FILENAME=bucket_sort4_tab_aligned

PROBLEM_SIZE=2000000000
BUCKETS=100
ITERATIONS=1

gcc -fopenmp ${VARIANT2_FILENAME}.c -o ${VARIANT2_FILENAME}
gcc -fopenmp ${VARIANT2_ALIGNED_FILENAME}.c -o ${VARIANT2_ALIGNED_FILENAME}
gcc -fopenmp ${VARIANT4_FILENAME}.c -o ${VARIANT4_FILENAME}
gcc -fopenmp ${VARIANT4_ALIGNED_FILENAME}.c -o ${VARIANT4_ALIGNED_FILENAME}

for thread_num in $(seq 2 2 48); do
    echo "=== THREADS: ${thread_num} ==="

    echo "--- Variant 2 ---"
    ./"${VARIANT2_FILENAME}" ${PROBLEM_SIZE} ${thread_num} ${BUCKETS} ${ITERATIONS}
    
    echo "--- Variant 2 ALIGNED ---"
    ./"${VARIANT2_ALIGNED_FILENAME}" ${PROBLEM_SIZE} ${thread_num} ${BUCKETS} ${ITERATIONS}
    
    echo "--- Variant 4 ---"
    ./"${VARIANT4_FILENAME}" ${PROBLEM_SIZE} ${thread_num} ${BUCKETS} ${ITERATIONS}
    
    echo "--- Variant 4 ALIGNED ---"
    ./"${VARIANT4_ALIGNED_FILENAME}" ${PROBLEM_SIZE} ${thread_num} ${BUCKETS} ${ITERATIONS}
done
