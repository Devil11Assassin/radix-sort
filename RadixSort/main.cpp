#include "benchmark.hpp"
#include "radix_sort.hpp"

int main()
{
	benchmark::testing
	//benchmark::benchmark
	({
		//.SORT            = 1,
		//.SORT_PAR        = 1,
		//.STABLE_SORT     = 1,
		//.STABLE_SORT_PAR = 1,
		.RADIX_SORT      = 1,
		.RADIX_SORT_PAR  = 1,
		.RANDOMIZED     = 1,
		.SORTED         = 1,
		.REVERSE_SORTED = 1,
		.NEARLY_SORTED  = 1,
		.DUPLICATES     = 1,
		//.RUN_SIZE     = { 100, 1'000, 10'000, 100'000, 1'000'000, 10'000'000, 100'000'000 },
		//.RUN_SIZE_STR = { 100, 1'000, 10'000, 100'000, 1'000'000, 10'000'000, 50'000'000 },
		//.RUN_SIZE_CLX = { 100, 1'000, 10'000, 100'000, 1'000'000, 10'000'000 },
		.RUN_SIZE     = { 5'000'000 },
		.RUN_SIZE_STR = { 5'000'000 },
		.RUN_SIZE_CLX = { 5'000'000 },
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
		//.CLX_I32 = 1,
		//.CLX_LL  = 1,
		//.CLX_FLT = 1,
		//.CLX_DBL = 1,
		//.CLX_STR = 1,
		.ITERATIONS = 3
	});

	std::vector<int> tempBreakpoint;
}