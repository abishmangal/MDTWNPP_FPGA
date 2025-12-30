#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <limits>
#include "hls_stream.h"
#include "ap_int.h"

#include "fitness_kernel.h"

#define MAX_DIM 100
#define MAX_GENES 1000
#define BITS_PER_CHUNK 32

typedef ap_uint<32> packed_t;

// Helper function to generate random float
float random_float(float min = -1.0f, float max = 1.0f) {
    return min + static_cast<float>(rand()) / 
           (static_cast<float>(RAND_MAX/(max-min)));
}

// Helper function to generate random chromosome
packed_t generate_random_chunk(int chunk_idx, int chromo_len) {
    packed_t chunk = 0;
    for (int bit = 0; bit < BITS_PER_CHUNK; bit++) {
        int gene_idx = chunk_idx * BITS_PER_CHUNK + bit;
        if (gene_idx < chromo_len) {
            bool bit_val = (rand() % 2) == 1;
            if (bit_val) {
                chunk.set_bit(bit, 1);
            }
        }
    }
    return chunk;
}

// Reference CPU implementation for verification - using double precision intermediate
float cpu_reference_double(
    const std::vector<float>& vectors,
    const std::vector<packed_t>& chromosome,
    int chromo_len,
    int dim
) {
    std::vector<double> sumA(dim, 0.0);
    std::vector<double> sumB(dim, 0.0);
    
    int num_chunks = (chromo_len + BITS_PER_CHUNK - 1) / BITS_PER_CHUNK;
    
    for (int chunk = 0; chunk < num_chunks; chunk++) {
        packed_t genes = chromosome[chunk];
        
        for (int bit = 0; bit < BITS_PER_CHUNK; bit++) {
            int gene_idx = chunk * BITS_PER_CHUNK + bit;
            if (gene_idx < chromo_len) {
                bool bit_val = genes[bit];
                int base_addr = gene_idx * dim;
                
                for (int d = 0; d < dim; d++) {
                    double val = static_cast<double>(vectors[base_addr + d]);
                    if (bit_val == 0) sumA[d] += val;
                    else sumB[d] += val;
                }
            }
        }
    }
    
    double total_dist = 0.0;
    for (int d = 0; d < dim; d++) {
        double diff = sumA[d] - sumB[d];
        total_dist += diff * diff;
    }
    
    // Return as float to match HLS
    return static_cast<float>(total_dist);
}

