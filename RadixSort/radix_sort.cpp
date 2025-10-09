#include "radix_sort.hpp"
#include <algorithm>
#include <thread>
#include <compare>
#include <atomic>
#include <iostream>
using namespace std;

atomic<int> threadsCreated = 0;

#pragma region int
void radix_sort::getCountVectorThreadInt(vector<int>& v, vector<int>& count, int l, int r, int shiftBits, int mask, int invertMask)
{
	if (shiftBits != 24)
	{
		for (int i = l; i < r; i++)
			count[(static_cast<unsigned int>(v[i]) >> shiftBits) & mask]++;
	}
	else
	{
		for (int i = l; i < r; i++)
			count[((static_cast<unsigned int>(v[i]) >> shiftBits) & mask) ^ invertMask]++;
	}
}

void radix_sort::getCountVector(vector<int>& v, vector<int>& count, int shiftBits, int base, int mask, int invertMask)
{
	const int idealSize = 8000000;
	const double idealBucketSize = 1000000.0;

	int size = v.size();
	int threadCount = static_cast<int>(ceil(size / idealBucketSize));

	if (threadCount <= 1 || size < idealSize)
	{
		if (shiftBits != 24)
		{
			for (const auto& num : v)
				count[(static_cast<unsigned int>(num) >> shiftBits) & mask]++;
		}
		else
		{
			for (const auto& num : v)
				count[((static_cast<unsigned int>(num) >> shiftBits) & mask) ^ invertMask]++;
		}
	}
	else
	{
		threadCount = min(threadCount, static_cast<int>(thread::hardware_concurrency()));
		int bucketSize = size / threadCount;

		vector<thread> threads;
		vector<vector<int>> counts(threadCount, vector<int>(base));

		for (int i = 0; i < threadCount; i++)
		{
			int start = i * bucketSize;
			int end = (i == threadCount - 1) ? size : start + bucketSize;
			threads.emplace_back(getCountVectorThreadInt, ref(v), ref(counts[i]), start, end, shiftBits, mask, invertMask);
		}

		for (auto& t : threads)
			t.join();

		for (int curThread = 0; curThread < threadCount; curThread++)
		{
			for (int i = 0; i < base; i++)
				count[i] += counts[curThread][i];
		}
	}
}

void radix_sort::insertionSort(vector<int>& v, int l, int r)
{
	for (int i = l + 1; i < r; i++)
	{
		int val = v[i];
		int j = i - 1;

		while (j >= l && v[j] > val)
		{
			v[j + 1] = v[j];
			j--;
		}

		v[j + 1] = val;
	}
}

