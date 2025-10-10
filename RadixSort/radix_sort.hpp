#pragma once
#pragma region HEADERS
#include <vector>
#include <string>
#include <mutex>
#include <limits>
#pragma endregion

#pragma region ASSERTS
static_assert(sizeof(int) == 4,
	"ERROR: radix_sort.hpp requires sizeof(int) == 4 bytes.\n");

static_assert(sizeof(long long) == 8,
	"ERROR: radix_sort.hpp requires sizeof(long long) == 8 bytes.\n");

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
	"ERROR: radix_sort.hpp requires IEEE-754 binary32 float (4 bytes).\n");

static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559,
	"ERROR: radix_sort.hpp requires IEEE-754 binary64 double (8 bytes).\n");
#pragma endregion

class radix_sort
{
#pragma region FIELDS & TYPE DECLARATIONS
	static constexpr int SHIFT_BITS = 8;
	static constexpr int BASE = 256;
	static constexpr int MASK = 0xFF;
	static constexpr int INVERT_MASK = 0x80;

	static constexpr int SIZE_THRESHOLD = 8000000;
	static constexpr double BUCKET_THRESHOLD = 1000000.0;

	static constexpr int CHARS_ALLOC = 257;
	static constexpr int CHARS = 256;
	
	typedef long long ll;
	typedef unsigned long long ull;

	struct RegionLL 
	{
		int l;
		int r;
		int len;
		RegionLL(int l, int r, int len) : l(l), r(r), len(len) {}
	};

	struct RegionString
	{
		int l;
		int r;
		int len;
		int curIndex;
		RegionString(int l, int r, int len, int curIndex) : l(l), r(r), len(len), curIndex(curIndex) {}
	};
#pragma endregion

#pragma region HELPERS
	template<typename T>
	static void insertionSort(std::vector<T>& v, int l, int r);

	template<typename T>
	static void getCountVectorThread(std::vector<T>& v, std::vector<int>& count, int curShiftOrIndex, int l, int r);

	template<typename T>
	static void getCountVector(std::vector<T>& v, std::vector<int>& count, int curShiftOrIndex, int l, int r);

	static int getChar(const std::string& s, int index);
#pragma endregion

#pragma region IMPLEMENTATIONS
	template<typename T>
	static void sort1(std::vector<T>& v);

	template<typename T>
	static void sort2(std::vector<T>& v);
#pragma endregion

	template<typename T, typename TRegion>
	static void sortT(std::vector<T>& v, std::vector<T>& tmp,
		std::vector<TRegion>& regions, std::unique_lock<std::mutex>& lkRegions,
		TRegion initialRegion, bool multiThreaded);

#pragma region ll
private:
	static void sortThreadLL(std::vector<ll>& v, std::vector<	ll>& tmp,
		std::vector<RegionLL>& regions, std::mutex& regionsLock, 
		std::atomic<int>& runningCounter, int threadIndex);
public:
	static void sortLL(std::vector<ll>& v);
#pragma endregion

#pragma region ull
private:
	static void sortThreadULL(std::vector<ull>& v, std::vector<ull>& tmp,
		std::vector<RegionLL>& regions, std::mutex& regionsLock, 
		std::atomic<int>& runningCounter, int threadIndex);
public:
	static void sortULL(std::vector<ull>& v);
#pragma endregion

#pragma region string
private:
	static void sortThreadString(std::vector<std::string>& v, std::vector<std::string>& tmp,
		std::vector<RegionString>& regions, std::mutex& regionsLock, 
		std::atomic<int>& runningCounter, int threadIndex);
public:
	static void sortString(std::vector<std::string>& v);
#pragma endregion

	template<typename T>
	static void sort(std::vector<T>& v);
};
