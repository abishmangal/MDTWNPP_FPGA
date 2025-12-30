#include <hls_stream.h>
#include <ap_int.h>
#include <hls_math.h>
#include <stdint.h>

#define MAX_CHROMO 500
#define MAX_DIM    20

extern "C" {

/* Your original kernel */
void fitness_kernel(
    hls::stream<uint8_t> &chromosome_stream,
    hls::stream<float>   &result_stream,
    const float          *vectors,
    int                  chromo_len,
    int                  dim
);

/* ================= TOP FUNCTION ================= */
extern "C" void fitness_top(
    hls::stream<uint8_t> &chromosome_stream,
    hls::stream<float>   &result_stream,
    const float          *vectors,
    int                  chromo_len,
    int                  dim
) {
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS INTERFACE axis port=chromosome_stream
#pragma HLS INTERFACE axis port=result_stream

#pragma HLS INTERFACE m_axi     port=vectors offset=slave bundle=gmem_vec depth=10000
#pragma HLS INTERFACE s_axilite port=vectors    bundle=control
#pragma HLS INTERFACE s_axilite port=chromo_len bundle=control
#pragma HLS INTERFACE s_axilite port=dim        bundle=control

    /* Call your kernel */
    fitness_kernel(chromosome_stream, result_stream, vectors, chromo_len, dim);
}

} // extern "C"
