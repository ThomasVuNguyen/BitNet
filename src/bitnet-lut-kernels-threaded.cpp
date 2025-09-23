#include "bitnet-lut-kernels-threaded.h"
#include <cstring>
#include <algorithm>

// Threaded version of qgemm_lut_3200_8640
int32_t qgemm_lut_3200_8640_threaded(void* A, void* LUT, void* Scales, void* LUT_Scales, void* C) {
    if (g_bitnet_thread_pool == nullptr) {
        bitnet_threading_init();
    }
    
    const int BM = 160;  // BM3200_8640
    const int BK = 64;   // BBK3200_8640
    const int total_k_blocks = 8640 / BK;
    
    // For small matrices, use single-threaded version
    if (total_k_blocks <= 2) {
        return qgemm_lut_3200_8640(A, LUT, Scales, LUT_Scales, C);
    }
    
    // Allocate thread-safe output buffer
    alignas(32) uint32_t* CBits = aligned_alloc<uint32_t>(BM);
    if (CBits == nullptr) {
        // Fallback to stack allocation
        alignas(32) uint32_t stack_CBits[BM];
        CBits = stack_CBits;
    }
    
    memset(CBits, 0, BM * sizeof(uint32_t));
    
    int num_threads = bitnet_get_optimal_thread_count();
    int k_blocks_per_thread = (total_k_blocks + num_threads - 1) / num_threads;
    
    // Process K blocks in parallel
    for (int t = 0; t < num_threads; ++t) {
        int start_k = t * k_blocks_per_thread;
        int end_k = std::min(start_k + k_blocks_per_thread, total_k_blocks);
        
        if (start_k >= end_k) continue;
        
        g_bitnet_thread_pool->enqueue([=]() {
            // Each thread processes a range of K blocks
            for (int32_t k_outer = start_k; k_outer < end_k; ++k_outer) {
                // Calculate offsets for this K block
                int lut_offset = k_outer * BK / 2 * 32;
                int a_offset = k_outer * BK / 2 / 2 * BM;
                
                // Process this K block
                tbl_impl_3200_8640(CBits, 
                                  (int8_t*)LUT + lut_offset,
                                  (uint8_t*)A + a_offset);
            }
        });
    }
    
    g_bitnet_thread_pool->wait_all();
    
    // Final scaling (single-threaded, as it's fast)
    for (int i = 0; i < BM; i++) {
        ((bitnet_float_type*)C)[i] = ((int32_t*)CBits)[i] / 
                                    ((bitnet_float_type*)LUT_Scales)[0] * 
                                    ((bitnet_float_type*)Scales)[0];
    }
    
    if (CBits != nullptr && BM > 160) {
        free(CBits);
    }
    
    return 0;
}

// Threaded version of qgemm_lut_3200_3200
int32_t qgemm_lut_3200_3200_threaded(void* A, void* LUT, void* Scales, void* LUT_Scales, void* C) {
    if (g_bitnet_thread_pool == nullptr) {
        bitnet_threading_init();
    }
    
    const int BM = 160;  // BM3200_3200
    const int BK = 128;  // BBK3200_3200
    const int total_k_blocks = 3200 / BK;
    
    // For small matrices, use single-threaded version
    if (total_k_blocks <= 2) {
        return qgemm_lut_3200_3200(A, LUT, Scales, LUT_Scales, C);
    }
    
    // Allocate thread-safe output buffer
    alignas(32) uint32_t* CBits = aligned_alloc<uint32_t>(BM);
    if (CBits == nullptr) {
        alignas(32) uint32_t stack_CBits[BM];
        CBits = stack_CBits;
    }
    
    memset(CBits, 0, BM * sizeof(uint32_t));
    
    int num_threads = bitnet_get_optimal_thread_count();
    int k_blocks_per_thread = (total_k_blocks + num_threads - 1) / num_threads;
    
    // Process K blocks in parallel
    for (int t = 0; t < num_threads; ++t) {
        int start_k = t * k_blocks_per_thread;
        int end_k = std::min(start_k + k_blocks_per_thread, total_k_blocks);
        
        if (start_k >= end_k) continue;
        
        g_bitnet_thread_pool->enqueue([=]() {
            for (int32_t k_outer = start_k; k_outer < end_k; ++k_outer) {
                int lut_offset = k_outer * BK / 2 * 32;
                int a_offset = k_outer * BK / 2 / 2 * BM;
                
                tbl_impl_3200_3200(CBits, 
                                  (int8_t*)LUT + lut_offset,
                                  (uint8_t*)A + a_offset);
            }
        });
    }
    
    g_bitnet_thread_pool->wait_all();
    
    // Final scaling
    for (int i = 0; i < BM; i++) {
        ((bitnet_float_type*)C)[i] = ((int32_t*)CBits)[i] / 
                                    ((bitnet_float_type*)LUT_Scales)[0] * 
                                    ((bitnet_float_type*)Scales)[0];
    }
    
    if (CBits != nullptr && BM > 160) {
        free(CBits);
    }
    
    return 0;
}

