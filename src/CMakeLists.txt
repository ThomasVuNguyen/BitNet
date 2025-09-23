set(GGML_HEADERS_BITNET ../include/ggml-bitnet.h)
set(GGML_SOURCES_BITNET ggml-bitnet-mad.cpp)
set(GGML_SOURCES_BITNET ggml-bitnet-lut.cpp)

# Add threading support for Raspberry Pi 5
set(GGML_HEADERS_BITNET_THREADING ../include/bitnet-threading.h ../include/bitnet-lut-kernels-threaded.h)
set(GGML_SOURCES_BITNET_THREADING bitnet-threading.cpp bitnet-lut-kernels-threaded.cpp)

# Combine all BitNet sources
set(GGML_SOURCES_BITNET ${GGML_SOURCES_BITNET} ${GGML_SOURCES_BITNET_THREADING})
set(GGML_HEADERS_BITNET ${GGML_HEADERS_BITNET} ${GGML_HEADERS_BITNET_THREADING})

include_directories(3rdparty/llama.cpp/ggml/include)

if (NOT (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU") OR
    NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    message(FATAL_ERROR "Clang or GCC is required for Bitnet.cpp compilation")
endif()
