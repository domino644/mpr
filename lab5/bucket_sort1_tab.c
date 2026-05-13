// Magdalena Pabisz
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_NUMBER 1000000
#define min(a,b) ((a) < (b) ? (a) : (b))

typedef struct Bucket
{
    int size;
    double *values;
    int max_size;
    // omp_lock_t lock;
} Bucket;


int bucket_sort(double *times, long long size, long long buckets, int threads);
void generate_random_numbers(double *numbers, long long size, int threads);
int split_numbers(double *numbers, long long size, Bucket *bucket_list, long long buckets, int threads);
void sort_buckets(Bucket *bucket_list, long long buckets);
void write_result(double *numbers, Bucket *bucket_list, long long buckets);

void add_number(double num, Bucket *bucket, int *overflow_flag);
void insertion_sort(Bucket *bucket);
int is_sorted(double *numbers, long long size);
void clean_up(double *numbers, Bucket *bucket_list, double *bucket_mem);
double calc_avg_time(double *times, int iterations);


int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: %s <size> <threads> <buckets> <iterations>\n", argv[0]);
        return 1;
    }


    long long size = atoll(argv[1]);
    int threads = atoi(argv[2]);
    long long buckets = atoll(argv[3]);
    int iterations = atoi(argv[4]);
    printf("Times for: size: %lld; threads: %d; buckets: %lld\n", size, threads, buckets);

    omp_set_num_threads(threads);

    double *times_random = malloc(iterations * sizeof(double));
    double *times_split = malloc(iterations * sizeof(double));
    double *times_sort_bucket = malloc(iterations * sizeof(double));
    double *times_write_result = malloc(iterations * sizeof(double));
    double *times_all = malloc(iterations * sizeof(double));
    double *times = malloc(5 * sizeof(double));

    int overflow, i = 0;
    while (i < iterations) {
        overflow = bucket_sort(times, size, buckets, threads);
        if (!overflow) {
            times_random[i] = times[0];
            times_split[i] = times[1];
            times_sort_bucket[i] = times[2];
            times_write_result[i] = times[3];
            times_all[i] = times[4];

            printf("[Iteration %d] time: %lf [s]\n", i + 1, times_all[i]);
            i++;
        }
        else {
            printf("[WARNING] overflow occured - one more try\n");
        }
    }

    printf("---; %lf; %lf; %lf; %lf; %lf\n\n", calc_avg_time(times_random, iterations),
        calc_avg_time(times_split, iterations),
        calc_avg_time(times_sort_bucket, iterations),
        calc_avg_time(times_write_result, iterations),
        calc_avg_time(times_all, iterations));

    free(times);
    free(times_random);
    free(times_split);
    free(times_sort_bucket);
    free(times_write_result);
    free(times_all);
    return 0;
}

int bucket_sort(double *times, long long size, long long buckets, int threads) {
    double time_start, time_start_step, time_end;
    int overflow;

    time_start = omp_get_wtime();
    // alokacja tablicy liczb
    double *numbers = malloc(size * sizeof(double));
    if (!numbers) {
        perror("Allocation error");
        exit(EXIT_FAILURE);
    }

    // alokacja tablicy kubełków
    int m = 3;
    long long bucket_size = (long long)(size / buckets) * m;
    Bucket *bucket_list = calloc(buckets, sizeof(Bucket));
    double *bucket_mem = malloc(size * m * sizeof(double));
    if (!bucket_mem) {
        perror("Bucket allocation error");
        exit(EXIT_FAILURE);
    }

#pragma omp parallel for schedule(guided)
    for (long long i = 0; i < buckets; i++) {
        bucket_list[i].values = bucket_mem + (i * bucket_size);
        bucket_list[i].max_size = bucket_size;
        bucket_list[i].size = 0;
    }

    // wypełnienie tablicy liczbami losowymi
    time_start_step = omp_get_wtime();
    generate_random_numbers(numbers, size, threads);
    times[0] = omp_get_wtime() - time_start_step;

    // Obliczanie sumy początkowej do weryfikacji
    double initial_sum = 0;
    for (long long i = 0; i < size; i++) {
        initial_sum += numbers[i];
    }

    // podział liczb do kubełków
    time_start_step = omp_get_wtime();
    overflow = split_numbers(numbers, size, bucket_list, buckets, threads);
    times[1] = omp_get_wtime() - time_start_step;

    if (overflow)
    {
        clean_up(numbers, bucket_list, bucket_mem);
        return 1;
    }

    // sortowanie kubełków
    time_start_step = omp_get_wtime();
    sort_buckets(bucket_list, buckets);
    times[2] = omp_get_wtime() - time_start_step;

    // przepisanie do posortowanych wartości
    time_start_step = omp_get_wtime();
    write_result(numbers, bucket_list, buckets);
    time_end = omp_get_wtime();
    times[3] = time_end - time_start_step;
    times[4] = time_end - time_start;

    // sprawdzenie poprawności
    int sorted = is_sorted(numbers, size);
    if (sorted == 0) {
        printf("NOT SORTED\n");
    }

    double final_sum = 0;
    for (long long i = 0; i < size; i++) {
        final_sum += numbers[i];
    }

    long long total_bucket_elements = 0;
    for (long long i = 0; i < buckets; i++) {
        total_bucket_elements += bucket_list[i].size;
    }

    if (sorted && total_bucket_elements == size) {
        printf("[VERIFICATION] Success: Sorted, Count matches (%lld), Sum matches (delta: %e)\n",
            total_bucket_elements, initial_sum - final_sum);
    }
    else {
        printf("[VERIFICATION] FAILED: Sorted: %s, Elements: %lld/%lld, Sum delta: %e\n",
            sorted ? "YES" : "NO", total_bucket_elements, size, initial_sum - final_sum);
    }

    clean_up(numbers, bucket_list, bucket_mem);
    return 0;
}

