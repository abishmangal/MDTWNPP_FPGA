#include <hls_stream.h>
#include <ap_int.h>
#include <hls_math.h>
#include "fitness_kernel.h"

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
) {
    // --- INTERFACES ---
    #pragma HLS INTERFACE axis port=chromosome_stream
    #pragma HLS INTERFACE axis port=result_stream
    #pragma HLS INTERFACE m_axi port=vectors_in offset=slave bundle=gmem_vec depth=MAX_GENES*MAX_DIM
    #pragma HLS INTERFACE s_axilite port=chromo_len bundle=control
    #pragma HLS INTERFACE s_axilite port=dim bundle=control
    #pragma HLS INTERFACE s_axilite port=num_bats bundle=control
    #pragma HLS INTERFACE s_axilite port=mode bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    
    // --- LOCAL STORAGE ---
    static float local_vector_cache[MAX_GENES * MAX_DIM];
    #pragma HLS ARRAY_PARTITION variable=local_vector_cache cyclic factor=PARTIAL_UNROLL dim=1
    #pragma HLS BIND_STORAGE variable=local_vector_cache type=ram_2p impl=bram

    // --- MODE 1: LOAD CACHE ---
    if (mode == 1) {
        int total_elements = chromo_len * dim;
        
        load_cache: for (int i = 0; i < total_elements; i++) {
            #pragma HLS PIPELINE II=1
            local_vector_cache[i] = vectors_in[i];
        }
        
        result_stream.write(0.0f);
    } 
    // --- MODE 0: COMPUTE FITNESS ---
    else {
        // Fixed arrays with cyclic partitioning
        float sumA[MAX_DIM];
        float sumB[MAX_DIM];
        #pragma HLS ARRAY_PARTITION variable=sumA cyclic factor=PARTIAL_UNROLL
        #pragma HLS ARRAY_PARTITION variable=sumB cyclic factor=PARTIAL_UNROLL
        
        process_batches: for (int bat = 0; bat < num_bats; bat++) {
            #pragma HLS LOOP_TRIPCOUNT min=1 max=1000
            
            // Initialize sums
            init_sums: for (int i = 0; i < MAX_DIM; i++) {
                #pragma HLS PIPELINE II=1
                if (i < dim) {
                    sumA[i] = 0.0f;
                    sumB[i] = 0.0f;
                }
            }
            
            // --- FIX: Separate chromosome read loop ---
            const int num_chunks = (chromo_len + BITS_PER_CHUNK - 1) / BITS_PER_CHUNK;
            
            // Read all chromosome chunks first into a buffer
            packed_t chromo_buffer[(MAX_GENES + BITS_PER_CHUNK - 1) / BITS_PER_CHUNK];
            #pragma HLS ARRAY_PARTITION variable=chromo_buffer cyclic factor=4
            
            read_chromosomes: for (int chunk = 0; chunk < num_chunks; chunk++) {
                #pragma HLS PIPELINE II=1
                chromo_buffer[chunk] = chromosome_stream.read();
            }
            
            // Process genes using the buffered chromosome data
            process_genes: for (int gene_idx = 0; gene_idx < chromo_len; gene_idx++) {
                #pragma HLS PIPELINE II=1
                
                int chunk_idx = gene_idx / BITS_PER_CHUNK;
                int bit_idx = gene_idx % BITS_PER_CHUNK;
                bool gene_bit = chromo_buffer[chunk_idx][bit_idx];
                int vector_base = gene_idx * dim;
                
                // Process dimensions with partial unroll - NO PIPELINE pragma inside
                process_dims: for (int d_block = 0; d_block < dim; d_block += PARTIAL_UNROLL) {
                    // Remove #pragma HLS PIPELINE from here - only partial unroll
                    int d_end = d_block + PARTIAL_UNROLL;
                    if (d_end > dim) d_end = dim;
                    
                    for (int d = d_block; d < d_end; d++) {
                        #pragma HLS UNROLL
                        float vector_val = local_vector_cache[vector_base + d];
                        
                        if (gene_bit == 0) {
                            // Use standard floating-point addition
                            float temp_sum = sumA[d];
                            sumA[d] = temp_sum + vector_val;
                        } else {
                            // Use standard floating-point addition
                            float temp_sum = sumB[d];
                            sumB[d] = temp_sum + vector_val;
                        }
                    }
                }
            }
            
            // --- Compute Euclidean distance with hierarchical reduction ---
            float distance_squared = 0.0f;
            
            // Use hierarchical reduction to avoid large unrolls
            const int REDUCTION_GROUPS = 4;  // Adjust based on your needs
            float group_sums[REDUCTION_GROUPS];
            #pragma HLS ARRAY_PARTITION variable=group_sums complete
            
            // Initialize group sums
            init_groups: for (int g = 0; g < REDUCTION_GROUPS; g++) {
                #pragma HLS UNROLL
                group_sums[g] = 0.0f;
            }
            
            // Compute groups - pipeline only, avoid full unroll
            compute_groups: for (int d = 0; d < dim; d++) {
                #pragma HLS PIPELINE II=1
                int group_idx = d % REDUCTION_GROUPS;  // Distribute across groups
                // Use standard floating-point subtraction and multiplication
                float diff = sumA[d] - sumB[d];
                float square = diff * diff;
                float temp_sum = group_sums[group_idx];
                group_sums[group_idx] = temp_sum + square;
            }
            
            // Accumulate groups - small enough to unroll
            accumulate_groups: for (int g = 0; g < REDUCTION_GROUPS; g++) {
                #pragma HLS UNROLL
                float temp_distance = distance_squared;
                distance_squared = temp_distance + group_sums[g];
            }
            
            // Alternative: If REDUCTION_GROUPS is too large, use this instead:
            // accumulate_groups_seq: for (int g = 0; g < REDUCTION_GROUPS; g++) {
            //     #pragma HLS PIPELINE II=1
            //     distance_squared += group_sums[g];
            // }
            
            result_stream.write(distance_squared);
        }
    }
}

#ifdef __cplusplus
}
#endif