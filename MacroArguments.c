#include <stdio.h>

#define SelectArg8(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7
#define BraceTrigger(...) ,

#define HasComma(...) SelectArg8(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 0)

#define IsArgEmpty_Helper_Case0001() ,
#define IsArgEmpty_Helper_Expand(a, b, c, d) HasComma(IsArgEmpty_Helper_Case ## a ## b ## c ## d ())
#define IsArgEmpty_Helper(a, b, c, d) IsArgEmpty_Helper_Expand(a, b, c, d)
#define IsArgEmpty(...) IsArgEmpty_Helper(	\
	HasComma(__VA_ARGS__),					\
	HasComma(BraceTrigger __VA_ARGS__),		\
	HasComma(__VA_ARGS__ ()),				\
	HasComma(BraceTrigger __VA_ARGS__ ())	\
)

//HasComma(__VA_ARGS__) tests whether ... contains 0 or 1 argument
//HasComma(BraceTrigger __VA_ARGS__) tests whether ... is an argument with braces
//HasComma(__VA_ARGS__ ()) tests whether ... is an expandable macro with braces
//HasComma(BraceTrigger __VA_ARGS__ ()) tests finally whether ... is empty

#define GetArgCount_0(...) SelectArg8(__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1)
#define GetArgCount_1(...) 0
#define GetArgCount_Expand2(n, ...) GetArgCount_ ## n (__VA_ARGS__)
#define GetArgCount_Expand(n, ...) GetArgCount_Expand2(n, __VA_ARGS__)
#define GetArgCount(...) GetArgCount_Expand(IsArgEmpty(__VA_ARGS__), __VA_ARGS__)

//MyMax accepts from 0 to 2 arguments
#define MyMax_0(...) 0
#define MyMax_1(a) (a)
#define MyMax_2(a, b) ((a) > (b) ? (a) : (b))
#define MyMax_Expand2(n, ...) MyMax_ ## n (__VA_ARGS__)
#define MyMax_Expand(n, ...) MyMax_Expand2(n, __VA_ARGS__)
#define MyMax(...) MyMax_Expand(GetArgCount(__VA_ARGS__), __VA_ARGS__)

int main(void) {
	printf("%d\n", GetArgCount());
	printf("%d\n", GetArgCount(/*ejidf*/));
	printf("%d\n", GetArgCount( () ));
	printf("%d\n", GetArgCount(a));
	printf("%d\n", GetArgCount(a, b, c));
	printf("--------------------\n");
	printf("%d\n", MyMax());
	printf("%d\n", MyMax(3));
	printf("%d\n", MyMax(5, 6));
}