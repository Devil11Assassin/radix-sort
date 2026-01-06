#pragma region HEADERS
#include "radix_sort.hpp"

#include <atomic>
#include <algorithm>
#include <compare>
#include <thread>
#include <type_traits>

using namespace std;
#pragma endregion

#pragma region HELPERS
template<typename T>
void radix_sort::insertionSort(vector<T>& v, int l, int r)
{
	constexpr auto cmp = (floating_point<T>) ?
		[](const T& a, const T& b) { return strong_order(a, b) < 0; } :
		[](const T& a, const T& b) { return a < b; };

	for (int i = l + 1; i < r; i++)
	{
		T val = move(v[i]);
		int j = i - 1;

		while (j >= l && cmp(val, v[j]))
		{
			v[j + 1] = move(v[j]);
			j--;
		}

		v[j + 1] = move(val);
	}
}

template<typename T>
void radix_sort::getCountVectorThread(vector<T>& v, vector<int>& count, int curShiftOrIndex, int l, int r)
{
	if constexpr (same_as<T, string>)
	{
		for (int i = l; i < r; i++)
			count[getChar(v[i], curShiftOrIndex)]++;
	}
	else if constexpr (unsigned_integral<T>)
	{
		for (int i = l; i < r; i++)
			count[(v[i] >> curShiftOrIndex) & MASK]++;
	}
	else if constexpr (signed_integral<T>)
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
void radix_sort::getCountVector(vector<T>& v, vector<int>& count, int curShiftOrIndex, int l, int r)
{
	const int SIZE = r - l;
	int threadCount = static_cast<int>(ceil(SIZE / BUCKET_THRESHOLD));

	if (threadCount <= 1 || SIZE < SIZE_THRESHOLD)
	{
		getCountVectorThread(v, count, curShiftOrIndex, l, r);
	}
	else
	{
		constexpr int ALLOC_SIZE = (same_as<T, string>) ? CHARS_ALLOC : BASE;

		threadCount = min(threadCount, static_cast<int>(thread::hardware_concurrency()));
		int bucketSize = SIZE / threadCount;

		vector<thread> threads;
		vector<vector<int>> counts(threadCount, vector<int>(ALLOC_SIZE));

		for (int i = 0; i < threadCount; i++)
		{
			int start = l + i * bucketSize;
			int end = (i == threadCount - 1) ? r : start + bucketSize;
			threads.emplace_back(getCountVectorThread<T>, ref(v), ref(counts[i]), curShiftOrIndex, start, end);
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

int radix_sort::getChar(const string& s, int index)
{
	return (index < s.length()) ? static_cast<unsigned char>(s[index]) : 256;
}

template<typename T, typename TRegion>
void radix_sort::sortInstance(vector<T>& v, vector<T>& tmp,
	vector<TRegion>& regions, unique_lock<mutex>& lkRegions,
	TRegion initialRegion, bool multiThreaded)
{
	vector<TRegion> regionsLocal;
	regionsLocal.reserve(v.size() / 100);
	regionsLocal.emplace_back(initialRegion);

	constexpr int INSERTION_SORT_THRESHOLD = (same_as<T, string>) ? 10 : 100;
	constexpr int ALLOC_SIZE = (same_as<T, string>) ? CHARS_ALLOC : BASE;

	while (regionsLocal.size())
	{
		TRegion region = move(regionsLocal.back());
		regionsLocal.pop_back();

		int l = region.l;
		int r = region.r;
		int len = region.len;
		int curShiftOrIndex;

		if constexpr (same_as<TRegion, RegionString>)
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

		vector<int> count(ALLOC_SIZE);
		vector<int> prefix(ALLOC_SIZE);

		getCountVector(v, count, curShiftOrIndex, l, r);

		if constexpr (same_as<T, string>)
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

		if constexpr (same_as<T, string>)
		{
			for (int i = l; i < r; i++)
				tmp[prefix[getChar(v[i], curShiftOrIndex)]++] = move(v[i]);
		}
		else if constexpr (unsigned_integral<T>)
		{
			for (int i = l; i < r; i++)
				tmp[prefix[(v[i] >> curShiftOrIndex) & MASK]++] = v[i];
		}
		else if constexpr (signed_integral<T>)
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

		move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

		len--;
		if (len == 0)
			continue;

		if constexpr (same_as<T, string>)
			curShiftOrIndex++;

		if (!multiThreaded)
		{
			if constexpr (same_as<T, string>)
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
			const int BUCKET_THRESHOLD = max((r - l) / 1000, 1000);

			if constexpr (same_as<T, string>)
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
void radix_sort::sortThread(vector<T>& v, vector<T>& tmp,
	vector<TRegion>& regions, std::mutex& regionsLock,
	atomic<int>& runningCounter, int threadIndex)
{
	unique_lock<mutex> lkRegions(regionsLock, defer_lock);

	bool isIdle = false;
	int iterationsIdle = 0;
	const bool ALLOW_SLEEP = v.size() > 1e6;

	while (true)
	{
		lkRegions.lock();
		if (regions.size())
		{
			TRegion region = move(regions.back());
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
				this_thread::sleep_for(chrono::nanoseconds(1));
			}
		}
	}
}
#pragma endregion

#pragma region IMPLEMENTATIONS
template<integral_sort_lsb T>
void radix_sort::sort_INT_UINT(std::vector<T>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}
	
	int n = sizeof(T);

	if constexpr (unsigned_integral<T>)
	{
		n = 0;
		T maxVal = *max_element(v.begin(), v.end());

		while (maxVal > 0)
		{
			n++;
			maxVal >>= SHIFT_BITS;
		}
	}

	vector<T> tmp(v.size());
	int curShift = 0;
	constexpr int MAX_SHIFT = (sizeof(T) - 1) * 8;

	while (n--)
	{
		vector<int> count(BASE);
		vector<int> prefix(BASE);

		getCountVector(v, count, curShift, 0, v.size());

		for (int i = 1; i < BASE; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		if constexpr (unsigned_integral<T>)
		{
			for (const auto& num : v)
				tmp[prefix[(num >> curShift) & MASK]++] = num;
		}
		else if constexpr (signed_integral<T>)
		{
			if (curShift != MAX_SHIFT)
			{
				for (const auto& num : v)
					tmp[prefix[(static_cast<unsigned int>(num) >> curShift) & MASK]++] = num;
			}
			else
			{
				for (const auto& num : v)
					tmp[prefix[((static_cast<unsigned int>(num) >> curShift) & MASK) ^ INVERT_MASK]++] = num;
			}
		}

		swap(v, tmp);

		curShift += SHIFT_BITS;
	}
}

template<typename T>
void radix_sort::sort_FLOAT_DOUBLE(std::vector<T>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	const int SIZE = v.size();
	
	using U = t2u<T>;
	constexpr int SIGN_SHIFT = (sizeof(T) * 8) - 1;
	constexpr U SIGN_MASK = 1LL << SIGN_SHIFT;

	vector<U> vu(SIZE);

	for (int i = 0; i < SIZE; i++)
	{
		memcpy(&vu[i], &v[i], sizeof(T));
		vu[i] = (vu[i] >> SIGN_SHIFT) ? ~vu[i] : vu[i] ^ SIGN_MASK;
	}

	sort(vu);

	for (int i = 0; i < SIZE; i++)
	{
		vu[i] = (vu[i] >> SIGN_SHIFT) ? vu[i] ^ SIGN_MASK : ~vu[i];
		memcpy(&v[i], &vu[i], sizeof(T));
	}
}

template<typename T>
void radix_sort::sort_LL_ULL_STRING(std::vector<T>& v)
{
	constexpr int INSERTION_SORT_THRESHOLD = (same_as<T, string>) ? 10 : 100;
	if (v.size() <= INSERTION_SORT_THRESHOLD)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	int len = 0;
	if constexpr (signed_integral<T>)
	{
		len = sizeof(T);
	}
	else if constexpr (unsigned_integral<T>)
	{
		T maxVal = *max_element(v.begin(), v.end());

		while (maxVal > 0)
		{
			len++;
			maxVal >>= SHIFT_BITS;
		}
	}
	else
	{
		len = (*max_element(v.begin(), v.end(),
			[](const string& a, const string& b) {
				return a.length() < b.length();
			})).length();
	}

	int numOfThreads = thread::hardware_concurrency();
	const int BUCKET_THRESHOLD = max(static_cast<int>(v.size() / 1000), 1000);
	vector<T> tmp(v.size());

	if (static_cast<int>(v.size() / 256) - (BUCKET_THRESHOLD / 10) < BUCKET_THRESHOLD)
	{
		mutex tmpMutex;
		unique_lock<mutex> tmpLock(tmpMutex, defer_lock);

		if constexpr (same_as<T, string>)
		{
			vector<RegionString> tmpVector;
			sortInstance(v, tmp, tmpVector, tmpLock, RegionString(0, v.size(), len, 0), 0);
		}
		else
		{
			vector<RegionIntegral> tmpVector;
			sortInstance(v, tmp, tmpVector, tmpLock, RegionIntegral(0, v.size(), len), 0);
		}
	}
	else
	{
		vector<thread> threads;
		mutex regionsLock;
		mutex idleLock;
		atomic<int> runningCounter = numOfThreads;

		if constexpr (same_as<T, string>)
		{
			vector<RegionString> regions;
			regions.reserve(v.size() / 1000);
			regions.emplace_back(0, v.size(), len, 0);
			for (int i = 0; i < numOfThreads; i++)
				threads.emplace_back(sortThread<T, RegionString>, ref(v), ref(tmp), ref(regions), ref(regionsLock), ref(runningCounter), i);

			for (auto& t : threads)
				t.join();
		}
		else
		{
			vector<RegionIntegral> regions;
			regions.reserve(v.size() / 1000);
			regions.emplace_back(0, v.size(), len);
			for (int i = 0; i < numOfThreads; i++)
				threads.emplace_back(sortThread<T, RegionIntegral>, ref(v), ref(tmp), ref(regions), ref(regionsLock), ref(runningCounter), i);

			for (auto& t : threads)
				t.join();
		}
	}
}
#pragma endregion

template<typename T>
void radix_sort::sort(vector<T>& v)
{
	//if constexpr (same_as<T, int> || same_as<T, unsigned int>)
	if constexpr (integral_sort_lsb<T>)
		return sort_INT_UINT(v);
	//else if constexpr (same_as<T, float> || same_as<T, double>)
	else if constexpr (floating_point<T>)
		return sort_FLOAT_DOUBLE(v);
	else if constexpr (integral_sort_msb<T> || string_sort<T>)
		return sort_LL_ULL_STRING(v);
	else
		static_assert(sizeof(T) == 0, "ERROR: Unable to sort vector!\nCAUSE: Unsupported type!\n");
}

template void radix_sort::sort<int>(vector<int>& v);
template void radix_sort::sort<unsigned int>(vector<unsigned int>& v);
template void radix_sort::sort<radix_sort::ll>(vector<radix_sort::ll>& v);
template void radix_sort::sort<radix_sort::ull>(vector<radix_sort::ull>& v);
template void radix_sort::sort<float>(vector<float>& v);
template void radix_sort::sort<double>(vector<double>& v);
template void radix_sort::sort<string>(vector<string>& v);