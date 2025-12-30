#ifndef FITNESS_KERNEL_H
#define FITNESS_KERNEL_H

#include <hls_stream.h>
#include <ap_int.h>

#define MAX_DIM 100
#define MAX_GENES 1000
#define BITS_PER_CHUNK 32
#define PARTIAL_UNROLL 10

typedef ap_uint<32> packed_t;

#ifdef __cplusplus
extern "C" {
#endif

void fitness_kernel(
    hls::stream<packed_t>& chromosome_stream,
    hls::stream<float>& result_stream,
    const float* vectors_in,
    int chromo_len,
    int dim,
    int num_bats,
    bool mode
);

#ifdef __cplusplus
}
#endif

#endif // FITNESS_KERNEL_H