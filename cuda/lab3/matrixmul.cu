#include <iostream>
#include <chrono>

// CUDA kernel for matrix multiplication
__global__ void matrixMul(float *A, float *B, float *C, int size) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < size && col < size) {
        float sum = 0.0f;
        for (int i = 0; i < size; ++i) {
            sum += A[row * size + i] * B[i * size + col];
        }
        C[row * size + col] = sum;
    }
}

int main() {
    int size = 1024; // Example matrix size
    // Allocate host memory
    float *h_A, *h_B, *h_C;
    h_A = new float[size * size];
    h_B = new float[size * size];
    h_C = new float[size * size];

    // Initialize host matrices
    for (int i = 0; i < size * size; ++i) {
        h_A[i] = 1.0f;
        h_B[i] = 2.0f;
    }

    // Allocate device memory
    float *d_A, *d_B, *d_C;
    cudaMalloc(&d_A, size * size * sizeof(float));
    cudaMalloc(&d_B, size * size * sizeof(float));
    cudaMalloc(&d_C, size * size * sizeof(float));

    // Copy data from host to device
    cudaMemcpy(d_A, h_A, size * size * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size * size * sizeof(float), cudaMemcpyHostToDevice);

    // Define grid and block dimensions
    int blockSize = 16;
    dim3 blockDim(blockSize, blockSize);
    dim3 gridDim((size + blockSize - 1) / blockSize, (size + blockSize - 1) / blockSize);

    // Measure kernel execution time
    auto start = std::chrono::high_resolution_clock::now();
    matrixMul<<<gridDim, blockDim>>>(d_A, d_B, d_C, size);
    cudaDeviceSynchronize(); // Wait for the kernel to finish
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate duration
    std::chrono::duration<double> duration = end - start;

    // Copy result from device to host
    cudaMemcpy(h_C, d_C, size * size * sizeof(float), cudaMemcpyDeviceToHost);

    // Verify result (optional, for debugging)
    // float expected = size * 1.0f * 2.0f;
    // if (h_C[0] != expected) {
    //     std::cerr << "Error: " << h_C[0] - expected << std::endl;
    // }

    std::cout << "Kernel execution time: " << duration.count() * 1000 << " ms" << std::endl;

    // Free device memory
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    // Free host memory
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;

    return 0;
}
