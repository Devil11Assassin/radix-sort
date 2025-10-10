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

	static constexpr int SHIFT_BITS = 8;
	static constexpr int BASE = 256;
	static constexpr int MASK = 0xFF;
	static constexpr int INVERT_MASK = 0x80;

	static constexpr int SIZE_THRESHOLD = 8000000;
	static constexpr double BUCKET_THRESHOLD = 1000000.0;

	static constexpr int CHARS_ALLOC = 257;
	static constexpr int CHARS = 256;

	template<typename T>
	static void getCountVectorThread(std::vector<T>& v, std::vector<int>& count, int curShiftOrIndex, int l, int r);

	template<typename T>
	static void getCountVector(std::vector<T>& v, std::vector<int>& count, int curShiftOrIndex, int l, int r);


	template<typename T>
	static void sort1(std::vector<T>& v);

	template<typename T>
	static void sort2(std::vector<T>& v);

#pragma region ll
private:
	typedef long long ll;

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
	static void sortLL(std::vector<ll>& v);
#pragma endregion

#pragma region ull
private:
	typedef unsigned long long ull;

	static void sortULL(std::vector<ull>& v, std::vector<ull>& tmp, 
		std::vector<RegionLL>& regions, std::unique_lock<std::mutex>& lkRegions, 
		RegionLL initialRegion, bool multiThreaded);
	static void sortThreadULL(std::vector<ull>& v, std::vector<ull>& tmp,
		std::vector<RegionLL>& regions, std::mutex& regionsLock, 
		std::atomic<int>& runningCounter, int threadIndex);
public:
	static void sortULL(std::vector<ull>& v);
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
	static void sortString(std::vector<std::string>& v, std::vector<std::string>& tmp, 
		std::vector<RegionString>& regions, std::unique_lock<std::mutex>& lkRegions,
		RegionString initialRegion, bool multiThreaded);
	static void sortThreadString(std::vector<std::string>& v, std::vector<std::string>& tmp,
		std::vector<RegionString>& regions, std::mutex& regionsLock, 
		std::atomic<int>& runningCounter, int threadIndex);
public:
	static void sortString(std::vector<std::string>& v);
#pragma endregion

	template<typename T>
	static void sort(std::vector<T>& v);
};
