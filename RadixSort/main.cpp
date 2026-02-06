#include <iostream>
#include "benchmark.hpp"
#include "radix_sort.hpp"
#include "show_off.hpp"

int main()
{
	//show_off::showOff
	//show_off::validate
	//({
	//	.CHAR        = 1,
	//	.UCHAR       = 1,
	//	.SHORT       = 1,
	//	.USHORT      = 1,
	//	.INT         = 1,
	//	.UINT        = 1,
	//	.LL          = 1,
	//	.ULL         = 1,
	//	.FLOAT       = 1,
	//	.DOUBLE      = 1,
	//	.STRING      = 1,
	//	.COMPLEX_I32 = 1,
	//	.COMPLEX_LL  = 1,
	//	.COMPLEX_FLT = 1,
	//	.COMPLEX_DBL = 1,
	//	.COMPLEX_STR = 1,
	//});
	//std::cout << "meow\n";

	benchmark::benchmark
	({
		.SORT            = 1,
		.SORT_PAR        = 1,
		.STABLE_SORT     = 1,
		.STABLE_SORT_PAR = 1,
		.RADIX_SORT      = 1,
		.RADIX_SORT_PAR  = 1,
		.RUN_SIZE     = { 100, 1'000, 10'000, 100'000, 1'000'000, 10'000'000, 100'000'000 },
		.RUN_SIZE_STR = { 100, 1'000, 10'000, 100'000, 1'000'000, 10'000'000, 50'000'000 },
		.RUN_SIZE_CLX = { 100, 1'000, 10'000, 100'000, 1'000'000, 10'000'000 },
		.CHAR    = 1,
		.UCHAR   = 1,
		.SHORT   = 1,
		.USHORT  = 1,
		.INT     = 1,
		.UINT    = 1,
		.LL      = 1,
		.ULL     = 1,
		.FLOAT   = 1,
		.DOUBLE  = 1,
		.STRING  = 1,
		.CLX_I32 = 1,
		.CLX_LL  = 1,
		.CLX_FLT = 1,
		.CLX_DBL = 1,
		.CLX_STR = 1,
		.ITERATIONS = 5
	});
}