void radix_sort::sort(vector<int>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	const int shiftBits = 8;
	const int base = 256;
	const int mask = 0xFF;
	const int invertMask = 0x80;

	vector<int> tmp(v.size());
	int n = 4;
	int curShift = 0;

	while (n--)
	{
		vector<int> count(base);
		vector<int> prefix(base);

		getCountVector(v, count, curShift, base, mask, invertMask);

		for (int i = 1; i < base; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		if (curShift != 24)
		{
			for (const auto& num : v)
				tmp[prefix[(static_cast<unsigned int>(num) >> curShift) & mask]++] = num;
		}
		else
		{
			for (const auto& num : v)
				tmp[prefix[((static_cast<unsigned int>(num) >> curShift) & mask) ^ invertMask]++] = num;
		}

		swap(v, tmp);

		curShift += shiftBits;
	}
}
#pragma endregion

#pragma region uint
void radix_sort::getCountVectorThreadUnsignedInt(vector<unsigned int>& v, vector<int>& count, int l, int r, int shiftBits, int mask)
{
	for (int i = l; i < r; i++)
		count[(v[i] >> shiftBits) & mask]++;
}

void radix_sort::getCountVector(vector<unsigned int>& v, vector<int>& count, int shiftBits, int base, int mask)
{
	const int idealSize = 8000000;
	const double idealBucketSize = 1000000.0;

	int size = v.size();
	int threadCount = static_cast<int>(ceil(size / idealBucketSize));

	if (threadCount <= 1 || size < idealSize)
	{
		for (const auto& num : v)
			count[(num >> shiftBits) & mask]++;
	}
	else
	{
		threadCount = min(threadCount, static_cast<int>(thread::hardware_concurrency()));
		int bucketSize = size / threadCount;

		vector<thread> threads;
		vector<vector<int>> counts(threadCount, vector<int>(base));

		for (int i = 0; i < threadCount; i++)
		{
			int start = i * bucketSize;
			int end = (i == threadCount - 1) ? size : start + bucketSize;
			threads.emplace_back(getCountVectorThreadUnsignedInt, ref(v), ref(counts[i]), start, end, shiftBits, mask);
		}

		for (auto& t : threads)
			t.join();

		for (int curThread = 0; curThread < threadCount; curThread++)
		{
			for (int i = 0; i < base; i++)
				count[i] += counts[curThread][i];
		}
	}
}

void radix_sort::insertionSort(vector<unsigned int>& v, int l, int r)
{
	for (int i = l + 1; i < r; i++)
	{
		unsigned int val = v[i];
		int j = i - 1;

		while (j >= l && v[j] > val)
		{
			v[j + 1] = v[j];
			j--;
		}

		v[j + 1] = val;
	}
}

void radix_sort::sort(vector<unsigned int>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	const int shiftBits = 8;
	const int base = 256;
	const int mask = 0xFF;

	int n = 0;
	unsigned int maxVal = *max_element(v.begin(), v.end());

	while (maxVal > 0)
	{
		n++;
		maxVal >>= shiftBits;
	}

	vector<unsigned int> tmp(v.size());
	int curShift = 0;

	while (n--)
	{
		vector<int> count(base);
		vector<int> prefix(base);

		getCountVector(v, count, curShift, base, mask);

		for (int i = 1; i < base; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		for (const auto& num : v)
			tmp[prefix[(num >> curShift) & mask]++] = num;

		swap(v, tmp);

		curShift += shiftBits;
	}
}
#pragma endregion

#pragma region ull
void radix_sort::getCountVectorThreadULL(vector<ull>& v, vector<int>& count, int l, int r, int shiftBits, int mask)
{
	for (int i = l; i < r; i++)
		count[(v[i] >> shiftBits) & mask]++;
}

void radix_sort::getCountVector(vector<ull>& v, vector<int>& count, int shiftBits, int base, int mask)
{
	const int idealSize = 8000000;
	const double idealBucketSize = 1000000.0;

	int size = v.size();
	int threadCount = static_cast<int>(ceil(size / idealBucketSize));

	if (threadCount <= 1 || size < idealSize)
	{
		for (const auto& num : v)
			count[(num >> shiftBits) & mask]++;
	}
	else
	{
		threadCount = min(threadCount, static_cast<int>(thread::hardware_concurrency()));
		int bucketSize = size / threadCount;

		vector<thread> threads;
		vector<vector<int>> counts(threadCount, vector<int>(base));

		for (int i = 0; i < threadCount; i++)
		{
			int start = i * bucketSize;
			int end = (i == threadCount - 1) ? size : start + bucketSize;
			threads.emplace_back(getCountVectorThreadULL, ref(v), ref(counts[i]), start, end, shiftBits, mask);
		}

		for (auto& t : threads)
			t.join();

		for (int curThread = 0; curThread < threadCount; curThread++)
		{
			for (int i = 0; i < base; i++)
				count[i] += counts[curThread][i];
		}
	}
}

void radix_sort::getCountVector(vector<ull>& v, vector<int>& count, int shiftBits, int base, int mask, int l, int r)
{
	const int idealSize = 8000000;
	const double idealBucketSize = 1000000.0;

	int size = r - l;
	int threadCount = static_cast<int>(ceil(size / idealBucketSize));

	if (threadCount <= 1 || size < idealSize)
	{
		for (int i = l; i < r; i++)
			count[(v[i] >> shiftBits) & mask]++;
	}
	else
	{
		threadCount = min(threadCount, static_cast<int>(thread::hardware_concurrency()));
		int bucketSize = size / threadCount;

		vector<thread> threads;
		vector<vector<int>> counts(threadCount, vector<int>(base));

		for (int i = 0; i < threadCount; i++)
		{
			int start = l + i * bucketSize;
			int end = (i == threadCount - 1) ? r : start + bucketSize;
			threads.emplace_back(getCountVectorThreadULL, ref(v), ref(counts[i]), start, end, shiftBits, mask);
		}

		for (auto& t : threads)
			t.join();

		for (int curThread = 0; curThread < threadCount; curThread++)
		{
			for (int i = 0; i < base; i++)
				count[i] += counts[curThread][i];
		}
	}
}

void radix_sort::insertionSort(vector<ull>& v, int l, int r)
{
	for (int i = l + 1; i < r; i++)
	{
		ull val = v[i];
		int j = i - 1;

		while (j >= l && v[j] > val)
		{
			v[j + 1] = v[j];
			j--;
		}

		v[j + 1] = val;
	}
}

void radix_sort::sort(vector<ull>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	const int shiftBits = 8;
	const int base = 256;
	const int mask = 0xFF;

	int n = 0;
	ull maxVal = *max_element(v.begin(), v.end());

	while (maxVal > 0)
	{
		n++;
		maxVal >>= shiftBits;
	}

	vector<ull> tmp(v.size());
	int curShift = 0;

	while (n--)
	{
		vector<int> count(base);
		vector<int> prefix(base);

		getCountVector(v, count, curShift, base, mask);

		for (int i = 1; i < base; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		for (const auto& num : v)
			tmp[prefix[(num >> curShift) & mask]++] = num;

		swap(v, tmp);

		curShift += shiftBits;
	}
}

void radix_sort::sortRecursiveULL(vector<ull>& v, vector<ull>& tmp, int l, int r, int n)
{
	if (r - l < 2 || n == 0)
		return;

	if (r - l <= 50)
	{
		insertionSort(v, l, r);
		return;
	}

	const int shiftBits = 8;
	const int base = 256;
	const int mask = 0xFF;
	const int curShift = (n - 1) * shiftBits;

	vector<int> count(base);
	vector<int> prefix(base);

	for (int i = l; i < r; i++)
		count[(v[i] >> curShift) & mask]++;

	prefix[0] = l;
	for (int i = 1; i < base; i++)
		prefix[i] = prefix[i - 1] + count[i - 1];

	for (int i = l; i < r; i++)
		tmp[prefix[(v[i] >> curShift) & mask]++] = v[i];

	copy(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

	vector<int>().swap(prefix);
	n--;

	if (n == 0)
		return;

	const int idealBucketSize = max((r - l) / 1000, 1000);
	int numOfThreads = 0;
	vector<thread> threads;

	for (int i = 0; i < base; i++)
	{
		if (count[i] >= idealBucketSize)
			numOfThreads++;
	}

	if (numOfThreads > 1)
	{
		for (int i = 0, start = l; i < base; i++)
		{
			if (count[i] >= idealBucketSize)
				threads.emplace_back(sortRecursiveULL, ref(v), ref(tmp), start, start + count[i], n);

			start += count[i];
		}

		threadsCreated.fetch_add(numOfThreads);
	}

	for (int i = 0, start = l; i < base; i++)
	{
		if (count[i] < idealBucketSize)
			//if (count[i] > 1)
			sortRecursiveULL(v, tmp, start, start + count[i], n);
		start += count[i];
	}

	for (auto& t : threads)
		t.join();
}

void radix_sort::sortNonRecursiveULL(vector<ull>& v, vector<ull>& tmp, int l, int r, int len)
{
	const int shiftBits = 8;
	const int base = 256;
	const int mask = 0xFF;

	vector<RegionString> regions;
	regions.reserve(v.size() / 10);
	regions.emplace_back(l, r, len, -1);

	vector<thread> threads;

	while (regions.size())
	{
		RegionString region = move(regions.back());
		regions.pop_back();

		l = region.l;
		r = region.r;
		len = region.len;

		if (r - l < 2 || len == 0)
			continue;

		if (r - l <= 100)
		{
			insertionSort(v, l, r);
			continue;
		}

		const int curShift = (len - 1) * shiftBits;
		vector<int> count(base);
		vector<int> prefix(base);

		for (int i = l; i < r; i++)
			count[(v[i] >> curShift) & mask]++;

		prefix[0] = l;
		for (int i = 1; i < base; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		for (int i = l; i < r; i++)
			tmp[prefix[(v[i] >> curShift) & mask]++] = v[i];

		vector<int>().swap(prefix);
		move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

		len--;

		if (len == 0)
			continue;

		const int idealBucketSize = max((r - l) / 1000, 1000);
		int numOfThreads = 0;

		for (int i = 0; i < base; i++)
		{
			if (count[i] >= idealBucketSize)
				numOfThreads++;
		}

		if (numOfThreads > 1)
		{
			for (int i = 0, start = l; i < base; i++)
			{
				if (count[i] >= idealBucketSize)
					threads.emplace_back(sortNonRecursiveULL, ref(v), ref(tmp), start, start + count[i], len);

				start += count[i];
			}

			threadsCreated.fetch_add(numOfThreads);
		}

		for (int i = 0, start = l; i < base; i++)
		{
			if (count[i] < idealBucketSize)
				regions.emplace_back(start, start + count[i], len, -1);
			start += count[i];
		}
	}

	for (auto& t : threads)
		t.join();
}

void radix_sort::sortTest(vector<ull>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	vector<ull> tmp(v.size());

	const int shiftBits = 8;
	int n = 0;
	ull maxVal = *max_element(v.begin(), v.end());

	while (maxVal > 0)
	{
		n++;
		maxVal >>= shiftBits;
	}

	threadsCreated.store(0);
	sortRecursiveULL(v, tmp, 0, v.size(), n);
	cout << threadsCreated.load() << '\n';
}

void radix_sort::sortNonRecursiveM(std::vector<ull>& v, std::vector<ull>& tmp, 
	std::vector<RegionULL>& regions, std::unique_lock<std::mutex>& lkRegions, 
	RegionULL initialRegion, bool multiThreaded)
{
	const int shiftBits = 8;
	const int base = 256;
	const int mask = 0xFF;

	vector<RegionULL> regionsLocal;
	regionsLocal.reserve(v.size() / 100);
	regionsLocal.emplace_back(initialRegion);

	while (regionsLocal.size())
	{
		RegionULL region = move(regionsLocal.back());
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

		const int curShift = (len - 1) * shiftBits;
		vector<int> count(base);
		vector<int> prefix(base);

		getCountVector(v, count, curShift, base, mask, l, r);

		prefix[0] = l;
		for (int i = 1; i < base; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		for (int i = l; i < r; i++)
			tmp[prefix[(v[i] >> curShift) & mask]++] = v[i];

		move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

		len--;

		if (len == 0)
			continue;

		if (!multiThreaded)
		{
			for (int i = 0, start = l; i < base; i++)
			{
				if (count[i])
					regionsLocal.emplace_back(start, start + count[i], len);
				start += count[i];
			}
		}
		else
		{
			const int BUCKET_THRESHOLD = max((r - l) / 1000, 1000);

			for (int i = 0, start = l; i < base; i++)
			{
				if (count[i] < BUCKET_THRESHOLD)
					regionsLocal.emplace_back(start, start + count[i], len);
				else
				{
					lkRegions.lock();
					regions.emplace_back(start, start + count[i], len);
					lkRegions.unlock();
					threadsCreated++;
				}
				start += count[i];
			}
		}
	}
}

void radix_sort::sortNonRecursiveThreadULL(std::vector<ull>& v, std::vector<ull>& tmp,
	std::vector<RegionULL>& regions, std::mutex& regionsLock, std::atomic<int>& runningCounter,
	int threadIndex, std::vector<ull>& stepsActive, std::vector<ull>& stepsIdle)
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
			RegionULL region = move(regions.back());
			regions.pop_back();
			lkRegions.unlock();

			if (isIdle)
			{
				isIdle = false;
				runningCounter++;
				iterationsIdle = 0;
			}

			sortNonRecursiveM(v, tmp, regions, lkRegions, region, 1);
			stepsActive[threadIndex]++;
		}
		else
		{
			lkRegions.unlock();
			stepsIdle[threadIndex]++;

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

void radix_sort::sortM(std::vector<ull>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	vector<ull> tmp(v.size());

	const int shiftBits = 8;
	int len = 0;
	ull maxVal = *max_element(v.begin(), v.end());

	while (maxVal > 0)
	{
		len++;
		maxVal >>= shiftBits;
	}

	threadsCreated.store(0);
	int numOfThreads = thread::hardware_concurrency();
	const int BUCKET_THRESHOLD = max(static_cast<int>(v.size() / 1000), 1000);

	if (static_cast<int>(v.size() / 256) - (BUCKET_THRESHOLD / 10) < BUCKET_THRESHOLD)
	{
		vector<RegionULL> tmpVector;
		mutex tmpMutex;
		unique_lock<mutex> tmpLock(tmpMutex, defer_lock);

		sortNonRecursiveM(v, tmp, tmpVector, tmpLock, RegionULL(0, v.size(), len), 0);
	}
	else
	{
		vector<RegionULL> regions;
		regions.reserve(v.size() / 1000);
		regions.emplace_back(0, v.size(), len);

		vector<int> isIdle(numOfThreads);
		mutex regionsLock;
		mutex idleLock;

		vector<thread> threads;
		vector<ull> stepsActive(numOfThreads);
		vector<ull> stepsIdle(numOfThreads);

		atomic<int> runningCounter = numOfThreads;

		for (int i = 0; i < numOfThreads; i++)
			threads.emplace_back(sortNonRecursiveThreadULL, ref(v), ref(tmp), ref(regions), ref(regionsLock), ref(runningCounter), i, ref(stepsActive), ref(stepsIdle));

		for (auto& t : threads)
			t.join();

		for (int i = 0; i < numOfThreads; i++)
			cout << i << "\t" << stepsActive[i] << "\t" << stepsIdle[i] << "\n";
	}
	cout << threadsCreated.load() << '\n';
}

#pragma endregion

#pragma region float
void radix_sort::insertionSort(vector<float>& v, int l, int r)
{
	for (int i = l + 1; i < r; i++)
	{
		float val = v[i];
		int j = i - 1;

		while (j >= l && strong_order(val, v[j]) < 0)
		{
			v[j + 1] = v[j];
			j--;
		}

		v[j + 1] = val;
	}
}

void radix_sort::sort(vector<float>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	int size = v.size();
	vector<unsigned int> vUint(size);
	
	for (int i = 0; i < size; i++)
	{
		memcpy(&vUint[i], &v[i], sizeof(float));
		//vUint[i] = (~vUint[i]) * (vUint[i] >> 31) + (vUint[i] ^ 0x80000000) * ((vUint[i] >> 31) == 0);
		vUint[i] = (vUint[i] >> 31)? ~vUint[i] : vUint[i] | 0x80000000;
	}
	
	sort(vUint);

	for (int i = 0; i < size; i++)
	{
		//vUint[i] = (~vUint[i]) * ((vUint[i] >> 31) == 0) + (vUint[i] ^ 0x80000000) * (vUint[i] >> 31);
		vUint[i] = (vUint[i] >> 31) ? vUint[i] & 0x7FFFFFFF : ~vUint[i];
		memcpy(&v[i], &vUint[i], sizeof(float));
	}
}
#pragma endregion

#pragma region double
void radix_sort::insertionSort(vector<double>& v, int l, int r)
{
	for (int i = l + 1; i < r; i++)
	{
		double val = v[i];
		int j = i - 1;

		while (j >= l && strong_order(val, v[j]) < 0)
		{
			v[j + 1] = v[j];
			j--;
		}

		v[j + 1] = val;
	}
}

void radix_sort::sort(vector<double>& v)
{
	if (v.size() <= 100)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	int size = v.size();
	vector<ull> vULL(size);
	
	for (int i = 0; i < size; i++)
	{
		memcpy(&vULL[i], &v[i], sizeof(double));
		//vULL[i] = (~vULL[i]) * (vULL[i] >> 63) + (vULL[i] ^ 0x8000000000000000) * ((vULL[i] >> 63) == 0);
		vULL[i] = (vULL[i] >> 63)? ~vULL[i] : vULL[i] ^ 0x8000000000000000;
	}
	
	sortM(vULL);

	for (int i = 0; i < size; i++)
	{
		//vULL[i] = (~vULL[i]) * ((vULL[i] >> 63) == 0) + (vULL[i] ^ 0x8000000000000000) * (vULL[i] >> 63);
		vULL[i] = (vULL[i] >> 63) ? vULL[i] ^ 0x8000000000000000 : ~vULL[i];
		memcpy(&v[i], &vULL[i], sizeof(double));
	}
}
#pragma endregion

#pragma region string
int radix_sort::getChar(const string& s, int index)
{
	return (index < s.length()) ? static_cast<unsigned char>(s[index]) : 256;
}

void radix_sort::getCountVectorThreadString(vector<string>& v, vector<int>& count, int l, int r, int curIndex)
{
	for (int i = l; i < r; i++)
		count[getChar(v[i], curIndex)]++;
}

void radix_sort::getCountVector(vector<string>& v, vector<int>& count, int l, int r, int curIndex)
{
	const int idealSize = 8000000;
	const double idealBucketSize = 1000000.0;

	int size = r - l;
	int threadCount = static_cast<int>(ceil(size / idealBucketSize));

	if (threadCount <= 1 || size < idealSize)
	{
		for (int i = l; i < r; i++)
			count[getChar(v[i], curIndex)]++;
	}
	else
	{
		int chars = 257;
		
		threadCount = min(threadCount, static_cast<int>(thread::hardware_concurrency()));
		int bucketSize = size / threadCount;
		threadsCreated.fetch_add(threadCount);

		vector<thread> threads;
		vector<vector<int>> counts(threadCount, vector<int>(chars));

		for (int i = 0; i < threadCount; i++)
		{
			int start = l + i * bucketSize;
			int end = (i == threadCount - 1) ? r : start + bucketSize;
			threads.emplace_back(getCountVectorThreadString, ref(v), ref(counts[i]), start, end, curIndex);
		}

		for (auto& t : threads)
			t.join();

		for (int curThread = 0; curThread < threadCount; curThread++)
		{
			for (int i = 0; i < chars; i++)
				count[i] += counts[curThread][i];
		}
	}
}

void radix_sort::insertionSort(vector<string>& v, int l, int r)
{
	for (int i = l + 1; i < r; i++)
	{
		string val = move(v[i]);
		int j = i - 1;

		while (j >= l && v[j] > val)
		{
			v[j + 1] = move(v[j]);
			j--;
		}

		v[j + 1] = move(val);
	}
}

//void radix_sort::sortRecursive(vector<string>& v, vector<string>& tmp, int l, int r, int len, int curIndex)
//{
//	if (r - l < 2 || len == 0)
//		return;
//
//	if (r - l <= 10)
//	{
//		insertionSort(v, l, r);
//		return;
//	}
//
//	int chars = 257;
//
//	vector<int> count(chars);
//	vector<int> prefix(chars);
//
//	chars--;
//
//	getCountVector(v, count, l, r, curIndex);
//	
//	prefix[256] = l;
//	prefix[0] = prefix[256] + count[256];
//	for (int i = 1; i < chars; i++)
//		prefix[i] = prefix[i - 1] + count[i - 1];
//
//	for (int i = l; i < r; i++)
//		tmp[prefix[getChar(v[i], curIndex)]++] = move(v[i]);
//
//	vector<int>().swap(prefix);
//	move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);
//
//	len--;
//	curIndex++;
//
//	if (len == 0)
//		return;
//
//	for (int i = 0, start = l + count[256]; i < chars; i++)
//	{
//		if (count[i] > 1)
//			sortRecursive(v, tmp, start, start + count[i], len, curIndex);
//		start += count[i];
//	}
//}

//void radix_sort::sortRecursive(vector<string>& v, vector<string>& tmp, int l, int r, int len, int curIndex)
//{
//	if (r - l < 2 || len == 0)
//		return;
//
//	if (r - l <= 10)
//	{
//		insertionSort(v, l, r);
//		return;
//	}
//
//	int chars = 257;
//
//	vector<int> count(chars);
//	vector<int> prefix(chars);
//
//	chars--;
//
//	getCountVector(v, count, l, r, curIndex);
//	
//	prefix[256] = l;
//	prefix[0] = prefix[256] + count[256];
//	for (int i = 1; i < chars; i++)
//		prefix[i] = prefix[i - 1] + count[i - 1];
//
//	for (int i = l; i < r; i++)
//		tmp[prefix[getChar(v[i], curIndex)]++] = move(v[i]);
//
//	vector<int>().swap(prefix);
//	move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);
//
//	len--;
//	curIndex++;
//
//	if (len == 0)
//		return;
//
//	const int idealBucketSize = max((r - l)/1000, 1000);
//	int numOfThreads = 0;
//	vector<thread> threads;
//	
//	for (int i = 0; i < chars; i++)
//	{
//		if (count[i] >= idealBucketSize)
//			numOfThreads++;
//	}
//
//	if (numOfThreads > 1)
//	{
//		threads.reserve(numOfThreads);
//
//		for (int i = 0, start = l + count[256]; i < chars; i++)
//		{
//			if (count[i] >= idealBucketSize)
//				threads.emplace_back(sortRecursive, ref(v), ref(tmp), start, start + count[i], len, curIndex);
//			
//			start += count[i];
//		}
//	}
//
//	for (int i = 0, start = l + count[256]; i < chars; i++)
//	{
//		if (count[i] < idealBucketSize)
//			sortRecursive(v, tmp, start, start + count[i], len, curIndex);
//		start += count[i];
//	}
//
//	for (auto& t : threads)
//		t.join();
//}

void radix_sort::sortRecursive(vector<string>& v, vector<string>& tmp, int l, int r, int len, int curIndex)
{
	if (r - l < 2 || len == 0)
		return;

	if (r - l <= 10)
	{
		insertionSort(v, l, r);
		return;
	}

	int chars = 257;

	vector<int> count(chars);
	vector<int> prefix(chars);

	chars--;

	getCountVector(v, count, l, r, curIndex);
	
	prefix[256] = l;
	prefix[0] = prefix[256] + count[256];
	for (int i = 1; i < chars; i++)
		prefix[i] = prefix[i - 1] + count[i - 1];

	for (int i = l; i < r; i++)
		tmp[prefix[getChar(v[i], curIndex)]++] = move(v[i]);

	vector<int>().swap(prefix);
	move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

	len--;
	curIndex++;

	if (len == 0)
		return;

	const int idealBucketSize = max((r - l)/1000, 1000);
	int numOfBuckets = 0;
	vector<thread> threads;
	
	for (int i = 0; i < chars; i++)
	{
		if (count[i] >= idealBucketSize)
			numOfBuckets++;
	}

	int numOfThreads = min(numOfBuckets, static_cast<int>(thread::hardware_concurrency()));
	int bucketsPerThread = numOfBuckets / numOfThreads;

	if (numOfBuckets > 1)
	{
		threads.reserve(numOfBuckets);

		for (int i = 0, start = l + count[256]; i < chars; i++)
		{
			if (count[i] >= idealBucketSize)
				threads.emplace_back(sortRecursive, ref(v), ref(tmp), start, start + count[i], len, curIndex);
			
			start += count[i];
		}
	}

	for (int i = 0, start = l + count[256]; i < chars; i++)
	{
		if (count[i] < idealBucketSize)
			sortRecursive(v, tmp, start, start + count[i], len, curIndex);
		start += count[i];
	}

	for (auto& t : threads)
		t.join();
}

void radix_sort::sortNonRecursive(vector<string>& v, vector<string>& tmp, int l, int r, int len, int curIndex)
{
	int regionIndex = 0;
	vector<RegionString> regions;
	regions.reserve(v.size() / 10);
	regions.emplace_back(l, r, len, curIndex);

	vector<thread> threads;
	
	while (regions.size())
	{
		RegionString region = move(regions.back());
		regions.pop_back();
		
		l = region.l;
		r = region.r;
		len = region.len;
		curIndex = region.curIndex;

		if (r - l < 2 || len == 0)
			continue;
		
		if (r - l <= 10)
		{
			insertionSort(v, l, r);
			continue;
		}
		
		int chars = 257;

		vector<int> count(chars);
		vector<int> prefix(chars);

		chars--;

		getCountVector(v, count, l, r, curIndex);

		prefix[256] = l;
		prefix[0] = prefix[256] + count[256];
		for (int i = 1; i < chars; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		for (int i = l; i < r; i++)
			tmp[prefix[getChar(v[i], curIndex)]++] = move(v[i]);

		vector<int>().swap(prefix);
		move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

		len--;
		curIndex++;

		if (len == 0)
			continue;

		const int idealBucketSize = max((r - l) / 1000, 1000);
		int numOfThreads = 0;

		for (int i = 0; i < chars; i++)
		{
			if (count[i] >= idealBucketSize)
				numOfThreads++;
		}

		if (numOfThreads > 1)
		{
			for (int i = 0, start = l + count[256]; i < chars; i++)
			{
				if (count[i] >= idealBucketSize)
					threads.emplace_back(sortNonRecursive, ref(v), ref(tmp), start, start + count[i], len, curIndex);

				start += count[i];
			}

			threadsCreated.fetch_add(numOfThreads);
		}

		for (int i = 0, start = l + count[256]; i < chars; i++)
		{
			if (count[i] < idealBucketSize)
				regions.emplace_back(start, start + count[i], len, curIndex);
			start += count[i];
		}
	}

	for (auto& t : threads)
		t.join();
}

void radix_sort::sort(vector<string>& v)
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

	threadsCreated.store(0);

	//if (len < 100)
	//	sortRecursive(v, tmp, 0, v.size(), len, 0);
	//else
		sortNonRecursive(v, tmp, 0, v.size(), len, 0);

	cout << threadsCreated.load() << '\n';
}

void radix_sort::sortNonRecursiveM(vector<string>& v, vector<string>& tmp, vector<RegionString>& regions,
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

		int chars = 257;

		vector<int> count(chars);
		vector<int> prefix(chars);

		chars--;

		getCountVector(v, count, l, r, curIndex);

		prefix[256] = l;
		prefix[0] = prefix[256] + count[256];
		for (int i = 1; i < chars; i++)
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
			for (int i = 0, start = l + count[256]; i < chars; i++)
			{
				if (count[i] > 1)
					regionsLocal.emplace_back(start, start + count[i], len, curIndex);
				start += count[i];
			}
		}
		else
		{
			const int BUCKET_THRESHOLD = max((r - l) / 1000, 1000);

			for (int i = 0, start = l + count[256]; i < chars; i++)
			{
				if (count[i] < BUCKET_THRESHOLD)
					regionsLocal.emplace_back(start, start + count[i], len, curIndex);
				else
				{
					lkRegions.lock();
					regions.emplace_back(start, start + count[i], len, curIndex);
					lkRegions.unlock();
					threadsCreated++;
				}
				start += count[i];
			}
		}
	}
}

void radix_sort::sortNonRecursiveThread(vector<string>& v, vector<string>& tmp,
	vector<RegionString>& regions, mutex& regionsLock, atomic<int>& runningCounter,
	int threadIndex, vector<ull>& stepsActive, vector<ull>& stepsIdle)
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

			sortNonRecursiveM(v, tmp, regions, lkRegions, region, 1);
			stepsActive[threadIndex]++;
		}
		else
		{
			lkRegions.unlock();
			stepsIdle[threadIndex]++;

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

void radix_sort::sortM(vector<string>& v)
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

	threadsCreated.store(0);
	int numOfThreads = thread::hardware_concurrency();
	const int BUCKET_THRESHOLD = max(static_cast<int>(v.size() / 1000), 1000);

	if (static_cast<int>(v.size() / 256) - (BUCKET_THRESHOLD / 10) < BUCKET_THRESHOLD)
	{
		vector<RegionString> tmpVector;
		mutex tmpMutex;
		unique_lock<mutex> tmpLock(tmpMutex, defer_lock);

		sortNonRecursiveM(v, tmp, tmpVector, tmpLock, RegionString(0, v.size(), len, 0), 0);
	}
	else
	{
		vector<RegionString> regions;
		regions.reserve(v.size() / 1000);
		regions.emplace_back(0, v.size(), len, 0);

		vector<int> isIdle(numOfThreads);
		mutex regionsLock;
		mutex idleLock;
	
		vector<thread> threads;
		vector<ull> stepsActive(numOfThreads);
		vector<ull> stepsIdle(numOfThreads);

		atomic<int> runningCounter = numOfThreads;

		for (int i = 0; i < numOfThreads; i++)
			threads.emplace_back(sortNonRecursiveThread, ref(v), ref(tmp), ref(regions), ref(regionsLock), ref(runningCounter), i, ref(stepsActive), ref(stepsIdle));

		for (auto& t : threads)
			t.join();

		for (int i = 0; i < numOfThreads; i++)
			cout << i << "\t" << stepsActive[i] << "\t" << stepsIdle[i] << "\n";
	}
	cout << threadsCreated.load() << '\n';
}

#pragma endregion