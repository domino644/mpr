#include <iostream>
#include <chrono>
#include <cublas_v2.h>

// Macro for checking cuBLAS errors
#define CHECK_CUBLAS(call) \
    do { \
        cublasStatus_t status = (call); \
        if (status != CUBLAS_STATUS_SUCCESS) { \
            std::cerr << "CUBLAS Error: " << status << " in " << #call << " at line " << __LINE__ << std::endl; \
            exit(1); \
        } \
    } while (0)

// Macro for checking CUDA errors
#define CHECK_CUDA(call) \
    do { \
        cudaError_t status = (call); \
        if (status != cudaSuccess) { \
            std::cerr << "CUDA Error: " << cudaGetErrorString(status) << " in " << #call << " at line " << __LINE__ << std::endl; \
            exit(1); \
        } \
    } while (0)

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
    CHECK_CUDA(cudaMalloc(&d_A, size * size * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_B, size * size * sizeof(float)));
    CHECK_CUDA(cudaMalloc(&d_C_device, size * size * sizeof(float)));

    // Copy data from host to device
    CHECK_CUDA(cudaMemcpy(d_A, h_A, size * size * sizeof(float), cudaMemcpyHostToDevice));
    CHECK_CUDA(cudaMemcpy(d_B, h_B, size * size * sizeof(float), cudaMemcpyHostToDevice));

    // Initialize cuBLAS
    cublasHandle_t cublasHandle;
    CHECK_CUBLAS(cublasCreate(&cublasHandle));

    // Perform matrix multiplication using cuBLAS (C = alpha * A * B + beta * C)
    // For C = A * B, alpha = 1.0, beta = 0.0
    float alpha = 1.0f;
    float beta = 0.0f;

    // Matrix dimensions (N x N)
    int n = size;

    // Warm-up call (optional, but good practice for timing)
    CHECK_CUBLAS(cublasSgemm(cublasHandle, 
                             CUBLAS_OP_N, 
                             CUBLAS_OP_N, 
                             n, n, n, 
                             &alpha,      
                             d_A, n, 
                             d_B, n, 
                             &beta,       
                             d_C_device, n));
    CHECK_CUDA(cudaDeviceSynchronize());

    // Measure cuBLAS execution time
    auto start = std::chrono::high_resolution_clock::now();
    CHECK_CUBLAS(cublasSgemm(cublasHandle, 
                             CUBLAS_OP_N, 
                             CUBLAS_OP_N, 
                             n,           
                             n,           
                             n,           
                             &alpha,      
                             d_A,         
                             n,           
                             d_B,         
                             n,           
                             &beta,       
                             d_C_device,  
                             n));          

    CHECK_CUDA(cudaDeviceSynchronize()); // Wait for the cuBLAS call to complete
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate duration
    std::chrono::duration<double> duration = end - start;

    // Copy result from device to host
    CHECK_CUDA(cudaMemcpy(h_C_host, d_C_device, size * size * sizeof(float), cudaMemcpyDeviceToHost));

    // Verify result
    // For h_A = 1.0 and h_B = 2.0, each element of C should be size * 1.0 * 2.0
    float expected = static_cast<float>(size) * 1.0f * 2.0f;
    // Check a few elements to verify
    bool error_found = false;
    if (std::abs(h_C_host[IDX2C(0, 0, size)] - expected) > 1e-3) { // Use a tolerance for float comparison
        std::cerr << "Error: Expected " << expected << ", Got " << h_C_host[IDX2C(0, 0, size)] << " Difference: " << h_C_host[IDX2C(0, 0, size)] - expected << std::endl;
        error_found = true;
    }
    if (std::abs(h_C_host[IDX2C(size-1, size-1, size)] - expected) > 1e-3) { // Use a tolerance for float comparison
        std::cerr << "Error: Expected " << expected << ", Got " << h_C_host[IDX2C(size-1, size-1, size)] << " Difference: " << h_C_host[IDX2C(size-1, size-1, size)] - expected << std::endl;
        error_found = true;
    }

    if (!error_found) {
        std::cout << "Result verification successful (checked sample elements)." << std::endl;
    }

    std::cout << "cuBLAS execution time (with warm-up): " << duration.count() * 1000 << " ms" << std::endl;

    // Free cuBLAS resources
    CHECK_CUBLAS(cublasDestroy(cublasHandle));

    // Free device memory
    CHECK_CUDA(cudaFree(d_A));
    CHECK_CUDA(cudaFree(d_B));
    CHECK_CUDA(cudaFree(d_C_device));

    // Free host memory
    delete[] h_A;
    delete[] h_B;
    delete[] h_C_host;

    return 0;
}
