// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xfitness_kernel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XFitness_kernel_CfgInitialize(XFitness_kernel *InstancePtr, XFitness_kernel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XFitness_kernel_Set_vectors(XFitness_kernel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XFitness_kernel_WriteReg(InstancePtr->Control_BaseAddress, XFITNESS_KERNEL_CONTROL_ADDR_VECTORS_DATA, (u32)(Data));
    XFitness_kernel_WriteReg(InstancePtr->Control_BaseAddress, XFITNESS_KERNEL_CONTROL_ADDR_VECTORS_DATA + 4, (u32)(Data >> 32));
}

u64 XFitness_kernel_Get_vectors(XFitness_kernel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XFitness_kernel_ReadReg(InstancePtr->Control_BaseAddress, XFITNESS_KERNEL_CONTROL_ADDR_VECTORS_DATA);
    Data += (u64)XFitness_kernel_ReadReg(InstancePtr->Control_BaseAddress, XFITNESS_KERNEL_CONTROL_ADDR_VECTORS_DATA + 4) << 32;
    return Data;
}

void XFitness_kernel_Set_chromo_len(XFitness_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XFitness_kernel_WriteReg(InstancePtr->Control_BaseAddress, XFITNESS_KERNEL_CONTROL_ADDR_CHROMO_LEN_DATA, Data);
}

u32 XFitness_kernel_Get_chromo_len(XFitness_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XFitness_kernel_ReadReg(InstancePtr->Control_BaseAddress, XFITNESS_KERNEL_CONTROL_ADDR_CHROMO_LEN_DATA);
    return Data;
}

void XFitness_kernel_Set_dim(XFitness_kernel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XFitness_kernel_WriteReg(InstancePtr->Control_BaseAddress, XFITNESS_KERNEL_CONTROL_ADDR_DIM_DATA, Data);
}

u32 XFitness_kernel_Get_dim(XFitness_kernel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XFitness_kernel_ReadReg(InstancePtr->Control_BaseAddress, XFITNESS_KERNEL_CONTROL_ADDR_DIM_DATA);
    return Data;
}

