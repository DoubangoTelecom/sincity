// cpu.cpp : Defines the entry point for the console application.
//

#include <windows.h>

#define cpuid(func, func2, a, b, c, d)\
	__asm mov eax, func\
	__asm mov ecx, func2\
	__asm cpuid\
	__asm mov a, eax\
	__asm mov b, ebx\
	__asm mov c, ecx\
	__asm mov d, edx

#define HAS_MMX     0x01
#define HAS_SSE     0x02
#define HAS_SSE2    0x04
#define HAS_SSE3    0x08
#define HAS_SSSE3   0x10
#define HAS_SSE4_1  0x20
#define HAS_AVX     0x40
#define HAS_AVX2    0x80
#ifndef BIT
#define BIT(n) (1<<n)
#endif

#define SC_DEBUG_INFO(FMT, ...) fprintf(stderr, "*[SINCITY INFO]: " FMT "\n", ##__VA_ARGS__)

int _tmain(int argc, _TCHAR* argv[])
{
	unsigned int flags = 0;
	unsigned int mask = ~0;
	unsigned int reg_eax, reg_ebx, reg_ecx, reg_edx;
	(void)reg_ebx;

	SC_DEBUG_INFO("==Doubango Telecom: Mentor EM CPU test==");

	MEMORYSTATUS memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&memInfo);
	SC_DEBUG_INFO("dwTotalVirtual=%d, dwAvailVirtual=%d, dwTotalPageFile=%d, dwAvailPageFile=%d", memInfo.dwTotalVirtual, memInfo.dwAvailVirtual, memInfo.dwTotalPageFile, memInfo.dwAvailPageFile);

	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	SC_DEBUG_INFO("dwNumberOfProcessors=%d", sysinfo.dwNumberOfProcessors);

	/* Ensure that the CPUID instruction supports extended features */
	cpuid(0, 0, reg_eax, reg_ebx, reg_ecx, reg_edx);

	if (reg_eax < 1) {
		SC_DEBUG_INFO("reg_eax < 1");
		goto bail;
	}

	/* Get the standard feature flags */
	cpuid(1, 0, reg_eax, reg_ebx, reg_ecx, reg_edx);

	if (reg_edx & BIT(23)){ flags |= HAS_MMX; SC_DEBUG_INFO("HAS_MMX=1");}
	else SC_DEBUG_INFO("MMX=0");

	if (reg_edx & BIT(25)){ flags |= HAS_SSE;SC_DEBUG_INFO("SSE=1");}
	else SC_DEBUG_INFO("SSE=0");

	if (reg_edx & BIT(26)){ flags |= HAS_SSE2;SC_DEBUG_INFO("SSE2=1");}
	else SC_DEBUG_INFO("SSE2=0");

	if (reg_ecx & BIT(0)){ flags |= HAS_SSE3;SC_DEBUG_INFO("SSE3=1");}
	else SC_DEBUG_INFO("SSE3=0");

	if (reg_ecx & BIT(9)){ flags |= HAS_SSSE3;SC_DEBUG_INFO("SSSE3=1");}
	else SC_DEBUG_INFO("SSSE3=0");

	if (reg_ecx & BIT(19)){ flags |= HAS_SSE4_1;SC_DEBUG_INFO("SSE4.1=1");}
	else SC_DEBUG_INFO("SSE4.1=0");

	if (reg_ecx & BIT(28)){ flags |= HAS_AVX;SC_DEBUG_INFO("AVX=1");}
	else SC_DEBUG_INFO("AVX=0");

	/* Get the leaf 7 feature flags. Needed to check for AVX2 support */
	reg_eax = 7;
	reg_ecx = 0;
	cpuid(7, 0, reg_eax, reg_ebx, reg_ecx, reg_edx);

	if (reg_ebx & BIT(5)){ flags |= HAS_AVX2;SC_DEBUG_INFO("AVX2=1");}
	else SC_DEBUG_INFO("AVX2=0");

bail:
	getchar();
	return 0;
}

