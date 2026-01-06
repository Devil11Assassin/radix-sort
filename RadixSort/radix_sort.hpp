#pragma once
#pragma region HEADERS
#include <concepts>
#include <limits>
#include <mutex>
#include <string>
#include <vector>
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

#pragma region PRIVATE NAMESPACE
namespace 
{
	template <typename T>
	concept integral_sort_lsb = std::integral<T> && sizeof(T) <= 4;

	template <typename T>
	concept integral_sort_msb = std::integral<T> && sizeof(T) > 4;

	template <typename T>
	concept string_sort = std::same_as<T, std::string>;
}
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

	struct RegionIntegral 
	{
		int l;
		int r;
		int len;
		RegionIntegral(int l, int r, int len) : l(l), r(r), len(len) {}
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

#pragma region TYPE CONVERTERS
	template <size_t S> struct t2u_imp;
	template <> struct t2u_imp<1> { using type = std::uint8_t;  };
	template <> struct t2u_imp<2> { using type = std::uint16_t; };
	template <> struct t2u_imp<4> { using type = std::uint32_t; };
	template <> struct t2u_imp<8> { using type = std::uint64_t; };

	template <typename T>
	using t2u = t2u_imp<sizeof(T)>::type;


#pragma endregion

#pragma region HELPERS
	template<typename T>
	static void insertionSort(std::vector<T>& v, int l, int r);

	template<typename T>
	static void getCountVectorThread(std::vector<T>& v, std::vector<int>& count, int curShiftOrIndex, int l, int r);

	template<typename T>
	static void getCountVector(std::vector<T>& v, std::vector<int>& count, int curShiftOrIndex, int l, int r);

	static int getChar(const std::string& s, int index);

	template<typename T, typename TRegion>
	static void sortInstance(std::vector<T>& v, std::vector<T>& tmp,
		std::vector<TRegion>& regions, std::unique_lock<std::mutex>& lkRegions,
		TRegion initialRegion, bool multiThreaded);

	template<typename T, typename TRegion>
	static void sortThread(std::vector<T>& v, std::vector<T>& tmp,
		std::vector<TRegion>& regions, std::mutex& regionsLock,
		std::atomic<int>& runningCounter, int threadIndex);
#pragma endregion

#pragma region IMPLEMENTATIONS
	template<integral_sort_lsb T>
	static void sort_INT_UINT(std::vector<T>& v);

	template<typename T>
	static void sort_FLOAT_DOUBLE(std::vector<T>& v);

	template<typename T>
	static void sort_LL_ULL_STRING(std::vector<T>& v);
#pragma endregion

public:
	template<typename T>
	static void sort(std::vector<T>& v);
};