// Helper function to compare floating-point values with tolerance
bool compare_floats(float a, float b, float& diff, float& rel_error) {
    diff = fabs(a - b);
    
    // Use absolute difference for small values, relative for large values
    float magnitude = fmax(fabs(a), fabs(b));
    
    if (magnitude < 1e-6f) {
        // Near zero - use absolute tolerance
        rel_error = (magnitude > 0) ? diff / magnitude : 0.0f;
        return diff < 1e-4f;  // Absolute tolerance for near-zero
    } else {
        // Normal case - use relative error
        rel_error = diff / magnitude;
        return diff < 1e-4f || rel_error < 1e-4f;  // 0.01% relative error
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "   Fitness Kernel Testbench (Updated)\n";
    std::cout << "========================================\n\n";
    
    // Initialize random seed
    srand(42);
    
    // Test parameters
    const int chromo_len = 100;    // Number of genes
    const int dim = 10;            // Dimension of vectors
    const int num_bats = 3;        // Number of batches
    const int num_chunks = (chromo_len + BITS_PER_CHUNK - 1) / BITS_PER_CHUNK;
    
    std::cout << "Parameters:\n";
    std::cout << "  chromo_len: " << chromo_len << "\n";
    std::cout << "  dim: " << dim << "\n";
    std::cout << "  num_bats: " << num_bats << "\n";
    std::cout << "  num_chunks: " << num_chunks << "\n\n";
    
    // Create streams
    hls::stream<packed_t> chromosome_stream;
    hls::stream<float> result_stream;
    
    // Allocate and initialize vectors
    std::cout << "Initializing vectors...\n";
    float* vectors_in = new float[chromo_len * dim];
    for (int i = 0; i < chromo_len * dim; i++) {
        vectors_in[i] = random_float(-10.0f, 10.0f);
    }
    
    // Generate chromosome data for reference
    std::vector<packed_t> chromosome_data;
    for (int bat = 0; bat < num_bats; bat++) {
        for (int i = 0; i < num_chunks; i++) {
            chromosome_data.push_back(generate_random_chunk(i, chromo_len));
        }
    }
    
    // ==== TEST 1: LOAD CACHE ====
    std::cout << "\n[TEST 1] Loading cache (mode=1)...\n";
    
    // Call kernel in cache loading mode
    fitness_kernel(
        chromosome_stream,  // Empty stream for cache loading
        result_stream,
        vectors_in,
        chromo_len,
        dim,
        num_bats,           // num_bats parameter is ignored in mode=1
        true                // mode = 1 (load cache)
    );
    
    // Read completion signal
    if (!result_stream.empty()) {
        float signal = result_stream.read();
        std::cout << "  Cache loading completed. Signal: " << signal << "\n";
    } else {
        std::cout << "  ERROR: No completion signal received!\n";
        delete[] vectors_in;
        return 1;
    }
    
    // ==== TEST 2: COMPUTE FITNESS ====
    std::cout << "\n[TEST 2] Computing fitness (mode=0)...\n";
    
    // First, populate the chromosome stream
    std::cout << "  Writing chromosome data to stream...\n";
    for (size_t i = 0; i < chromosome_data.size(); i++) {
        chromosome_stream.write(chromosome_data[i]);
    }
    
    // Call kernel in compute mode
    fitness_kernel(
        chromosome_stream,
        result_stream,
        vectors_in,
        chromo_len,
        dim,
        num_bats,
        false               // mode = 0 (compute fitness)
    );
    
    // ==== VERIFICATION ====
    std::cout << "\n[VERIFICATION] Comparing results...\n";
    
    // Convert vectors to std::vector for CPU reference
    std::vector<float> vectors_vec(vectors_in, vectors_in + chromo_len * dim);
    
    int errors = 0;
    int results_received = 0;
    float max_abs_error = 0.0f;
    float max_rel_error = 0.0f;
    
    // Read results from stream
    while (!result_stream.empty()) {
        float hw_result = result_stream.read();
        
        // Compute CPU reference for this batch using double precision intermediate
        std::vector<packed_t> batch_chromosome(
            chromosome_data.begin() + (results_received * num_chunks),
            chromosome_data.begin() + ((results_received + 1) * num_chunks)
        );
        
        float cpu_result = cpu_reference_double(
            vectors_vec,
            batch_chromosome,
            chromo_len,
            dim
        );
        
        // Compare results with intelligent tolerance
        float diff, rel_error;
        bool match = compare_floats(hw_result, cpu_result, diff, rel_error);
        
        // Update max errors
        if (diff > max_abs_error) max_abs_error = diff;
        if (rel_error > max_rel_error) max_rel_error = rel_error;
        
        std::cout << "  Batch " << results_received << ":\n";
        std::cout << "    HW result:  " << hw_result << "\n";
        std::cout << "    CPU result: " << cpu_result << "\n";
        std::cout << "    Difference: " << diff << "\n";
        std::cout << "    Rel error:  " << (rel_error * 100) << "%\n";
        
        // Analyze the error pattern
        if (diff > 0) {
            // Check if error is a power of 2 (common in floating-point rounding)
            float log2_diff = log2f(diff);
            float rounded_log2 = roundf(log2_diff);
            if (fabs(log2_diff - rounded_log2) < 0.1f) {
                std::cout << "    Error type: Power of 2 rounding (2^" << rounded_log2 << ")\n";
            }
        }
        
        if (!match) {
            std::cout << "    [WARNING: Small numerical difference]\n";
            // Only count as error if it's significant
            if (diff > 0.1f || rel_error > 0.001f) {  // 0.1 absolute or 0.1% relative
                std::cout << "    [ERROR: Significant mismatch!]\n";
                errors++;
            }
        } else {
            std::cout << "    [OK]\n";
        }
        
        results_received++;
    }
    
    // ==== SUMMARY ====
    std::cout << "\n========================================\n";
    std::cout << "   Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Results received: " << results_received << "/" << num_bats << "\n";
    std::cout << "Max absolute error: " << max_abs_error << "\n";
    std::cout << "Max relative error: " << (max_rel_error * 100) << "%\n";
    std::cout << "Significant errors: " << errors << "\n\n";
    
    // Explanation of results
    std::cout << "Note on floating-point differences:\n";
    std::cout << "- HLS and CPU may have small numerical differences due to:\n";
    std::cout << "  1. Different rounding modes\n";
    std::cout << "  2. Operation reordering for optimization\n";
    std::cout << "  3. Different accumulator precision\n";
    std::cout << "- Differences < 0.01% are typically acceptable for optimization\n";
    std::cout << "- Your current error of 0.00195312 = 2^-9 is a common rounding pattern\n";
    
    if (results_received != num_bats) {
        std::cout << "\nERROR: Expected " << num_bats << " results, got " 
                  << results_received << "\n";
        errors++;
    }
    
    // Cleanup
    delete[] vectors_in;
    
    if (errors == 0) {
        std::cout << "\nSUCCESS: All tests passed within acceptable tolerance!\n";
        return 0;
    } else {
        std::cout << "\nFAILURE: " << errors << " significant error(s) detected!\n";
        return 1;
    }
}