#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <vector>
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

// Reference CPU implementation for verification
float cpu_reference(
    const std::vector<float>& vectors,
    const std::vector<packed_t>& chromosome,
    int chromo_len,
    int dim,
    int batch_idx
) {
    std::vector<float> sumA(dim, 0.0f);
    std::vector<float> sumB(dim, 0.0f);
    
    int num_chunks = (chromo_len + BITS_PER_CHUNK - 1) / BITS_PER_CHUNK;
    
    for (int chunk = 0; chunk < num_chunks; chunk++) {
        packed_t genes = chromosome[chunk];
        
        for (int bit = 0; bit < BITS_PER_CHUNK; bit++) {
            int gene_idx = chunk * BITS_PER_CHUNK + bit;
            if (gene_idx < chromo_len) {
                bool bit_val = genes[bit];
                int base_addr = gene_idx * dim;
                
                for (int d = 0; d < dim; d++) {
                    float val = vectors[base_addr + d];
                    if (bit_val == 0) sumA[d] += val;
                    else sumB[d] += val;
                }
            }
        }
    }
    
    float total_dist = 0.0f;
    for (int d = 0; d < dim; d++) {
        float diff = sumA[d] - sumB[d];
        total_dist += diff * diff;
    }
    
    return total_dist;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "   Fitness Kernel Testbench\n";
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
    
    // Read results from stream
    while (!result_stream.empty()) {
        float hw_result = result_stream.read();
        
        // Compute CPU reference for this batch
        std::vector<packed_t> batch_chromosome(
            chromosome_data.begin() + (results_received * num_chunks),
            chromosome_data.begin() + ((results_received + 1) * num_chunks)
        );
        
        float cpu_result = cpu_reference(
            vectors_vec,
            batch_chromosome,
            chromo_len,
            dim,
            results_received
        );
        
        // Compare results
        float diff = fabs(hw_result - cpu_result);
        float tolerance = 0.001f;
        
        std::cout << "  Batch " << results_received << ":\n";
        std::cout << "    HW result: " << hw_result << "\n";
        std::cout << "    CPU result: " << cpu_result << "\n";
        std::cout << "    Difference: " << diff;
        
        if (diff > tolerance) {
            std::cout << "  [ERROR: Mismatch!]\n";
            errors++;
        } else {
            std::cout << "  [OK]\n";
        }
        
        results_received++;
    }
    
    // ==== SUMMARY ====
    std::cout << "\n========================================\n";
    std::cout << "   Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Results received: " << results_received << "/" << num_bats << "\n";
    std::cout << "Errors: " << errors << "\n";
    
    if (results_received != num_bats) {
        std::cout << "ERROR: Expected " << num_bats << " results, got " 
                  << results_received << "\n";
        errors++;
    }
    
    // Cleanup
    delete[] vectors_in;
    
    if (errors == 0) {
        std::cout << "\nSUCCESS: All tests passed!\n";
        return 0;
    } else {
        std::cout << "\nFAILURE: " << errors << " test(s) failed!\n";
        return 1;
    }
}