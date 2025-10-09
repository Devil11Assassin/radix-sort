#pragma once
#include <vector>
#include <string>
#include <mutex>

class radix_sort
{
#pragma region int
private:
	static void getCountVectorThreadInt(std::vector<int>& v, std::vector<int>& count, int l, int r, int shiftBits, int mask, int invertMask);
	static void getCountVector(std::vector<int>& v, std::vector<int>& count, int shiftBits, int base, int mask, int invertMask);
	static void insertionSort(std::vector<int>& v, int l, int r);
public:
	static void sort(std::vector<int>& v);
#pragma endregion

#pragma region uint
private:
	static void getCountVectorThreadUnsignedInt(std::vector<unsigned int>& v, std::vector<int>& count, int l, int r, int shiftBits, int mask);
	static void getCountVector(std::vector<unsigned int>& v, std::vector<int>& count, int shiftBits, int base, int mask);
	static void insertionSort(std::vector<unsigned int>& v, int l, int r);
public:
	static void sort(std::vector<unsigned int>& v);
#pragma endregion

#pragma region ull
private:
	typedef unsigned long long ull;
	static void getCountVectorThreadULL(std::vector<ull>& v, std::vector<int>& count, int l, int r, int shiftBits, int mask);
	static void getCountVector(std::vector<ull>& v, std::vector<int>& count, int shiftBits, int base, int mask, int l, int r);
	static void insertionSort(std::vector<ull>& v, int l, int r);

	struct RegionULL {
		int l;
		int r;
		int len;
		RegionULL(int l, int r, int len) : l(l), r(r), len(len) {}
	};

	static void sortULL(std::vector<ull>& v, std::vector<ull>& tmp, 
		std::vector<RegionULL>& regions, std::unique_lock<std::mutex>& lkRegions, 
		RegionULL initialRegion, bool multiThreaded);
	static void sortThreadULL(std::vector<ull>& v, std::vector<ull>& tmp,
		std::vector<RegionULL>& regions, std::mutex& regionsLock, 
		std::atomic<int>& runningCounter, int threadIndex);
public:
	static void sort(std::vector<ull>& v);
#pragma endregion

#pragma region float
private:
	static void insertionSort(std::vector<float>& v, int l, int r);
public:
	static void sort(std::vector<float>& v);
#pragma endregion

#pragma region double
private:
	static void insertionSort(std::vector<double>& v, int l, int r);
public:
	static void sort(std::vector<double>& v);
#pragma endregion

#pragma region string
private:
	static int getChar(const std::string& s, int index);
	static void getCountVectorThreadString(std::vector<std::string>& v, std::vector<int>& count, int l, int r, int curIndex);
	static void getCountVector(std::vector<std::string>& v, std::vector<int>& count, int l, int r, int curIndex);
	static void insertionSort(std::vector<std::string>& v, int l, int r);
	
	struct RegionString {
		int l;
		int r;
		int len;
		int curIndex;
		RegionString(int l, int r, int len, int curIndex) : l(l), r(r), len(len), curIndex(curIndex) {}
	};

	static void sortString(std::vector<std::string>& v, std::vector<std::string>& tmp, std::vector<RegionString>& regions, std::unique_lock<std::mutex>& lkRegions, RegionString initialRegion, bool multiThreaded);
	static void sortThreadString(std::vector<std::string>& v, std::vector<std::string>& tmp,
		std::vector<RegionString>& regions, std::mutex& regionsLock, 
		std::atomic<int>& runningCounter, int threadIndex);
public:
	static void sort(std::vector<std::string>& v);
#pragma endregion
};
