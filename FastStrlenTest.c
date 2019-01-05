#include <nmmintrin.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

//Everything can be seen here: https://www.strchr.com/strcmp_and_strlen_using_sse_4.2

#define AlignUpward(n, align) (((uintptr_t)(n) + (align) - 1) & ~(uintptr_t)((align) - 1))
#define AlignDownward(n, align) ((uintptr_t)(n) & ~(uintptr_t)((align) - 1))

char str[1024 * 1024];

size_t strlen_1(const char *str) {	//Standard strlen
	size_t nLen = 0;
	while (str[nLen] != '\0')
		nLen++;
	return nLen;
}

size_t strlen_2(const char *str) {	//strlen optimized with SSE4.2
	//NOTE: When optimization is enabled, strlen_2 will be faster than CRT strlen(or intrinsic strlen)
	const char *strTemp;
	__m128i xmm0, xmm1;
	size_t nExtra, i;
	int nTimes;
	
	nExtra = 0;
	strTemp = (const char*)AlignUpward(str, 16);
	nTimes = (int)(strTemp - str);
	for (int i = 0; i < nTimes; i++) {
		if (str[i] == '\0')
			return nExtra;
		nExtra++;
	}
	
	xmm0 = _mm_setzero_si128();
	xmm1 = _mm_load_si128((__m128i*)strTemp);
	for (i = 0; !_mm_cmpistrc(xmm0, xmm1, _SIDD_CMP_EQUAL_EACH); xmm1 = _mm_load_si128((__m128i*)strTemp + i))
		i++;
	
	while (str[i * 16 + nExtra] != '\0')
		nExtra++;
	
	return i * 16 + nExtra;
}

size_t strlen_3(const char *str) {	//SSE2 strlen from the web, faster than SSE4.2 strlen(optimization required)
	__m128i zero = _mm_set1_epi8(0);
	__m128i *s_aligned = (__m128i*)(((long)str) & -0x10L);
	uint8_t misbits = ((long)str) & 0xf;
	__m128i s16cs = _mm_load_si128(s_aligned);
	__m128i bytemask = _mm_cmpeq_epi8(s16cs, zero);
	int bitmask = _mm_movemask_epi8(bytemask);
	bitmask = (bitmask >> misbits) << misbits;
	
	// Alternative: use TEST instead of BSF, then BSF at end (only). Much better on older CPUs
	// TEST has latency 1, while BSF has 3!
	while (bitmask == 0) {
		s16cs = _mm_load_si128(++s_aligned);
		bytemask = _mm_cmpeq_epi8(s16cs, zero);
		bitmask = _mm_movemask_epi8(bytemask);
	}

	// bsf only takes 1 clock cycle on modern cpus
	return (((const char*)s_aligned) - str) + __builtin_ctz(bitmask);
}

double benchmark(size_t (*pfStrlen)(const char *str), const char *str, int nTimes) {
	clock_t clkBegin, clkEnd;
	clkBegin = clock();
	for (int i = 0; i < nTimes; i++) {
		pfStrlen(str);
	}
	clkEnd = clock();
	return (double)(clkEnd - clkBegin) / CLOCKS_PER_SEC;
}

int main(void) {
	int nTimes = 1000;
	
	memset(str, '1', sizeof(str) - 1);
	str[1024 * 1024 - 348] = '\0';
	
	printf("Start!\n");
	
	printf("Test strlen(): returned %d\n", (int)strlen(str + 1));	//CRT strlen
	printf("Cost time %.2fs\n\n", benchmark(strlen, str + 1, nTimes));
	
	printf("Test strlen_1(): returned %d\n", (int)strlen_1(str + 1));	//Standard strlen
	printf("Cost time %.2fs\n\n", benchmark(strlen_1, str + 1, nTimes));
	
	printf("Test strlen_2(): returned %d\n", (int)strlen_2(str + 1));	//SSE4.2 strlen
	printf("Cost time %.2fs\n\n", benchmark(strlen_2, str + 1, nTimes));
	
	printf("Test strlen_3(): returned %d\n", (int)strlen_3(str + 1));	//SSE2 strlen
	printf("Cost time %.2fs\n\n", benchmark(strlen_3, str + 1, nTimes));
	
	system("pause");
	
	return 0;
}