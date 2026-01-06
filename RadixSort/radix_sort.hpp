#pragma once
#pragma region HEADERS
#include <atomic>
#include <algorithm>
#include <compare>
#include <concepts>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
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

	template <typename T>
	concept unknown = !std::integral<T> && !std::floating_point<T> && !string_sort<T>;
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
	static void insertionSort(std::vector<T>& v, int l, int r)
	{
		constexpr auto cmp = (std::floating_point<T>) ?
			[](const T& a, const T& b) { return std::strong_order(a, b) < 0; } :
			[](const T& a, const T& b) { return a < b; };

		for (int i = l + 1; i < r; i++)
		{
			T val = std::move(v[i]);
			int j = i - 1;

			while (j >= l && cmp(val, v[j]))
			{
				v[j + 1] = std::move(v[j]);
				j--;
			}

			v[j + 1] = std::move(val);
		}
	}

	template<typename T>
	static void getCountVectorThread(std::vector<T>& v, std::vector<int>& count, int curShiftOrIndex, int l, int r)
	{
		if constexpr (std::same_as<T, std::string>)
		{
			for (int i = l; i < r; i++)
				count[getChar(v[i], curShiftOrIndex)]++;
		}
		else if constexpr (std::unsigned_integral<T>)
		{
			for (int i = l; i < r; i++)
				count[(v[i] >> curShiftOrIndex) & MASK]++;
		}
		else if constexpr (std::signed_integral<T>)
		{
			constexpr int MAX_SHIFT = (sizeof(T) - 1) * 8;

			if (curShiftOrIndex != MAX_SHIFT)
			{
				for (int i = l; i < r; i++)
					count[(static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK]++;
			}
			else
			{
				for (int i = l; i < r; i++)
					count[((static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++;
			}
		}
	}

	template<typename T>
	static void getCountVector(std::vector<T>& v, std::vector<int>& count, int curShiftOrIndex, int l, int r)
	{
		const int SIZE = r - l;
		int threadCount = static_cast<int>(ceil(SIZE / BUCKET_THRESHOLD));

		if (threadCount <= 1 || SIZE < SIZE_THRESHOLD)
		{
			getCountVectorThread(v, count, curShiftOrIndex, l, r);
		}
		else
		{
			constexpr int ALLOC_SIZE = (std::same_as<T, std::string>) ? CHARS_ALLOC : BASE;

			threadCount = std::min(threadCount, static_cast<int>(std::thread::hardware_concurrency()));
			int bucketSize = SIZE / threadCount;

			std::vector<std::thread> threads;
			std::vector<std::vector<int>> counts(threadCount, std::vector<int>(ALLOC_SIZE));

			for (int i = 0; i < threadCount; i++)
			{
				int start = l + i * bucketSize;
				int end = (i == threadCount - 1) ? r : start + bucketSize;
				threads.emplace_back(getCountVectorThread<T>, std::ref(v), std::ref(counts[i]), curShiftOrIndex, start, end);
			}

			for (auto& t : threads)
				t.join();

			for (int curThread = 0; curThread < threadCount; curThread++)
			{
				for (int i = 0; i < ALLOC_SIZE; i++)
					count[i] += counts[curThread][i];
			}
		}
	}

	static int getChar(const std::string& s, int index)
	{
		return (index < s.length()) ? static_cast<unsigned char>(s[index]) : 256;
	}

	template<typename T, typename TRegion>
	static void sortInstance(std::vector<T>& v, std::vector<T>& tmp,
		std::vector<TRegion>& regions, std::unique_lock<std::mutex>& lkRegions,
		TRegion initialRegion, bool multiThreaded)
	{
		std::vector<TRegion> regionsLocal;
		regionsLocal.reserve(v.size() / 100);
		regionsLocal.emplace_back(initialRegion);

		constexpr int INSERTION_SORT_THRESHOLD = (std::same_as<T, std::string>) ? 10 : 100;
		constexpr int ALLOC_SIZE = (std::same_as<T, std::string>) ? CHARS_ALLOC : BASE;

		while (regionsLocal.size())
		{
			TRegion region = std::move(regionsLocal.back());
			regionsLocal.pop_back();

			int l = region.l;
			int r = region.r;
			int len = region.len;
			int curShiftOrIndex;

			if constexpr (std::same_as<TRegion, RegionString>)
				curShiftOrIndex = region.curIndex;
			else
				curShiftOrIndex = (len - 1) * SHIFT_BITS;

			if (r - l < 2 || len == 0)
				continue;

			if (r - l <= INSERTION_SORT_THRESHOLD)
			{
				insertionSort(v, l, r);
				continue;
			}

			std::vector<int> count(ALLOC_SIZE);
			std::vector<int> prefix(ALLOC_SIZE);

			getCountVector(v, count, curShiftOrIndex, l, r);

			if constexpr (std::same_as<T, std::string>)
			{
				prefix[256] = l;
				prefix[0] = prefix[256] + count[256];
			}
			else
			{
				prefix[0] = l;
			}

			for (int i = 1; i < BASE; i++)
				prefix[i] = prefix[i - 1] + count[i - 1];

			if constexpr (std::same_as<T, std::string>)
			{
				for (int i = l; i < r; i++)
					tmp[prefix[getChar(v[i], curShiftOrIndex)]++] = move(v[i]);
			}
			else if constexpr (std::unsigned_integral<T>)
			{
				for (int i = l; i < r; i++)
					tmp[prefix[(v[i] >> curShiftOrIndex) & MASK]++] = v[i];
			}
			else if constexpr (std::signed_integral<T>)
			{
				constexpr int MAX_SHIFT = (sizeof(T) - 1) * 8;

				if (curShiftOrIndex != MAX_SHIFT)
				{
					for (int i = l; i < r; i++)
						tmp[prefix[(static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK]++] = v[i];
				}
				else
				{
					for (int i = l; i < r; i++)
						tmp[prefix[((static_cast<t2u<T>>(v[i]) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++] = v[i];
				}
			}

			std::move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

			len--;
			if (len == 0)
				continue;

			if constexpr (std::same_as<T, std::string>)
				curShiftOrIndex++;

			if (!multiThreaded)
			{
				if constexpr (std::same_as<T, std::string>)
				{
					for (int i = 0, start = l + count[256]; i < CHARS; i++)
					{
						if (count[i] > 1)
							regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
						start += count[i];
					}
				}
				else
				{
					for (int i = 0, start = l; i < BASE; i++)
					{
						if (count[i] > 1)
							regionsLocal.emplace_back(start, start + count[i], len);
						start += count[i];
					}
				}
			}
			else
			{
				const int BUCKET_THRESHOLD = std::max((r - l) / 1000, 1000);

				if constexpr (std::same_as<T, std::string>)
				{
					for (int i = 0, start = l + count[256]; i < CHARS; i++)
					{
						if (count[i] < BUCKET_THRESHOLD)
							regionsLocal.emplace_back(start, start + count[i], len, curShiftOrIndex);
						else
						{
							lkRegions.lock();
							regions.emplace_back(start, start + count[i], len, curShiftOrIndex);
							lkRegions.unlock();
						}
						start += count[i];
					}
				}
				else
				{
					for (int i = 0, start = l; i < BASE; i++)
					{
						if (count[i] < BUCKET_THRESHOLD)
							regionsLocal.emplace_back(start, start + count[i], len);
						else
						{
							lkRegions.lock();
							regions.emplace_back(start, start + count[i], len);
							lkRegions.unlock();
						}
						start += count[i];
					}
				}
			}
		}
	}

	template<typename T, typename TRegion>
	static void sortThread(std::vector<T>& v, std::vector<T>& tmp,
		std::vector<TRegion>& regions, std::mutex& regionsLock,
		std::atomic<int>& runningCounter, int threadIndex)
	{
		std::unique_lock<std::mutex> lkRegions(regionsLock, std::defer_lock);

		bool isIdle = false;
		int iterationsIdle = 0;
		const bool ALLOW_SLEEP = v.size() > 1e6;

		while (true)
		{
			lkRegions.lock();
			if (regions.size())
			{
				TRegion region = std::move(regions.back());
				regions.pop_back();
				lkRegions.unlock();

				if (isIdle)
				{
					isIdle = false;
					runningCounter++;
					iterationsIdle = 0;
				}

				sortInstance(v, tmp, regions, lkRegions, region, 1);
			}
			else
			{
				lkRegions.unlock();

				if (!isIdle)
				{
					isIdle = true;
					runningCounter--;
				}

				if (runningCounter.load() == 0)
					break;

				iterationsIdle++;

				if (ALLOW_SLEEP && iterationsIdle > 100)
				{
					iterationsIdle = 0;
					std::this_thread::sleep_for(std::chrono::nanoseconds(1));
				}
			}
		}
	}
#pragma endregion

public:
#pragma region IMPLEMENTATIONS
	template<integral_sort_lsb T>
	static void sort(std::vector<T>& v)
	{
		if (v.size() <= 100)
		{
			insertionSort(v, 0, v.size());
			return;
		}

		int n = sizeof(T);

		if constexpr (std::unsigned_integral<T>)
		{
			n = 0;
			T maxVal = *std::max_element(v.begin(), v.end());

			while (maxVal > 0)
			{
				n++;
				maxVal >>= SHIFT_BITS;
			}
		}

		std::vector<T> tmp(v.size());
		int curShift = 0;
		constexpr int MAX_SHIFT = (sizeof(T) - 1) * 8;

		while (n--)
		{
			std::vector<int> count(BASE);
			std::vector<int> prefix(BASE);

			getCountVector(v, count, curShift, 0, v.size());

			for (int i = 1; i < BASE; i++)
				prefix[i] = prefix[i - 1] + count[i - 1];

			if constexpr (std::unsigned_integral<T>)
			{
				for (const auto& num : v)
					tmp[prefix[(num >> curShift) & MASK]++] = num;
			}
			else if constexpr (std::signed_integral<T>)
			{
				if (curShift != MAX_SHIFT)
				{
					for (const auto& num : v)
						tmp[prefix[(static_cast<t2u<T>>(num) >> curShift) & MASK]++] = num;
				}
				else
				{
					for (const auto& num : v)
						tmp[prefix[((static_cast<t2u<T>>(num) >> curShift) & MASK) ^ INVERT_MASK]++] = num;
				}
			}

			swap(v, tmp);

			curShift += SHIFT_BITS;
		}
	}

	template<std::floating_point T>
	static void sort(std::vector<T>& v)
	{
		static_assert(
			std::numeric_limits<T>::is_iec559,
			"ERROR: generators.hpp requires IEEE-754/IEC-559 compliance.\n"
			);

		if (v.size() <= 100)
		{
			insertionSort(v, 0, v.size());
			return;
		}

		const int SIZE = v.size();

		using U = t2u<T>;
		constexpr int SIGN_SHIFT = (sizeof(T) * 8) - 1;
		constexpr U SIGN_MASK = 1LL << SIGN_SHIFT;

		std::vector<U> vu(SIZE);

		for (int i = 0; i < SIZE; i++)
		{
			std::memcpy(&vu[i], &v[i], sizeof(T));
			vu[i] = (vu[i] >> SIGN_SHIFT) ? ~vu[i] : vu[i] ^ SIGN_MASK;
		}

		sort(vu);

		for (int i = 0; i < SIZE; i++)
		{
			vu[i] = (vu[i] >> SIGN_SHIFT) ? vu[i] ^ SIGN_MASK : ~vu[i];
			std::memcpy(&v[i], &vu[i], sizeof(T));
		}
	}

	template<typename T> requires (integral_sort_msb<T> || string_sort<T>)
	static void sort(std::vector<T>& v)
	{
		constexpr int INSERTION_SORT_THRESHOLD = (std::same_as<T, std::string>) ? 10 : 100;
		if (v.size() <= INSERTION_SORT_THRESHOLD)
		{
			insertionSort(v, 0, v.size());
			return;
		}

		int len = 0;
		if constexpr (std::signed_integral<T>)
		{
			len = sizeof(T);
		}
		else if constexpr (std::unsigned_integral<T>)
		{
			T maxVal = *std::max_element(v.begin(), v.end());

			while (maxVal > 0)
			{
				len++;
				maxVal >>= SHIFT_BITS;
			}
		}
		else
		{
			len = (*std::max_element(v.begin(), v.end(),
				[](const std::string& a, const std::string& b) {
					return a.length() < b.length();
				})).length();
		}

		int numOfThreads = std::thread::hardware_concurrency();
		const int BUCKET_THRESHOLD = std::max(static_cast<int>(v.size() / 1000), 1000);
		std::vector<T> tmp(v.size());

		if (static_cast<int>(v.size() / 256) - (BUCKET_THRESHOLD / 10) < BUCKET_THRESHOLD)
		{
			std::mutex tmpMutex;
			std::unique_lock<std::mutex> tmpLock(tmpMutex, std::defer_lock);

			if constexpr (std::same_as<T, std::string>)
			{
				std::vector<RegionString> tmpVector;
				sortInstance(v, tmp, tmpVector, tmpLock, RegionString(0, v.size(), len, 0), 0);
			}
			else
			{
				std::vector<RegionIntegral> tmpVector;
				sortInstance(v, tmp, tmpVector, tmpLock, RegionIntegral(0, v.size(), len), 0);
			}
		}
		else
		{
			std::vector<std::thread> threads;
			std::mutex regionsLock;
			std::mutex idleLock;
			std::atomic<int> runningCounter = numOfThreads;

			if constexpr (std::same_as<T, std::string>)
			{
				std::vector<RegionString> regions;
				regions.reserve(v.size() / 1000);
				regions.emplace_back(0, v.size(), len, 0);
				for (int i = 0; i < numOfThreads; i++)
					threads.emplace_back(sortThread<T, RegionString>, std::ref(v), std::ref(tmp), std::ref(regions), std::ref(regionsLock), std::ref(runningCounter), i);

				for (auto& t : threads)
					t.join();
			}
			else
			{
				std::vector<RegionIntegral> regions;
				regions.reserve(v.size() / 1000);
				regions.emplace_back(0, v.size(), len);
				for (int i = 0; i < numOfThreads; i++)
					threads.emplace_back(sortThread<T, RegionIntegral>, std::ref(v), std::ref(tmp), std::ref(regions), std::ref(regionsLock), std::ref(runningCounter), i);

				for (auto& t : threads)
					t.join();
			}
		}
	}

	template<unknown T>
	static void sort(std::vector<T>& v)
	{
		static_assert(sizeof(T) == 0, "ERROR: Unable to sort vector!\nCAUSE: Unsupported type!\n");
	}
#pragma endregion
};
