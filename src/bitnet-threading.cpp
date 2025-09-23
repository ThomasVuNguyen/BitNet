#include "bitnet-threading.h"
#include <iostream>
#include <algorithm>
#include <unistd.h>

// Global thread pool instance
std::unique_ptr<BitNetThreadPool> g_bitnet_thread_pool = nullptr;

void bitnet_threading_init() {
    if (g_bitnet_thread_pool == nullptr) {
        g_bitnet_thread_pool = std::make_unique<BitNetThreadPool>();
        std::cout << "BitNet threading initialized with " 
                  << std::thread::hardware_concurrency() << " threads" << std::endl;
    }
}

void bitnet_threading_cleanup() {
    if (g_bitnet_thread_pool != nullptr) {
        g_bitnet_thread_pool.reset();
        std::cout << "BitNet threading cleaned up" << std::endl;
    }
}

int bitnet_get_optimal_thread_count() {
    // For Raspberry Pi 5, we want to use all 4 cores
    int hw_threads = std::thread::hardware_concurrency();
    
    // Cap at 4 for Pi 5, but allow fewer if system has less
    int optimal = std::min(4, hw_threads);
    
    // Don't use more threads than we have cores
    optimal = std::min(optimal, hw_threads);
    
    return std::max(1, optimal);  // At least 1 thread
}

// Specialized threading functions for BitNet operations
namespace BitNetThreading {

// Threaded matrix multiplication for LUT kernels
template<typename KernelFunc>
void parallel_lut_kernel(int m, int k, 
                        void* A, void* LUT, void* Scales, void* LUT_Scales, void* C,
                        KernelFunc kernel_func) {
    
    if (g_bitnet_thread_pool == nullptr) {
        bitnet_threading_init();
    }
    
    int num_threads = bitnet_get_optimal_thread_count();
    
    // For small matrices, use single thread
    if (m < 512 || k < 512) {
        kernel_func(m, k, A, LUT, Scales, LUT_Scales, C);
        return;
    }
    
    // Calculate optimal tile size for Pi 5 cache hierarchy
    // L1 cache: 32KB, L2 cache: 512KB per core
    int tile_size = 64;  // Start with 64x64 tiles
    
    // Adjust tile size based on matrix dimensions
    if (m > 2048) tile_size = 128;
    if (m > 4096) tile_size = 256;
    
    TileDistributor distributor(m, 1, tile_size, num_threads);  // Column dimension is 1 for LUT
    ProgressTracker progress(distributor.total_tiles());
    
    // Process tiles in parallel
    for (int t = 0; t < num_threads; ++t) {
        g_bitnet_thread_pool->enqueue([&, t]() {
            MatrixTile tile(0, 0, 0, 0, 0);
            
            while (distributor.get_next_tile(tile)) {
                // Prefetch data for this tile
                int tile_rows = tile.end_row - tile.start_row;
                
                // Prefetch input data
                prefetch_for_read((char*)A + tile.start_row * k / 8);  // A is uint8_t, 8 bits per element
                prefetch_for_read((char*)LUT + tile.start_row * k * 16);  // LUT is int8_t
                prefetch_for_write((char*)C + tile.start_row * sizeof(float));
                
                // Create temporary output buffer for this tile
                float* tile_output = aligned_alloc<float>(tile_rows);
                if (tile_output == nullptr) {
                    // Fallback to stack allocation
                    float stack_output[256];
                    tile_output = stack_output;
                }
                
                // Process this tile
                kernel_func(tile_rows, k, 
                           (char*)A + tile.start_row * k / 8,
                           (char*)LUT + tile.start_row * k * 16,
                           Scales, LUT_Scales, tile_output);
                
                // Copy result back to main output
                memcpy((char*)C + tile.start_row * sizeof(float), 
                       tile_output, tile_rows * sizeof(float));
                
                // Free temporary buffer if allocated
                if (tile_output != nullptr && tile_rows > 256) {
                    free(tile_output);
                }
                
                progress.mark_completed();
            }
        });
    }
    
    // Wait for all threads to complete
    g_bitnet_thread_pool->wait_all();
}

// Threaded preprocessor for LUT construction
template<typename PreprocessFunc>
void parallel_preprocess(int m, int k,
                        void* B, void* LUT_Scales, void* QLUT,
                        PreprocessFunc preprocess_func) {
    
    if (g_bitnet_thread_pool == nullptr) {
        bitnet_threading_init();
    }
    
    int num_threads = bitnet_get_optimal_thread_count();
    
    // For small matrices, use single thread
    if (m < 1024 || k < 1024) {
        preprocess_func(m, k, B, LUT_Scales, QLUT);
        return;
    }
    
    // Split work by K dimension for preprocessing
    int k_tile_size = k / num_threads;
    if (k_tile_size < 64) k_tile_size = 64;  // Minimum tile size
    
    for (int t = 0; t < num_threads; ++t) {
        int start_k = t * k_tile_size;
        int end_k = std::min(start_k + k_tile_size, k);
        
        if (start_k >= end_k) continue;
        
        g_bitnet_thread_pool->enqueue([=]() {
            // Prefetch data
            prefetch_for_read((char*)B + start_k * m * sizeof(float));
            prefetch_for_write((char*)QLUT + start_k * m * 2);  // 2 bits per element
            
            // Process this K slice
            preprocess_func(m, end_k - start_k, 
                           (char*)B + start_k * m * sizeof(float),
                           LUT_Scales,
                           (char*)QLUT + start_k * m * 2);
        });
    }
    
    g_bitnet_thread_pool->wait_all();
}

} // namespace BitNetThreading
