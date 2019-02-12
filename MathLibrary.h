//Warning: Legacy file. May be changed later.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//Constant definitions
//	Public
static const double MathLib_lfNaN = 0.0 / 0.0;
static const float MathLib_fNaN = 0.0f / 0.0f;
static const double MathLib_lfInf = 1.0 / 0.0;
static const float MathLib_fInf = 1.0f / 0.0f;

//Macro definitions
//	Public
#define ML_FUNC_CC __stdcall	//Calling convention
//	Integral Calculator
#define ML_CI_MIN_PRECISION 1
#define ML_CI_MAX_PRECISION 20
#define ML_CI_DEFAULT_PRECISION 5

//Type definitions
//	Integral Calculator
typedef double(ML_FUNC_CC *MathLib_CalculateIntegral_FunctionCallback)(double x, void *pParam);

//Functions
//	Integral Calculator
double ML_FUNC_CC MathLib_CalculateIntegral(MathLib_CalculateIntegral_FunctionCallback pFunc, void *pParam, double fRangeBegin, double fRangeEnd, int n);

#ifdef __cplusplus
}
#endif