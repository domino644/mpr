#include <iostream>
#include <chrono>

// CUDA kernel for matrix multiplication using shared memory
__global__ void matrixMul(float *A, float *B, float *C, int size) {
    // Thread index within block
    int tx = threadIdx.x;
    int ty = threadIdx.y;

    // Global row and column index
    int row = blockIdx.y * blockDim.y + ty;
    int col = blockIdx.x * blockDim.x + tx;

    // Define shared memory for block A and B
    // Block size must be known at compile time for shared memory allocation
    // For this example, we assume blockDim.x and blockDim.y are equal to `TILE_DIM`
    extern __shared__ float s_A[]; // TILE_DIM * TILE_DIM
    extern __shared__ float s_B[]; // TILE_DIM * TILE_DIM

    float Cvalue = 0;

    // Loop over the tiles of the input matrices
    // 'size' is the dimension of the square matrices
    int TILE_DIM = blockDim.x; // Assuming square blocks

    for (int m = 0; m < (size + TILE_DIM - 1) / TILE_DIM; ++m) {
        // Load a tile of A and B into shared memory
        int aRow = row;
        int aCol = m * TILE_DIM + tx;
        int bRow = m * TILE_DIM + ty;
        int bCol = col;

        // Boundary check for loading into shared memory
        if (aRow < size && aCol < size) {
            s_A[ty * TILE_DIM + tx] = A[aRow * size + aCol];
        } else {
            s_A[ty * TILE_DIM + tx] = 0.0f; // Pad with zeros if out of bounds
        }

        if (bRow < size && bCol < size) {
            s_B[ty * TILE_DIM + tx] = B[bRow * size + bCol];
        } else {
            s_B[ty * TILE_DIM + tx] = 0.0f; // Pad with zeros if out of bounds
        }

        __syncthreads(); // Synchronize all threads in the block to ensure tile is loaded

        // Perform the matrix multiplication for the current tiles
        for (int k = 0; k < TILE_DIM; ++k) {
            Cvalue += s_A[ty * TILE_DIM + k] * s_B[k * TILE_DIM + tx];
        }

        __syncthreads(); // Synchronize again to ensure shared memory is flushed before next tile load
    }

    // Store the final result in global memory
    if (row < size && col < size) {
        C[row * size + col] = Cvalue;
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
    int blockSize = 32; // Changed block size for shared memory optimization
    dim3 blockDim(blockSize, blockSize);
    dim3 gridDim((size + blockSize - 1) / blockSize, (size + blockSize - 1) / blockSize);

    // Calculate shared memory size needed for the kernel
    // Each shared memory array holds TILE_DIM * TILE_DIM floats.
    // Since we have two (s_A and s_B), it's 2 * TILE_DIM * TILE_DIM * sizeof(float).
    int sharedMemSize = 2 * blockSize * blockSize * sizeof(float);

    // Measure kernel execution time
    auto start = std::chrono::high_resolution_clock::now();
    matrixMul<<<gridDim, blockDim, sharedMemSize>>>(d_A, d_B, d_C, size);
    cudaDeviceSynchronize(); // Wait for the kernel to finish
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate duration
    std::chrono::duration<double> duration = end - start;

    // Copy result from device to host
    cudaMemcpy(h_C, d_C, size * size * sizeof(float), cudaMemcpyDeviceToHost);

    // Verify result (optional, for debugging)
    // For this example, with h_A = 1.0 and h_B = 2.0, each element of C should be size * 1.0 * 2.0
    //float expected = static_cast<float>(size) * 1.0f * 2.0f;
    //if (h_C[0] != expected) {
    //     std::cerr << "Error: Expected " << expected << ", Got " << h_C[0] << " Difference: " << h_C[0] - expected << std::endl;
    //}

    std::cout << "Kernel execution time (with shared memory): " << duration.count() * 1000 << " ms" << std::endl;

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
