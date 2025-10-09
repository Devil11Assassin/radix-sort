#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <limits>

static_assert(sizeof(int) == 4,
	"ERROR: radix_sort.hpp requires sizeof(int) == 4 bytes.\n");

static_assert(sizeof(long long) == 8,
	"ERROR: radix_sort.hpp requires sizeof(long long) == 8 bytes.\n");

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
	"ERROR: radix_sort.hpp requires IEEE-754 binary32 float (4 bytes).\n");

static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559,
	"ERROR: radix_sort.hpp requires IEEE-754 binary64 double (8 bytes).\n");

class radix_sort
{
private:
	template<typename T>
	static void insertionSort(std::vector<T>& v, int l, int r);

#pragma region int
private:
	static void getCountVectorThreadInt(std::vector<int>& v, std::vector<int>& count, int l, int r, int curShift, int mask, int invertMask);
	static void getCountVector(std::vector<int>& v, std::vector<int>& count, int shiftBits, int base, int mask, int invertMask);
public:
	static void sort(std::vector<int>& v);
#pragma endregion

#pragma region uint
private:
	static void getCountVectorThreadUnsignedInt(std::vector<unsigned int>& v, std::vector<int>& count, int l, int r, int shiftBits, int mask);
	static void getCountVector(std::vector<unsigned int>& v, std::vector<int>& count, int shiftBits, int base, int mask);
public:
	static void sort(std::vector<unsigned int>& v);
#pragma endregion

#pragma region ll
private:
	typedef long long ll;
	static void getCountVectorThreadLL(std::vector<ll>& v, std::vector<int>& count, int l, int r, int shiftBits, int mask, int invertMask);
	static void getCountVector(std::vector<ll>& v, std::vector<int>& count, int l, int r, int shiftBits, int base, int mask, int invertMask);

	struct RegionLL {
		int l;
		int r;
		int len;
		RegionLL(int l, int r, int len) : l(l), r(r), len(len) {}
	};

	static void sortLL(std::vector<ll>& v, std::vector<ll>& tmp, 
		std::vector<RegionLL>& regions, std::unique_lock<std::mutex>& lkRegions, 
		RegionLL initialRegion, bool multiThreaded);
	static void sortThreadLL(std::vector<ll>& v, std::vector<	ll>& tmp,
		std::vector<RegionLL>& regions, std::mutex& regionsLock, 
		std::atomic<int>& runningCounter, int threadIndex);
public:
	static void sort(std::vector<ll>& v);
#pragma endregion

#pragma region ull
private:
	typedef unsigned long long ull;
	static void getCountVectorThreadULL(std::vector<ull>& v, std::vector<int>& count, int l, int r, int shiftBits, int mask);
	static void getCountVector(std::vector<ull>& v, std::vector<int>& count, int shiftBits, int base, int mask, int l, int r);

	static void sortULL(std::vector<ull>& v, std::vector<ull>& tmp, 
		std::vector<RegionLL>& regions, std::unique_lock<std::mutex>& lkRegions, 
		RegionLL initialRegion, bool multiThreaded);
	static void sortThreadULL(std::vector<ull>& v, std::vector<ull>& tmp,
		std::vector<RegionLL>& regions, std::mutex& regionsLock, 
		std::atomic<int>& runningCounter, int threadIndex);
public:
	static void sort(std::vector<ull>& v);
#pragma endregion

#pragma region float
public:
	static void sort(std::vector<float>& v);
#pragma endregion

#pragma region double
public:
	static void sort(std::vector<double>& v);
#pragma endregion

#pragma region string
private:
	struct RegionString 
	{
		int l;
		int r;
		int len;
		int curIndex;
		RegionString(int l, int r, int len, int curIndex) : l(l), r(r), len(len), curIndex(curIndex) {}
	};

	static int getChar(const std::string& s, int index);
	static void getCountVectorThreadString(std::vector<std::string>& v, std::vector<int>& count, int l, int r, int curIndex);
	static void getCountVector(std::vector<std::string>& v, std::vector<int>& count, int l, int r, int curIndex);
	static void sortString(std::vector<std::string>& v, std::vector<std::string>& tmp, 
		std::vector<RegionString>& regions, std::unique_lock<std::mutex>& lkRegions,
		RegionString initialRegion, bool multiThreaded);
	static void sortThreadString(std::vector<std::string>& v, std::vector<std::string>& tmp,
		std::vector<RegionString>& regions, std::mutex& regionsLock, 
		std::atomic<int>& runningCounter, int threadIndex);
public:
	static void sort(std::vector<std::string>& v);
#pragma endregion
};