void generate_random_numbers(double *numbers, long long size, int threads) {
#pragma omp parallel
    {
        unsigned int seed = (int)omp_get_wtime() * 1e6 + omp_get_thread_num();

#pragma omp for	schedule(guided)
        for (long long i = 0; i < size; i++) {
            // losowanie liczb z przedziału [0, 1]
            numbers[i] = (double)rand_r(&seed) / RAND_MAX * MAX_NUMBER;
        }
    }
}


int split_numbers(double *numbers, long long size, Bucket *bucket_list, long long buckets, int threads) {
    int thread_id, buckets_per_thread_base, buckets_per_thread_rest;
    double bucket_interval, bucket_start, bucket_end;
    bucket_interval = MAX_NUMBER / (double)buckets;
    buckets_per_thread_base = buckets / threads;
    buckets_per_thread_rest = buckets % threads;
    int overflow_flag = 0;


#pragma omp parallel private(thread_id, bucket_start, bucket_end)
    {
        thread_id = omp_get_thread_num();
        bucket_start = (thread_id * buckets_per_thread_base + min(thread_id, buckets_per_thread_rest)) * bucket_interval;
        bucket_end = bucket_start + (buckets_per_thread_base + (thread_id < buckets_per_thread_rest ? 1 : 0)) * bucket_interval;
        // printf("%d %lf %lf\n", thread_id, bucket_start, bucket_end);
        for (long long i = 0; i < size; i++) {
            if (bucket_start <= numbers[i] && numbers[i] < bucket_end) {
                int bucket_id = (int)(numbers[i] * buckets / (double)MAX_NUMBER);
                add_number(numbers[i], bucket_list + bucket_id, &overflow_flag);
            }
        }
    }
    return overflow_flag;
}

void sort_buckets(Bucket *bucket_list, long long buckets) {
#pragma omp parallel for schedule(guided)
    for (long long i = 0; i < buckets; i++)
    {
        insertion_sort(bucket_list + i);
    }

}

void write_result(double *numbers, Bucket *bucket_list, long long buckets) {
    long long *start = calloc(buckets, sizeof(long long));
    for (long long i = 1; i < buckets; i++) {
        start[i] = start[i - 1] + bucket_list[i - 1].size;
    }

#pragma omp parallel for schedule(guided)
    for (long long i = 0; i < buckets; i++) {
        long long idx = start[i];
        for (int j = 0; j < bucket_list[i].size; j++) {
            numbers[idx + j] = bucket_list[i].values[j];
        }
    }
    free(start);
}

void add_number(double num, Bucket *bucket, int *overflow_flag) {
    if (bucket->size < bucket->max_size) {
        bucket->values[bucket->size] = num;
        bucket->size++;
    }
    else {
#pragma omp atomic write
        *overflow_flag = 1;
    }
}

void insertion_sort(Bucket *bucket)
{
    double *arr = bucket->values;
    int n = bucket->size;

    for (int i = 1; i < n; i++)
    {
        double val = arr[i];
        int j = i - 1;

        while (j >= 0 && arr[j] > val)
        {
            arr[j + 1] = arr[j];
            j--;
        }

        arr[j + 1] = val;
    }
}

int is_sorted(double *numbers, long long size)
{
    for (long long i = 1; i < size; i++) {
        if (numbers[i] < numbers[i - 1]) {
            return 0;
        }
    }
    return 1;
}

double calc_avg_time(double *times, int iterations) {
    double sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += times[i];
    }
    return sum / iterations;
}

void clean_up(double *numbers, Bucket *bucket_list, double *bucket_mem)
{
    free(bucket_mem);
    free(bucket_list);
    free(numbers);
}
