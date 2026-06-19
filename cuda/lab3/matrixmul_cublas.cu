#include <iostream>
#include <chrono>
#include <cublas_v2.h>

#define IDX2C(i, j, ld) (((j)*(ld))+(i)) // Column-major indexing

int main() {
    int size = 1024; // Example matrix size (N x N)
    // Allocate host memory
    float *h_A, *h_B, *h_C_host;
    h_A = new float[size * size];
    h_B = new float[size * size];
    h_C_host = new float[size * size];

    // Initialize host matrices (column-major for cuBLAS)
    for (int j = 0; j < size; ++j) {
        for (int i = 0; i < size; ++i) {
            h_A[IDX2C(i, j, size)] = 1.0f;
            h_B[IDX2C(i, j, size)] = 2.0f;
        }
    }

    // Allocate device memory
    float *d_A, *d_B, *d_C_device;
    cudaMalloc(&d_A, size * size * sizeof(float));
    cudaMalloc(&d_B, size * size * sizeof(float));
    cudaMalloc(&d_C_device, size * size * sizeof(float));

    // Copy data from host to device
    cudaMemcpy(d_A, h_A, size * size * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size * size * sizeof(float), cudaMemcpyHostToDevice);

    // Initialize cuBLAS
    cublasHandle_t cublasHandle;
    cublasCreate(&cublasHandle);

    // Perform matrix multiplication using cuBLAS (C = alpha * A * B + beta * C)
    // For C = A * B, alpha = 1.0, beta = 0.0
    float alpha = 1.0f;
    float beta = 0.0f;

    // Matrix dimensions (N x N)
    int n = size;

    // Measure cuBLAS execution time
    auto start = std::chrono::high_resolution_clock::now();
    // cublasSgemm(handle, transa, transb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc)
    // For C = A * B, where A, B, C are N x N matrices
    // m = N, n = N, k = N
    // lda = N, ldb = N, ldc = N
    // A and B are in column-major order.
    cublasSgemm(cublasHandle, 
                CUBLAS_OP_N, // A is not transposed
                CUBLAS_OP_N, // B is not transposed
                n,           // m: number of rows of A and C
                n,           // n: number of columns of B and C
                n,           // k: number of columns of A and rows of B
                &alpha,      // scalar alpha
                d_A,         // A matrix on device
                n,           // lda: leading dimension of A
                d_B,         // B matrix on device
                n,           // ldb: leading dimension of B
                &beta,       // scalar beta
                d_C_device,  // C matrix on device
                n);          // ldc: leading dimension of C

    cudaDeviceSynchronize(); // Wait for the cuBLAS call to complete
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate duration
    std::chrono::duration<double> duration = end - start;

    // Copy result from device to host
    cudaMemcpy(h_C_host, d_C_device, size * size * sizeof(float), cudaMemcpyDeviceToHost);

    // Verify result
    // For h_A = 1.0 and h_B = 2.0, each element of C should be size * 1.0 * 2.0
    float expected = static_cast<float>(size) * 1.0f * 2.0f;
    // Check a few elements to verify
    bool error_found = false;
    if (h_C_host[IDX2C(0, 0, size)] != expected) {
        std::cerr << "Error: Expected " << expected << ", Got " << h_C_host[IDX2C(0, 0, size)] << " Difference: " << h_C_host[IDX2C(0, 0, size)] - expected << std::endl;
        error_found = true;
    }
    if (h_C_host[IDX2C(size-1, size-1, size)] != expected) {
        std::cerr << "Error: Expected " << expected << ", Got " << h_C_host[IDX2C(size-1, size-1, size)] << " Difference: " << h_C_host[IDX2C(size-1, size-1, size)] - expected << std::endl;
        error_found = true;
    }

    if (!error_found) {
        std::cout << "Result verification successful (checked sample elements)." << std::endl;
    }

    std::cout << "cuBLAS execution time: " << duration.count() * 1000 << " ms" << std::endl;

    // Free cuBLAS resources
    cublasDestroy(cublasHandle);

    // Free device memory
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C_device);

    // Free host memory
    delete[] h_A;
    delete[] h_B;
    delete[] h_C_host;

    return 0;
}
