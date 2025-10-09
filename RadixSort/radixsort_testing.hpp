#pragma once
#include <vector>
#include <string>

class radixsort_testing
{
	//int
	enum MethodInt {
		SORT,
		RADIX_10,
		RADIX_256,
		RADIX_256_FAST,
		RADIX_256_COUNTING,
		RADIX_256_COUNTING_MULTI,
		RADIX_256_MSD
	};

	static std::vector<int> methodsInt;

	static void radixSort(std::vector<int>& v);
	static void radixSort256(std::vector<int>& v);
	static void radixSort256Fast(std::vector<int>& v);
	static void radixSort256Counting(std::vector<int>& v);

	static void getCountVectorThread(std::vector<int>& v, std::vector<int>& count, int l, int r, int shiftBits, int mask);
	static void getCountVector(std::vector<int>& v, std::vector<int>& count, int shiftBits, int base, int mask);
	static void radixSort256CountingMulti(std::vector<int>& v);

	static void insertionSort(std::vector<int>& v, int l, int r);
	static void radixSort256MSDRecursion(std::vector<int>& v, std::vector<int>& tmp, int l, int r, int n);
	static void radixSort256MSD(std::vector<int>& v);

	static long long useMethod(std::vector<int>& v, MethodInt method);
	static void addOutput(std::string& output, MethodInt method, long long time);

	//string
	enum MethodString {
		STD_SORT,
		RADIX_MSD
	}; 

	static std::vector<int> methodsString;

	inline static int getChar(const std::string& s, int index);
	static void insertionSort(std::vector<std::string>& v, int l, int r);
	static void radixSortMSDRecursion(std::vector<std::string>& v, std::vector<std::string>& tmp, int l, int r, int n, int curIndex);
	static void radixSortLSD(std::vector<std::string>& v, std::vector<std::string>& tmp, int n);
	static void radixSortMSD(std::vector<std::string>& v);

	static long long useMethod(std::vector<std::string>& v, MethodString method);
	static void addOutput(std::string& output, MethodString method, long long time);

public:
	//int
	static void sort(std::vector<int>& v);
	static void sortCompare(std::vector<int>& v);
	
	//string
	static void sortCompare(std::vector<std::string>& v);
};
