#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <random>

using namespace std;

void radixSort(vector<int> &v)
{
	int n = 0;
	int maxVal = *max_element(v.begin(), v.end());
	
	while (maxVal)
	{
		n++;
		maxVal /= 10;
	}
	
	int div = 1;

	while (n--)
	{
		vector<int> tmp;
		tmp.reserve(v.size());

		vector<vector<int>> buckets(10);
		for (auto& bucket : buckets)
			bucket.reserve(v.size() / 10);

		for (const auto& num : v)
			buckets[(num / div) % 10].emplace_back(num);

		for (const auto& bucket : buckets)
		{
			for (const auto& num : bucket)
				tmp.emplace_back(num);
		}

		v = move(tmp);
		div *= 10;
	}
}

void radixSort256(vector<int> &v)
{
	const int shiftBits = 8;
	const int base = 256;
	const int mask = 0xFF;

	int n = 0;
	int maxVal = *max_element(v.begin(), v.end());
	int maxN = 0;

	while (maxVal > 0)
	{
		maxN++;
		if (maxVal & mask)
			n = maxN;
		maxVal >>= shiftBits;
	}

	int curShift = 0;
	int bucketSize = v.size() / base;

	while (n--)
	{
		vector<int> tmp;
		tmp.reserve(v.size());

		vector<vector<int>> buckets(base);

		for (auto& bucket : buckets)
			bucket.reserve(bucketSize);

		for (const auto& num : v)
			buckets[(num >> curShift) & mask].emplace_back(num);

		for (const auto& bucket : buckets)
		{
			for (const auto& num : bucket)
				tmp.emplace_back(num);
		}

		v = move(tmp);
		curShift += 8;
	}
}

void radixSort256Fast(vector<int> &v)
{
	const int shiftBits = 8;
	const int base = 256;
	const int mask = 0xFF;

	int n = 0;
	int maxVal = *max_element(v.begin(), v.end());

	while (maxVal > 0)
	{
		n++;
		maxVal >>= shiftBits;
	}

	int curShift = 0;
	int bucketSize = v.size() / base;

	vector<vector<int>> buckets(base);
	for (auto& bucket : buckets)
		bucket.reserve(bucketSize);

	while (n--)
	{
		for (const auto& num : v)
			buckets[(num >> curShift) & mask].emplace_back(num);

		v.clear();
		for (auto& bucket : buckets)
		{
			v.insert(v.end(), bucket.begin(), bucket.end());
			bucket.clear();
		}

		curShift += shiftBits;
	}
}