// Threaded version of qgemm_lut_8640_3200
int32_t qgemm_lut_8640_3200_threaded(void* A, void* LUT, void* Scales, void* LUT_Scales, void* C) {
    if (g_bitnet_thread_pool == nullptr) {
        bitnet_threading_init();
    }
    
    const int BM = 320;  // BM8640_3200
    const int BK = 64;   // BBK8640_3200
    const int total_k_blocks = 3200 / BK;
    
    // For small matrices, use single-threaded version
    if (total_k_blocks <= 2) {
        return qgemm_lut_8640_3200(A, LUT, Scales, LUT_Scales, C);
    }
    
    // Allocate thread-safe output buffer
    alignas(32) uint32_t* CBits = aligned_alloc<uint32_t>(BM);
    if (CBits == nullptr) {
        alignas(32) uint32_t stack_CBits[BM];
        CBits = stack_CBits;
    }
    
    memset(CBits, 0, BM * sizeof(uint32_t));
    
    int num_threads = bitnet_get_optimal_thread_count();
    int k_blocks_per_thread = (total_k_blocks + num_threads - 1) / num_threads;
    
    // Process K blocks in parallel
    for (int t = 0; t < num_threads; ++t) {
        int start_k = t * k_blocks_per_thread;
        int end_k = std::min(start_k + k_blocks_per_thread, total_k_blocks);
        
        if (start_k >= end_k) continue;
        
        g_bitnet_thread_pool->enqueue([=]() {
            for (int32_t k_outer = start_k; k_outer < end_k; ++k_outer) {
                int lut_offset = k_outer * BK / 2 * 32;
                int a_offset = k_outer * BK / 2 / 2 * BM;
                
                tbl_impl_8640_3200(CBits, 
                                  (int8_t*)LUT + lut_offset,
                                  (uint8_t*)A + a_offset);
            }
        });
    }
    
    g_bitnet_thread_pool->wait_all();
    
    // Final scaling
    for (int i = 0; i < BM; i++) {
        ((bitnet_float_type*)C)[i] = ((int32_t*)CBits)[i] / 
                                    ((bitnet_float_type*)LUT_Scales)[0] * 
                                    ((bitnet_float_type*)Scales)[0];
    }
    
    if (CBits != nullptr && BM > 320) {
        free(CBits);
    }
    
    return 0;
}

// Threaded preprocessor
void ggml_preprocessor_threaded(int m, int k, void* B, void* LUT_Scales, void* QLUT) {
    if (g_bitnet_thread_pool == nullptr) {
        bitnet_threading_init();
    }
    
    // For small matrices, use single-threaded version
    if (m < 1024 || k < 1024) {
        ggml_preprocessor(m, k, B, LUT_Scales, QLUT);
        return;
    }
    
    // Split work by K dimension for preprocessing
    int num_threads = bitnet_get_optimal_thread_count();
    int k_tile_size = k / num_threads;
    if (k_tile_size < 64) k_tile_size = 64;
    
    for (int t = 0; t < num_threads; ++t) {
        int start_k = t * k_tile_size;
        int end_k = std::min(start_k + k_tile_size, k);
        
        if (start_k >= end_k) continue;
        
        g_bitnet_thread_pool->enqueue([=]() {
            // Process this K slice
            if (m == 3200 && k == 8640) {
                preprocessor_k<8640>((char*)B + start_k * m * sizeof(bitnet_float_type), 
                                   LUT_Scales, 
                                   (char*)QLUT + start_k * m * 2);
            }
            else if (m == 3200 && k == 3200) {
                preprocessor_k<3200>((char*)B + start_k * m * sizeof(bitnet_float_type), 
                                   LUT_Scales, 
                                   (char*)QLUT + start_k * m * 2);
            }
            else if (m == 8640 && k == 3200) {
                preprocessor_k<3200>((char*)B + start_k * m * sizeof(bitnet_float_type), 
                                   LUT_Scales, 
                                   (char*)QLUT + start_k * m * 2);
            }
        });
    }
    
    g_bitnet_thread_pool->wait_all();
}

// Main threaded dispatch function
void ggml_qgemm_lut_threaded(int m, int k, void* A, void* LUT, void* Scales, void* LUT_Scales, void* C) {
    if (m == 3200 && k == 8640) {
        qgemm_lut_3200_8640_threaded(A, LUT, Scales, LUT_Scales, C);
    }
    else if (m == 3200 && k == 3200) {
        qgemm_lut_3200_3200_threaded(A, LUT, Scales, LUT_Scales, C);
    }
    else if (m == 8640 && k == 3200) {
        qgemm_lut_8640_3200_threaded(A, LUT, Scales, LUT_Scales, C);
    }
    else {
        // Fallback to single-threaded version
        ggml_qgemm_lut(m, k, A, LUT, Scales, LUT_Scales, C);
    }
}

// Threaded matrix multiplication with automatic kernel selection
void ggml_bitnet_mul_mat_threaded(void* src0, void* scales, void* qlut, void* lut_scales, 
                                 void* lut_biases, void* dst, int n, int k, int m, int bits) {
    if (g_bitnet_thread_pool == nullptr) {
        bitnet_threading_init();
    }
    
    // Use threaded LUT kernels
    ggml_qgemm_lut_threaded(m, k, src0, qlut, scales, lut_scales, dst);
}
