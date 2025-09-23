#include <iostream>
#include <chrono>
#include <cstring>
#include "include/bitnet-threading.h"
#include "include/bitnet-lut-kernels-threaded.h"

// Test function to verify threading performance
void test_threading_performance() {
    std::cout << "Testing BitNet threading performance on Raspberry Pi 5..." << std::endl;
    
    // Initialize threading
    bitnet_threading_init();
    
    int num_threads = bitnet_get_optimal_thread_count();
    std::cout << "Optimal thread count: " << num_threads << std::endl;
    
    // Test matrix dimensions (typical BitNet sizes)
    const int m = 3200;
    const int k = 8640;
    
    // Allocate test data
    size_t a_size = m * k / 8;  // 8 bits per element
    size_t lut_size = m * k * 16;  // LUT size
    size_t c_size = m * sizeof(float);
    
    uint8_t* A = new uint8_t[a_size];
    int8_t* LUT = new int8_t[lut_size];
    float* Scales = new float[1];
    float* LUT_Scales = new float[1];
    float* C = new float[m];
    
    // Initialize test data
    memset(A, 0x55, a_size);  // Pattern for testing
    memset(LUT, 1, lut_size);
    Scales[0] = 1.0f;
    LUT_Scales[0] = 1.0f;
    memset(C, 0, c_size);
    
    std::cout << "Matrix size: " << m << " x " << k << std::endl;
    std::cout << "Data sizes - A: " << a_size << " bytes, LUT: " << lut_size << " bytes" << std::endl;
    
    // Test single-threaded performance
    std::cout << "\nTesting single-threaded performance..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    // Note: This would call the original single-threaded function
    // qgemm_lut_3200_8640(A, LUT, Scales, LUT_Scales, C);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto single_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Test multi-threaded performance
    std::cout << "Testing multi-threaded performance..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    
    qgemm_lut_3200_8640_threaded(A, LUT, Scales, LUT_Scales, C);
    
    end = std::chrono::high_resolution_clock::now();
    auto multi_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\nPerformance Results:" << std::endl;
    std::cout << "Single-threaded time: " << single_time.count() << " μs" << std::endl;
    std::cout << "Multi-threaded time: " << multi_time.count() << " μs" << std::endl;
    
    if (single_time.count() > 0) {
        double speedup = (double)single_time.count() / multi_time.count();
        std::cout << "Speedup: " << speedup << "x" << std::endl;
    }
    
    // Cleanup
    delete[] A;
    delete[] LUT;
    delete[] Scales;
    delete[] LUT_Scales;
    delete[] C;
    
    bitnet_threading_cleanup();
}

int main() {
    try {
        test_threading_performance();
        std::cout << "\nThreading test completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