void radixSort256Counting(vector<int> &v)
{
	const int shiftBits = 8;
	const int base = 256;
	const int mask = 0xFF;

	int n = 0;
	int maxVal = *max_element(v.begin(), v.end());

	while (maxVal > 0)
	{
		n++;
		maxVal >>= shiftBits;
	}

	vector<int> tmp(v.size());
	int curShift = 0;

	while (n--)
	{
		vector<int> count(base);
		vector<int> prefix(base);

		for (const auto& num : v)
			count[(num >> curShift) & mask]++;

		for (int i = 1; i < base; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		for (const auto& num : v)
			tmp[prefix[(num >> curShift) & mask]++] = num;

		swap(v, tmp);

		curShift += shiftBits;
	}
}

void getCountVectorThread(vector<int>& v, vector<int>& count, int l, int r, int shiftBits, int mask)
{
	for (int i = l; i < r; i++)
		count[(v[i] >> shiftBits) & mask]++;
}

void getCountVector(vector<int>& v, vector<int>& count, int shiftBits, int base, int mask)
{
	int size = v.size();
	int threadCount = thread::hardware_concurrency();
	int bucketSize = size / threadCount;
	
	vector<thread> threads;
	vector<vector<int>> counts(threadCount, vector<int>(base));

	for (int i = 0; i < threadCount; i++)
	{
		int start = i * bucketSize;
		int end = (i == threadCount - 1) ? size : start + bucketSize;
		threads.emplace_back(getCountVectorThread, ref(v), ref(counts[i]), start, end, shiftBits, mask);
	}

	for (auto& t : threads)
		t.join();

	for (int curThread = 0; curThread < threadCount; curThread++)
	{
		for (int i = 0; i < base; i++)
			count[i] += counts[curThread][i];
	}
}

void radixSort256CountingMulti(vector<int> &v)
{
	const int shiftBits = 8;
	const int base = 256;
	const int mask = 0xFF;

	int n = 0;
	int maxVal = *max_element(v.begin(), v.end());

	while (maxVal > 0)
	{
		n++;
		maxVal >>= shiftBits;
	}

	vector<int> tmp(v.size());
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

enum Method {
	SORT,
	RADIX_10,
	RADIX_256,
	RADIX_256_FAST,
	RADIX_256_COUNTING,
	RADIX_256_COUNTING_MULTI
};

long long helper(vector<int> &v, Method method)
{
	auto start = chrono::high_resolution_clock::now();

	switch (method)
	{
		case SORT:
			sort(v.begin(), v.end());
			break;
		case RADIX_10:
			radixSort(v);
			break;
		case RADIX_256:
			radixSort256(v);
			break;
		case RADIX_256_FAST:
			radixSort256Fast(v);
			break;
		case RADIX_256_COUNTING:
			radixSort256Counting(v);
			break;
		case RADIX_256_COUNTING_MULTI:
			radixSort256CountingMulti(v);
			break;
	}

	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	return time;
}

vector<int> generateArray(int n)
{
	vector<int> v;
	v.reserve(n);

	mt19937 gen(69);

	while (n--)
		v.emplace_back(gen() % INT_MAX);

	return v;
}

int main()
{
	int n = 100000000;
	
	vector<int> vSort(generateArray(n));
	vector<int> vRadix10(vSort);
	vector<int> vRadix256(vSort);
	vector<int> vRadix256Fast(vSort);
	vector<int> vRadix256Counting(vSort);
	vector<int> vRadix256CountingMulti(vSort);

	//this_thread::sleep_for(chrono::seconds(3));

	auto sortTime = helper(vSort, SORT);
	auto radix10Time = helper(vRadix10, RADIX_10);
	auto radix256Time = helper(vRadix256, RADIX_256);
	auto radix256FastTime = helper(vRadix256Fast, RADIX_256_FAST);
	auto radix256CountingTime = helper(vRadix256Counting, RADIX_256_COUNTING);
	auto radix256CountingMultiTime = helper(vRadix256CountingMulti, RADIX_256_COUNTING_MULTI);

	cout << "Sort: " << sortTime << '\n';

	if (equal(vRadix10.begin(), vRadix10.end(), vSort.begin(), vSort.end()))
		cout << "Radix10: " << radix10Time << '\n';
	else
		cout << "ERROR: Arrays aren't the same!\n";

	if (equal(vRadix256.begin(), vRadix256.end(), vSort.begin(), vSort.end()))
		cout << "Radix256: " << radix256Time << '\n';
	else
		cout << "ERROR: Arrays aren't the same!\n";

	if (equal(vRadix256Fast.begin(), vRadix256Fast.end(), vSort.begin(), vSort.end()))
		cout << "Radix256Fast: " << radix256FastTime << '\n';
	else
		cout << "ERROR: Arrays aren't the same!\n";

	if (equal(vRadix256Counting.begin(), vRadix256Counting.end(), vSort.begin(), vSort.end()))
		cout << "Radix256Counting: " << radix256CountingTime << '\n';
	else
		cout << "ERROR: Arrays aren't the same!\n";

	if (equal(vRadix256CountingMulti.begin(), vRadix256CountingMulti.end(), vSort.begin(), vSort.end()))
		cout << "Radix256CountingMutli: " << radix256CountingMultiTime << '\n';
	else
		cout << "ERROR: Arrays aren't the same!\n";

	cout << "MEOW";

	//this_thread::sleep_for(chrono::seconds(3));
}