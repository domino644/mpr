#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Użycie: mpirun -np <N> %s <liczba_punktów>\n", argv[0]);
        return 1;
    }

    long long total_points = atoll(argv[1]);
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    long long points_per_process = total_points / size;
    long long remainder = total_points % size;
    
    if (rank == size - 1) {
        points_per_process += remainder;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    unsigned int seed = time(NULL) + rank;
    long long circle_points = 0;

    for (long long i = 0; i < points_per_process; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;
        
        // Równanie okręgu: x^2 + y^2 <= 1 (dla ćwiartki koła o r=1)
        if (x * x + y * y <= 1.0) {
            circle_points++;
        }
    }

    double end_time = MPI_Wtime();
    double local_elapsed = end_time - start_time;

    // Zbieranie wyników (liczba punktów w kole) do procesu 0
    long long total_circle_points = 0;
    MPI_Reduce(&circle_points, &total_circle_points, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    // Zbieranie czasów wykonania z każdego procesu do tablicy w procesie 0
    double* all_times = NULL;
    if (rank == 0) {
        all_times = (double*)malloc(size * sizeof(double));
    }
    MPI_Gather(&local_elapsed, 1, MPI_DOUBLE, all_times, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Wyświetlanie wyników przez proces 0
    if (rank == 0) {
        double pi_estimate = 4.0 * (double)total_circle_points / (double)total_points;
        
        printf("--- WYNIKI KONFIGURACJI ---\n");
        printf("Liczba punktów ogółem: %lld\n", total_points);
        printf("Liczba procesów MPI:  %d\n", size);
        printf("Przybliżenie PI:      %.10f\n", pi_estimate);
        printf("Błąd bezwzględny:     %.10f\n", fabs(pi_estimate - M_PI));
        printf("\nCzasy działania poszczególnych procesów [s]:\n");
        
        double max_time = 0;
        for (int i = 0; i < size; i++) {
            printf("Proces %d: %.6f s\n", i, all_times[i]);
            if (all_times[i] > max_time) max_time = all_times[i];
        }
        printf("\nCałkowity czas (max): %.6f s\n", max_time);
        printf("---------------------------\n");

        free(all_times);
    }

    MPI_Finalize();
    return 0;
}