#include "radix_sort.hpp"
#include <algorithm>
#include <thread>
#include <compare>
#include <atomic>
#include <type_traits>
using namespace std;

template<typename T>
void radix_sort::insertionSort(vector<T>& v, int l, int r)
{
	for (int i = l + 1; i < r; i++)
	{
		T val = move(v[i]);
		int j = i - 1;

		if constexpr (is_same_v<T, float> || is_same_v<T, double>)
		{
			while (j >= l && strong_order(val, v[j]) < 0)
			{
				v[j + 1] = move(v[j]);
				j--;
			}
		}
		else
		{
			while (j >= l && val < v[j])
			{
				v[j + 1] = move(v[j]);
				j--;
			}
		}

		v[j + 1] = move(val);
	}
}

#pragma region getcountTHREAD
template<typename T>
void radix_sort::getCountVectorThread(vector<T>& v, vector<int>& count, int curShiftOrIndex, int l, int r)
{
	if constexpr (is_same_v<T, unsigned int> || is_same_v<T, ull>)
	{
		for (int i = l; i < r; i++)
			count[(v[i] >> curShiftOrIndex) & MASK]++;
	}
	else if constexpr (is_same_v<T, int>)
	{
		if (curShiftOrIndex != 24)
		{
			for (int i = l; i < r; i++)
				count[(static_cast<unsigned int>(v[i]) >> curShiftOrIndex) & MASK]++;
		}
		else
		{
			for (int i = l; i < r; i++)
				count[((static_cast<unsigned int>(v[i]) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++;
		}
	}
	else if constexpr (is_same_v<T, ll>)
	{
		if (curShiftOrIndex != 56)
		{
			for (int i = l; i < r; i++)
				count[(static_cast<ull>(v[i]) >> curShiftOrIndex) & MASK]++;
		}
		else
		{
			for (int i = l; i < r; i++)
				count[((static_cast<ull>(v[i]) >> curShiftOrIndex) & MASK) ^ INVERT_MASK]++;
		}
	}
	else if constexpr (is_same_v<T, string>)
	{
		for (int i = l; i < r; i++)
			count[getChar(v[i], curShiftOrIndex)]++;
	}
}
#pragma endregion

#pragma region getcount
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
		constexpr int ALLOC_SIZE = (is_same_v<T, string>) ? CHARS_ALLOC : BASE;
		
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
#pragma endregion

#pragma region int uint
template<typename T>
void radix_sort::sort1(std::vector<T>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}
	
	int n = 4;

	if constexpr (is_same_v<T, unsigned int>)
	{
		n = 0;
		unsigned int maxVal = *max_element(v.begin(), v.end());

		while (maxVal > 0)
		{
			n++;
			maxVal >>= SHIFT_BITS;
		}
	}

	vector<T> tmp(v.size());
	int curShift = 0;

	while (n--)
	{
		vector<int> count(BASE);
		vector<int> prefix(BASE);

		getCountVector(v, count, curShift, 0, v.size());

		for (int i = 1; i < BASE; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		if constexpr (is_same_v<T, unsigned int>)
		{
			for (const auto& num : v)
				tmp[prefix[(num >> curShift) & MASK]++] = num;
		}
		else if constexpr (is_same_v<T, int>)
		{
			if (curShift != 24)
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
#pragma endregion

#pragma region float double
template<typename T>
void radix_sort::sort2(std::vector<T>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	const int SIZE = v.size();

	if constexpr (is_same_v<T, float>)
	{
		vector<unsigned int> vUint(SIZE);

		for (int i = 0; i < SIZE; i++)
		{
			memcpy(&vUint[i], &v[i], sizeof(float));
			vUint[i] = (vUint[i] >> 31) ? ~vUint[i] : vUint[i] ^ 0x80000000;
		}

		sort(vUint);

		for (int i = 0; i < SIZE; i++)
		{
			vUint[i] = (vUint[i] >> 31) ? vUint[i] ^ 0x80000000 : ~vUint[i];
			memcpy(&v[i], &vUint[i], sizeof(float));
		}
	}
	else if constexpr (is_same_v<T, double>)
	{
		vector<ull> vULL(SIZE);

		for (int i = 0; i < SIZE; i++)
		{
			memcpy(&vULL[i], &v[i], sizeof(double));
			vULL[i] = (vULL[i] >> 63) ? ~vULL[i] : vULL[i] ^ 0x8000000000000000;
		}

		sort(vULL);

		for (int i = 0; i < SIZE; i++)
		{
			vULL[i] = (vULL[i] >> 63) ? vULL[i] ^ 0x8000000000000000 : ~vULL[i];
			memcpy(&v[i], &vULL[i], sizeof(double));
		}
	}
}
#pragma endregion

#pragma region ll
void radix_sort::sortLL(std::vector<ll>& v, std::vector<ll>& tmp, 
	std::vector<RegionLL>& regions, std::unique_lock<std::mutex>& lkRegions, 
	RegionLL initialRegion, bool multiThreaded)
{
	vector<RegionLL> regionsLocal;
	regionsLocal.reserve(v.size() / 100);
	regionsLocal.emplace_back(initialRegion);

	while (regionsLocal.size())
	{
		RegionLL region = move(regionsLocal.back());
		regionsLocal.pop_back();

		int l = region.l;
		int r = region.r;
		int len = region.len;

		if (r - l < 2 || len == 0)
			continue;

		if (r - l <= 100)
		{
			insertionSort(v, l, r);
			continue;
		}

		const int CUR_SHIFT = (len - 1) * SHIFT_BITS;
		vector<int> count(BASE);
		vector<int> prefix(BASE);

		getCountVector(v, count, CUR_SHIFT, l, r);

		prefix[0] = l;
		for (int i = 1; i < BASE; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];
		
		if (CUR_SHIFT != 56)
		{
			for (int i = l; i < r; i++)
				tmp[prefix[(static_cast<ull>(v[i]) >> CUR_SHIFT) & MASK]++] = v[i];
		}
		else
		{
			for (int i = l; i < r; i++)
				tmp[prefix[((static_cast<ull>(v[i]) >> CUR_SHIFT) & MASK) ^ INVERT_MASK]++] = v[i];
		}

		move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

		len--;

		if (len == 0)
			continue;

		if (!multiThreaded)
		{
			for (int i = 0, start = l; i < BASE; i++)
			{
				if (count[i])
					regionsLocal.emplace_back(start, start + count[i], len);
				start += count[i];
			}
		}
		else
		{
			const int BUCKET_THRESHOLD = max((r - l) / 1000, 1000);

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

void radix_sort::sortThreadLL(std::vector<ll>& v, std::vector<ll>& tmp,
	std::vector<RegionLL>& regions, std::mutex& regionsLock, 
	std::atomic<int>& runningCounter, int threadIndex)
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
			RegionLL region = move(regions.back());
			regions.pop_back();
			lkRegions.unlock();

			if (isIdle)
			{
				isIdle = false;
				runningCounter++;
				iterationsIdle = 0;
			}

			sortLL(v, tmp, regions, lkRegions, region, 1);
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

void radix_sort::sortLL(std::vector<ll>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	vector<ll> tmp(v.size());

	constexpr int SHIFT_BITS = 8;
	int len = 8;

	int numOfThreads = thread::hardware_concurrency();
	const int BUCKET_THRESHOLD = max(static_cast<int>(v.size() / 1000), 1000);

	if (static_cast<int>(v.size() / 256) - (BUCKET_THRESHOLD / 10) < BUCKET_THRESHOLD)
	{
		vector<RegionLL> tmpVector;
		mutex tmpMutex;
		unique_lock<mutex> tmpLock(tmpMutex, defer_lock);

		sortLL(v, tmp, tmpVector, tmpLock, RegionLL(0, v.size(), len), 0);
	}
	else
	{
		vector<RegionLL> regions;
		regions.reserve(v.size() / 1000);
		regions.emplace_back(0, v.size(), len);

		mutex regionsLock;
		mutex idleLock;
		atomic<int> runningCounter = numOfThreads;

		vector<thread> threads;

		for (int i = 0; i < numOfThreads; i++)
			threads.emplace_back(sortThreadLL, ref(v), ref(tmp), ref(regions), ref(regionsLock), ref(runningCounter), i);

		for (auto& t : threads)
			t.join();
	}
}
#pragma endregion

#pragma region ull
void radix_sort::sortULL(std::vector<ull>& v, std::vector<ull>& tmp, 
	std::vector<RegionLL>& regions, std::unique_lock<std::mutex>& lkRegions, 
	RegionLL initialRegion, bool multiThreaded)
{
	vector<RegionLL> regionsLocal;
	regionsLocal.reserve(v.size() / 100);
	regionsLocal.emplace_back(initialRegion);

	while (regionsLocal.size())
	{
		RegionLL region = move(regionsLocal.back());
		regionsLocal.pop_back();

		int l = region.l;
		int r = region.r;
		int len = region.len;

		if (r - l < 2 || len == 0)
			continue;

		if (r - l <= 100)
		{
			insertionSort(v, l, r);
			continue;
		}

		const int CUR_SHIFT = (len - 1) * SHIFT_BITS;
		vector<int> count(BASE);
		vector<int> prefix(BASE);

		getCountVector(v, count, CUR_SHIFT, l, r);

		prefix[0] = l;
		for (int i = 1; i < BASE; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		for (int i = l; i < r; i++)
			tmp[prefix[(v[i] >> CUR_SHIFT) & MASK]++] = v[i];

		move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

		len--;

		if (len == 0)
			continue;

		if (!multiThreaded)
		{
			for (int i = 0, start = l; i < BASE; i++)
			{
				if (count[i])
					regionsLocal.emplace_back(start, start + count[i], len);
				start += count[i];
			}
		}
		else
		{
			const int BUCKET_THRESHOLD = max((r - l) / 1000, 1000);

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

void radix_sort::sortThreadULL(std::vector<ull>& v, std::vector<ull>& tmp,
	std::vector<RegionLL>& regions, std::mutex& regionsLock, 
	std::atomic<int>& runningCounter, int threadIndex)
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
			RegionLL region = move(regions.back());
			regions.pop_back();
			lkRegions.unlock();

			if (isIdle)
			{
				isIdle = false;
				runningCounter++;
				iterationsIdle = 0;
			}

			sortULL(v, tmp, regions, lkRegions, region, 1);
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

void radix_sort::sortULL(std::vector<ull>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	vector<ull> tmp(v.size());

	constexpr int SHIFT_BITS = 8;
	int len = 0;
	ull maxVal = *max_element(v.begin(), v.end());

	while (maxVal > 0)
	{
		len++;
		maxVal >>= SHIFT_BITS;
	}

	int numOfThreads = thread::hardware_concurrency();
	const int BUCKET_THRESHOLD = max(static_cast<int>(v.size() / 1000), 1000);

	if (static_cast<int>(v.size() / 256) - (BUCKET_THRESHOLD / 10) < BUCKET_THRESHOLD)
	{
		vector<RegionLL> tmpVector;
		mutex tmpMutex;
		unique_lock<mutex> tmpLock(tmpMutex, defer_lock);

		sortULL(v, tmp, tmpVector, tmpLock, RegionLL(0, v.size(), len), 0);
	}
	else
	{
		vector<RegionLL> regions;
		regions.reserve(v.size() / 1000);
		regions.emplace_back(0, v.size(), len);

		mutex regionsLock;
		mutex idleLock;
		atomic<int> runningCounter = numOfThreads;

		vector<thread> threads;

		for (int i = 0; i < numOfThreads; i++)
			threads.emplace_back(sortThreadULL, ref(v), ref(tmp), ref(regions), ref(regionsLock), ref(runningCounter), i);

		for (auto& t : threads)
			t.join();
	}
}
#pragma endregion

#pragma region string
int radix_sort::getChar(const string& s, int index)
{
	return (index < s.length()) ? static_cast<unsigned char>(s[index]) : 256;
}

void radix_sort::sortString(vector<string>& v, vector<string>& tmp, vector<RegionString>& regions,
	unique_lock<mutex>& lkRegions, RegionString initialRegion, bool multiThreaded)
{
	vector<RegionString> regionsLocal;
	regionsLocal.reserve(v.size() / 100);
	regionsLocal.emplace_back(initialRegion);

	while (regionsLocal.size())
	{
		RegionString region = move(regionsLocal.back());
		regionsLocal.pop_back();

		int l = region.l;
		int r = region.r;
		int len = region.len;
		int curIndex = region.curIndex;

		if (r - l < 2 || len == 0)
			continue;

		if (r - l <= 10)
		{
			insertionSort(v, l, r);
			continue;
		}

		constexpr int CHARS_ALLOC = 257;

		vector<int> count(CHARS_ALLOC);
		vector<int> prefix(CHARS_ALLOC);

		constexpr int CHARS = 256;

		getCountVector(v, count, curIndex, l, r);

		prefix[256] = l;
		prefix[0] = prefix[256] + count[256];
		for (int i = 1; i < CHARS; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		for (int i = l; i < r; i++)
			tmp[prefix[getChar(v[i], curIndex)]++] = move(v[i]);

		move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

		len--;
		curIndex++;

		if (len == 0)
			continue;

		if (!multiThreaded)
		{
			for (int i = 0, start = l + count[256]; i < CHARS; i++)
			{
				if (count[i] > 1)
					regionsLocal.emplace_back(start, start + count[i], len, curIndex);
				start += count[i];
			}
		}
		else
		{
			const int BUCKET_THRESHOLD = max((r - l) / 1000, 1000);

			for (int i = 0, start = l + count[256]; i < CHARS; i++)
			{
				if (count[i] < BUCKET_THRESHOLD)
					regionsLocal.emplace_back(start, start + count[i], len, curIndex);
				else
				{
					lkRegions.lock();
					regions.emplace_back(start, start + count[i], len, curIndex);
					lkRegions.unlock();
				}
				start += count[i];
			}
		}
	}
}

void radix_sort::sortThreadString(vector<string>& v, vector<string>& tmp,
	vector<RegionString>& regions, mutex& regionsLock, 
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
			RegionString region = move(regions.back());
			regions.pop_back();
			lkRegions.unlock();

			if (isIdle)
			{
				isIdle = false;
				runningCounter++;
				iterationsIdle = 0;
			}

			sortString(v, tmp, regions, lkRegions, region, 1);
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

void radix_sort::sortString(vector<string>& v)
{
	if (v.size() <= 10)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	vector<string> tmp(v.size());

	int len = (*max_element(v.begin(), v.end(),
		[](const string& a, const string& b) {
			return a.length() < b.length();
		})).length();

	int numOfThreads = thread::hardware_concurrency();
	const int BUCKET_THRESHOLD = max(static_cast<int>(v.size() / 1000), 1000);

	if (static_cast<int>(v.size() / 256) - (BUCKET_THRESHOLD / 10) < BUCKET_THRESHOLD)
	{
		vector<RegionString> tmpVector;
		mutex tmpMutex;
		unique_lock<mutex> tmpLock(tmpMutex, defer_lock);

		sortString(v, tmp, tmpVector, tmpLock, RegionString(0, v.size(), len, 0), 0);
	}
	else
	{
		vector<RegionString> regions;
		regions.reserve(v.size() / 1000);
		regions.emplace_back(0, v.size(), len, 0);

		mutex regionsLock;
		mutex idleLock;
		atomic<int> runningCounter = numOfThreads;

		vector<thread> threads;

		for (int i = 0; i < numOfThreads; i++)
			threads.emplace_back(sortThreadString, ref(v), ref(tmp), ref(regions), ref(regionsLock), ref(runningCounter), i);

		for (auto& t : threads)
			t.join();
	}
}
#pragma endregion

template<typename T>
void radix_sort::sort(vector<T>& v)
{
	if constexpr (is_same_v<T, int> || is_same_v<T, unsigned int>)
		return sort1(v);
	else if constexpr (is_same_v<T, long long>)
		return sortLL(v);
	else if constexpr (is_same_v<T, unsigned long long>)
		return sortULL(v);
	else if constexpr (is_same_v<T, float> || is_same_v<T, double>)
		return sort2(v);
	else if constexpr (is_same_v<T, string>)
		return sortString(v);
}

template void radix_sort::sort<int>(vector<int>& v);
template void radix_sort::sort<unsigned int>(vector<unsigned int>& v);
template void radix_sort::sort<radix_sort::ll>(vector<radix_sort::ll>& v);
template void radix_sort::sort<radix_sort::ull>(vector<radix_sort::ull>& v);
template void radix_sort::sort<float>(vector<float>& v);
template void radix_sort::sort<double>(vector<double>& v);
template void radix_sort::sort<string>(vector<string>& v);