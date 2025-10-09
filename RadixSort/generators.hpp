#pragma once
#include <vector>
#include <string>

class generators
{
public:
	static std::vector<int> generateInts(int n);
	static std::vector<unsigned int> generateUnsignedInts(int n);
	static std::vector<unsigned long long> generateULLs(int n);
	static std::vector<float> generateFloats(int n);
	static std::vector<double> generateDoubles(int n);
	static std::vector<std::string> generateStringsFixed(int n, int len);
	static std::vector<std::string> generateStrings(int n, int maxLen);
};

