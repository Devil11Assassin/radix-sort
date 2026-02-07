#pragma once
#include <vector>

namespace benchmark
{
	struct Methods
	{
		const int SORT            = 0;
		const int SORT_PAR        = 0;
		const int STABLE_SORT     = 0;
		const int STABLE_SORT_PAR = 0;
		const int RADIX_SORT      = 0;
		const int RADIX_SORT_PAR  = 0;
	};

	struct Sizes
	{
		std::vector<size_t> RUN_SIZE = {};
		std::vector<size_t> RUN_SIZE_STR = {};
		std::vector<size_t> RUN_SIZE_CLX = {};
	};

	struct RunParams
	{
		const int SORT            = 0;
		const int SORT_PAR        = 0;
		const int STABLE_SORT     = 0;
		const int STABLE_SORT_PAR = 0;
		const int RADIX_SORT      = 0;
		const int RADIX_SORT_PAR  = 0;

		const int RANDOMIZED     = 0;
		const int SORTED         = 0;
		const int REVERSE_SORTED = 0;
		const int NEARLY_SORTED  = 0;
		const int DUPLICATES     = 0;

		std::vector<size_t> RUN_SIZE     = { static_cast<size_t>(1e8) };
		std::vector<size_t> RUN_SIZE_STR = { static_cast<size_t>(5e7) };
		std::vector<size_t> RUN_SIZE_CLX = { static_cast<size_t>(1e7) };

		const int CHAR    = 0;
		const int UCHAR   = 0;
		const int SHORT   = 0;
		const int USHORT  = 0;
		const int INT     = 0;
		const int UINT    = 0;
		const int LL      = 0;
		const int ULL     = 0;
		const int FLOAT   = 0;
		const int DOUBLE  = 0;
		const int STRING  = 0;
		const int CLX_I32 = 0;
		const int CLX_LL  = 0;
		const int CLX_FLT = 0;
		const int CLX_DBL = 0;
		const int CLX_STR = 0;

		const int ITERATIONS = 1;
	};

	void benchmark(RunParams params);
}