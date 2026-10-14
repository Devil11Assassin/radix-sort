#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "radix_sort.hpp"

int main()
{
	std::vector<int> vOrig(100'000'000);
	std::iota(vOrig.rbegin(), vOrig.rend(), 0);

	ankerl::nanobench::Bench().minEpochIterations(2).run("data_copy", [&vOrig]
	{
		std::vector<int> v(vOrig);
		ankerl::nanobench::doNotOptimizeAway(v);
	});

	ankerl::nanobench::Bench().run("radix_sort", [&vOrig] 
	{
		std::vector<int> v(vOrig);
		radix_sort::sort(v);
		ankerl::nanobench::doNotOptimizeAway(v);
	});
}
