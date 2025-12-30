// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
// control
// 0x00 : reserved
// 0x04 : reserved
// 0x08 : reserved
// 0x0c : reserved
// 0x10 : Data signal of vectors
//        bit 31~0 - vectors[31:0] (Read/Write)
// 0x14 : Data signal of vectors
//        bit 31~0 - vectors[63:32] (Read/Write)
// 0x18 : reserved
// 0x1c : Data signal of chromo_len
//        bit 31~0 - chromo_len[31:0] (Read/Write)
// 0x20 : reserved
// 0x24 : Data signal of dim
//        bit 31~0 - dim[31:0] (Read/Write)
// 0x28 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XFITNESS_KERNEL_CONTROL_ADDR_VECTORS_DATA    0x10
#define XFITNESS_KERNEL_CONTROL_BITS_VECTORS_DATA    64
#define XFITNESS_KERNEL_CONTROL_ADDR_CHROMO_LEN_DATA 0x1c
#define XFITNESS_KERNEL_CONTROL_BITS_CHROMO_LEN_DATA 32
#define XFITNESS_KERNEL_CONTROL_ADDR_DIM_DATA        0x24
#define XFITNESS_KERNEL_CONTROL_BITS_DIM_DATA        32

