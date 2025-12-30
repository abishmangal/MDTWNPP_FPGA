// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#ifdef SDT
#include "xparameters.h"
#endif
#include "xfitness_kernel.h"

extern XFitness_kernel_Config XFitness_kernel_ConfigTable[];

#ifdef SDT
XFitness_kernel_Config *XFitness_kernel_LookupConfig(UINTPTR BaseAddress) {
	XFitness_kernel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XFitness_kernel_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XFitness_kernel_ConfigTable[Index].Control_BaseAddress == BaseAddress) {
			ConfigPtr = &XFitness_kernel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XFitness_kernel_Initialize(XFitness_kernel *InstancePtr, UINTPTR BaseAddress) {
	XFitness_kernel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XFitness_kernel_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XFitness_kernel_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XFitness_kernel_Config *XFitness_kernel_LookupConfig(u16 DeviceId) {
	XFitness_kernel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XFITNESS_KERNEL_NUM_INSTANCES; Index++) {
		if (XFitness_kernel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XFitness_kernel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XFitness_kernel_Initialize(XFitness_kernel *InstancePtr, u16 DeviceId) {
	XFitness_kernel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XFitness_kernel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XFitness_kernel_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

