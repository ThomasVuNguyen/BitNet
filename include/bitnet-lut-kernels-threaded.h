#pragma once

#include "bitnet-lut-kernels.h"
#include "bitnet-threading.h"

#ifdef __cplusplus
extern "C" {
#endif

// Threaded versions of the LUT kernels
int32_t qgemm_lut_3200_8640_threaded(void* A, void* LUT, void* Scales, void* LUT_Scales, void* C);
int32_t qgemm_lut_3200_3200_threaded(void* A, void* LUT, void* Scales, void* LUT_Scales, void* C);
int32_t qgemm_lut_8640_3200_threaded(void* A, void* LUT, void* Scales, void* LUT_Scales, void* C);

// Threaded preprocessor functions
void ggml_preprocessor_threaded(int m, int k, void* B, void* LUT_Scales, void* QLUT);

// Main threaded dispatch function
void ggml_qgemm_lut_threaded(int m, int k, void* A, void* LUT, void* Scales, void* LUT_Scales, void* C);

// Threaded matrix multiplication with automatic kernel selection
void ggml_bitnet_mul_mat_threaded(void* src0, void* scales, void* qlut, void* lut_scales, 
                                 void* lut_biases, void* dst, int n, int k, int m, int bits);

#ifdef __cplusplus
}
#endif
