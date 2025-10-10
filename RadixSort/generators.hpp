#pragma once
#include <vector>
#include <string>

static_assert(sizeof(int) == 4,
	"ERROR: generators.hpp requires sizeof(int) == 4 bytes.\n");

static_assert(sizeof(long long) == 8,
	"ERROR: generators.hpp requires sizeof(long long) == 8 bytes.\n");

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
	"ERROR: generators.hpp requires IEEE-754 binary32 float (4 bytes).\n");

static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559,
	"ERROR: generators.hpp requires IEEE-754 binary64 double (8 bytes).\n");

class generators
{
public:
	static std::vector<int> generateInts(int n);
	static std::vector<unsigned int> generateUnsignedInts(int n);
	static std::vector<long long> generateLLs(int n);
	static std::vector<unsigned long long> generateULLs(int n);
	static std::vector<float> generateFloats(int n);
	static std::vector<double> generateDoubles(int n);
	static std::vector<std::string> generateStringsFixed(int n, int len);
	static std::vector<std::string> generateStrings(int n, int maxLen);
	
	template<typename T>
	static std::vector<T> generate(int n)
	{
		if constexpr (std::is_same_v<T, int>)
			return generateInts(n);
		else if constexpr (std::is_same_v<T, unsigned int>)
			return generateUnsignedInts(n);
		else if constexpr (std::is_same_v<T, long long>)
			return generateLLs(n);
		else if constexpr (std::is_same_v<T, unsigned long long>)
			return generateULLs(n);
		else if constexpr (std::is_same_v<T, float>)
			return generateFloats(n);
		else if constexpr (std::is_same_v<T, double>)
			return generateDoubles(n);
		else if constexpr (std::is_same_v<T, std::string>)
			return generateStrings(n, 20);
		else
			static_assert(sizeof(T) == 0, "ERROR: Unable to generate vector!\nCAUSE: Unsupported type!\n");
	}
};
