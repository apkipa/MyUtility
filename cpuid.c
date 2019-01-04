//CPUID introduction is on Page 3-190[Page 292 relative to Intel SDM Volume 2A]
//Opcode: 0F A2
//Fill input in eax (and in ecx if necessary) and retrieve data from eax, ebx, ecx and edx

/*	Details:
	eax = 0H(extra: 80000000H):
		eax: Maximum input value for basic / extended CPUID information
		ebx, ecx, edx: Manufacturer ID("GenuineIntel" for Intel) on eax = 0H, or reserved on eax = 80000000H
	eax = 01H:
		eax: Signature of the CPU(Type, Family, Model, and Stepping ID)(See more on Page 3-208[Page 310])
		ebx: Miscellaneous(Brand Index, ...)(See more on Page 3-191[Page 293])
		ecx, edx: Feature information(See more on Page 3-210[Page 312])
	...
*/

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#ifdef _MSC_VER
#include <intrin.h>
#elif defined __GNUC__
#include <cpuid.h>
#endif	//!defined __GNUC__

#define MakeDword(low, high) ((uint16_t)(low) | (uint32_t)((high) << 16))
#define LowWord(dword) ((uint16_t)(dword))
#define HighWord(dword) ((uint16_t)((dword) >> 16))

#define CpuId_EaxBitOffset(n) (n)
#define CpuId_EbxBitOffset(n) ((n) + 32)
#define CpuId_EcxBitOffset(n) ((n) + 64)
#define CpuId_EdxBitOffset(n) ((n) + 96)
#define CpuIdEx_EaxBitOffset(leaf, n) MakeDword(n, leaf)
#define CpuIdEx_EbxBitOffset(leaf, n) MakeDword((n) + 32, leaf)
#define CpuIdEx_EcxBitOffset(leaf, n) MakeDword((n) + 64, leaf)
#define CpuIdEx_EdxBitOffset(leaf, n) MakeDword((n) + 96, leaf)

#define CpuId_FeatureOffset_SSE CpuId_EdxBitOffset(25)
#define CpuId_FeatureOffset_SSE2 CpuId_EdxBitOffset(26)
#define CpuId_FeatureOffset_SSE3 CpuId_EcxBitOffset(0)
#define CpuId_FeatureOffset_SSSE3 CpuId_EcxBitOffset(9)
#define CpuId_FeatureOffset_SSE4_1 CpuId_EcxBitOffset(19)
#define CpuId_FeatureOffset_SSE4_2 CpuId_EcxBitOffset(20)
#define CpuId_FeatureOffset_AVX CpuId_EcxBitOffset(28)
#define CpuId_FeatureOffset_AVX2 CpuIdEx_EbxBitOffset(0, 5)	//CpuIdEx required

void ExecCpuId(int nLevel, int *eax, int *ebx, int *ecx, int *edx) {
	int arr[4];
#ifdef _MSC_VER
	__cpuid(arr, nLevel);
#elif defined __GNUC__
	__cpuid(nLevel, arr[0], arr[1], arr[2], arr[3]);
	//__get_cpuid(nLevel, &arr[0], &arr[1], &arr[2], &arr[3]);
#endif	//!defined __GNUC__
	if (eax != NULL)
		*eax = arr[0];
	if (ebx != NULL)
		*ebx = arr[1];
	if (ecx != NULL)
		*ecx = arr[2];
	if (edx != NULL)
		*edx = arr[3];
}

void ExecCpuIdEx(int nLevel, int nLeaf, int *eax, int *ebx, int *ecx, int *edx) {
	int arr[4];
#ifdef _MSC_VER
	__cpuidex(arr, nLevel, nLeaf);
#elif defined __GNUC__
	__cpuid_count(nLevel, nLeaf, arr[0], arr[1], arr[2], arr[3]);
#endif	//!defined __GNUC__
	if (eax != NULL)
		*eax = arr[0];
	if (ebx != NULL)
		*ebx = arr[1];
	if (ecx != NULL)
		*ecx = arr[2];
	if (edx != NULL)
		*edx = arr[3];
}

//NOTE: nBitOffset	[0 - 31]	-> eax[nBitOffset]
//					[32 - 63]	-> ebx[nBitOffset - 32]
//					[64 - 95]	-> ecx[nBitOffset - 64]
//					[96 - 126]	-> edx[nBitOffset - 96]
bool CpuId_TestBit(int nLevel, unsigned int nBitOffset) {
	int a, b, c, d;
	//__cpuid(nLevel, a, b, c, d);
	ExecCpuId(nLevel, &a, &b, &c, &d);
	if (nBitOffset < 32)
		return (bool)(a & (1 << nBitOffset));
	if (nBitOffset < 64)
		return (bool)(b & (1 << (nBitOffset - 32)));
	if (nBitOffset < 96)
		return (bool)(c & (1 << (nBitOffset - 64)));
	if (nBitOffset < 128)
		return (bool)(d & (1 << (nBitOffset - 96)));
	return false;
}

bool CpuIdEx_TestBit(int nLevel, unsigned int nBitOffset) {
	int a, b, c, d;
	//__cpuid_count(nLevel, HighWord(nBitOffset), a, b, c, d);
	ExecCpuIdEx(nLevel, HighWord(nBitOffset), &a, &b, &c, &d);
	if (LowWord(nBitOffset) < 32)
		return (bool)(a & (1 << nBitOffset));
	if (LowWord(nBitOffset) < 64)
		return (bool)(b & (1 << (nBitOffset - 32)));
	if (LowWord(nBitOffset) < 96)
		return (bool)(c & (1 << (nBitOffset - 64)));
	if (LowWord(nBitOffset) < 128)
		return (bool)(d & (1 << (nBitOffset - 96)));
	return false;
}

const char* CpuId_GetManufacturerId(void) {
	static int arr[4];
	//__cpuid(0, arr[3], arr[0], arr[2], arr[1]);	//String order: ebx, edx, ecx
	ExecCpuId(0, NULL, &arr[0], &arr[2], &arr[1]);	//String order: ebx, edx, ecx
	arr[3] = 0;
	return (char*)arr;
}

int main(void) {
	//int a, b, c, d;
	//__cpuid(1, a, b, c, d);
	//printf("Level 1: %x %x %x %x\n\n", a, b, c, d);
	
	printf("Manufacturer Id: %s\n", CpuId_GetManufacturerId());
	printf("Instructions support:\n");
	printf("SSE: %s\n", CpuId_TestBit(1, CpuId_FeatureOffset_SSE) ? "Supported" : "Unsupported");
	printf("SSE2: %s\n", CpuId_TestBit(1, CpuId_FeatureOffset_SSE2) ? "Supported" : "Unsupported");
	printf("SSE3: %s\n", CpuId_TestBit(1, CpuId_FeatureOffset_SSE3) ? "Supported" : "Unsupported");
	printf("SSSE3: %s\n", CpuId_TestBit(1, CpuId_FeatureOffset_SSSE3) ? "Supported" : "Unsupported");
	printf("SSE4.1: %s\n", CpuId_TestBit(1, CpuId_FeatureOffset_SSE4_1) ? "Supported" : "Unsupported");
	printf("SSE4.2: %s\n", CpuId_TestBit(1, CpuId_FeatureOffset_SSE4_2) ? "Supported" : "Unsupported");
	printf("AVX: %s\n", CpuId_TestBit(1, CpuId_FeatureOffset_AVX) ? "Supported" : "Unsupported");
	printf("AVX2: %s\n", CpuIdEx_TestBit(7, CpuId_FeatureOffset_AVX2) ? "Supported" : "Unsupported");
	
	//system("pause");
	
	return 0;
}