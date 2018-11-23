#include <bits/stdc++.h>

using namespace std;

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif	//!defined max
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif	//!defined min
#ifndef clamp
#define clamp(n, nMin, nMax) min(max(n, nMin), nMax)
#endif	//!defined clamp

#ifdef redirect
#undef redirect
#endif	//defined clamp
#define redirect(in, out) do {	\
	freopen(in, "r", stdin);	\
	freopen(out, "w", stdout);	\
} while (0)
