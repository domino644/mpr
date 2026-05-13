#!/bin/bash
#SBATCH --time=1:30:00
#SBATCH --output=result-%a.out
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=48
#SBATCH --partition=plgrid
#SBATCH --account=plgmpr26-cpu
#SBATCH --array=1-10

module load gcc/13.2.0

SEQUENTIAL_FILENAME=bucket_sort_seq
PARALLEL_V1_FILENAME=bucket_sort1_tab
PARALLEL_V2_FILENAME=bucket_sort2_tab
PARALLEL_V4_FILENAME=bucket_sort4_tab

PROBLEM_SIZE=2000000000
BUCKETS=50000
ITERATIONS=1

gcc -fopenmp ${SEQUENTIAL_FILENAME}.c -o ${SEQUENTIAL_FILENAME}
gcc -fopenmp ${PARALLEL_V1_FILENAME}.c -o ${PARALLEL_V1_FILENAME}
gcc -fopenmp ${PARALLEL_V2_FILENAME}.c -o ${PARALLEL_V2_FILENAME}
gcc -fopenmp ${PARALLEL_V4_FILENAME}.c -o ${PARALLEL_V4_FILENAME}

echo "=== SEQUENTIAL ==="
./"${SEQUENTIAL_FILENAME}" ${PROBLEM_SIZE} ${BUCKETS} ${ITERATIONS}

for thread_num in $(seq 2 2 48); do
    echo "=== THREADS: ${thread_num} ==="
    
    echo "--- Variant 1 ---"
    ./"${PARALLEL_V1_FILENAME}" ${PROBLEM_SIZE} ${thread_num} ${BUCKETS} ${ITERATIONS}
    
    echo "--- Variant 2 ---"
    ./"${PARALLEL_V2_FILENAME}" ${PROBLEM_SIZE} ${thread_num} ${BUCKETS} ${ITERATIONS}
    
    echo "--- Variant 4 ---"
    ./"${PARALLEL_V4_FILENAME}" ${PROBLEM_SIZE} ${thread_num} ${BUCKETS} ${ITERATIONS}
done
