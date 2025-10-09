#include "radixsort_testing.hpp"
#include <algorithm>
#include <thread>
#include <chrono>
#include <iostream>
using namespace std;

#pragma region INT_TESTS
vector<int> radixsort_testing::methodsInt = { 0, 1, 1, 1, 1, 1, 0 };

void radixsort_testing::radixSort(vector<int>& v)
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

void radixsort_testing::radixSort256(vector<int>& v)
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

void radixsort_testing::radixSort256Fast(vector<int>& v)
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

void radixsort_testing::radixSort256Counting(vector<int>& v)
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

void radixsort_testing::getCountVectorThread(vector<int>& v, vector<int>& count, int l, int r, int shiftBits, int mask)
{
	for (int i = l; i < r; i++)
		count[(v[i] >> shiftBits) & mask]++;
}

void radixsort_testing::getCountVector(vector<int>& v, vector<int>& count, int shiftBits, int base, int mask)
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
}

void radixsort_testing::radixSort256CountingMulti(vector<int>& v)
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

void radixsort_testing::insertionSort(vector<int>& v, int l, int r)
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

void radixsort_testing::radixSort256MSDRecursion(vector<int>& v, vector<int>& tmp, int l, int r, int n)
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

	for (int i = 0, start = l; i < base; i++)
	{
		if (n != 0 && count[i] > 1)
			radixSort256MSDRecursion(v, tmp, start, start + count[i], n);
		start += count[i];
	}
}

void radixsort_testing::radixSort256MSD(vector<int>& v)
{
	const int shiftBits = 8;

	int n = 0;
	int maxVal = *max_element(v.begin(), v.end());

	while (maxVal > 0)
	{
		n++;
		maxVal >>= shiftBits;
	}

	vector<int> tmp(v.size());
	radixSort256MSDRecursion(v, tmp, 0, v.size(), n);
}

long long radixsort_testing::useMethod(vector<int>& v, MethodInt method)
{
	auto start = chrono::high_resolution_clock::now();

	switch (method)
	{
		case SORT:
			std::sort(v.begin(), v.end());
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
		case RADIX_256_MSD:
			radixSort256MSD(v);
			break;
	}

	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	return time;
}

void radixsort_testing::addOutput(string& output, MethodInt method, long long time)
{
	switch (method)
	{
		case SORT:
			output += "Sort: " + to_string(time) + "\n";
				break;
		case RADIX_10:
			output += "Radix10: " + to_string(time) + "\n";
			break;
		case RADIX_256:
			output += "Radix256: " + to_string(time) + "\n";
			break;
		case RADIX_256_FAST:
			output += "Radix256Fast: " + to_string(time) + "\n";
			break;
		case RADIX_256_COUNTING:
			output += "Radix256Counting: " + to_string(time) + "\n";
			break;
		case RADIX_256_COUNTING_MULTI:
			output += "Radix256CountingMutli: " + to_string(time) + "\n";
			break;
		case RADIX_256_MSD:
			output += "Radix256MSD: " + to_string(time) + "\n";
			break;
	}
}

void radixsort_testing::sort(vector<int>& v)
{
	string output = "";

	for (int method = SORT; method <= RADIX_256_MSD; method++)
	{
		if (methodsInt[method])
		{
			vector<int> vSort(v);
			auto time = useMethod(vSort, static_cast<MethodInt>(method));
			addOutput(output, static_cast<MethodInt>(method), time);
		}
	}

	cout << output;
}

void radixsort_testing::sortCompare(vector<int>& v)
{
	string output = "";

	vector<int> vCompare(v);
	auto sortTime = useMethod(vCompare, MethodInt::SORT);
	addOutput(output, MethodInt::SORT, sortTime);

	for (int method = RADIX_10; method <= RADIX_256_MSD; method++)
	{
		if (methodsInt[method])
		{
			vector<int> vSort(v);
			auto time = useMethod(vSort, static_cast<MethodInt>(method));
			
			if (equal(vSort.begin(), vSort.end(), vCompare.begin(), vCompare.end()))
				addOutput(output, static_cast<MethodInt>(method), time);
			else
				output += "ERROR: Arrays aren't the same!\n";
		}
	}

	cout << output;
}
#pragma endregion

#pragma region STRING_TESTS
vector<int> radixsort_testing::methodsString = { 1, 1 };

//
// signed char variant
// 
//inline char radixsort_testing::getChar(string& s, int index)
//{
//	return (index < s.length()) ? s[index] : 0;
//}
//void radixsort_testing::radixSortMSDRecursion(vector<string>& v, vector<string>& tmp, int l, int r, int n, int curIndex)
//{
//	if (r - l < 2 || n == 0)
//		return;
//
//	if (r - l <= 10)
//	{
//		insertionSort(v, l, r);
//		return;
//	}
//
//	const int chars = 127;
//
//	vector<int> count(chars);
//	vector<int> prefix(chars);
//
//	for (int i = l; i < r; i++)
//		count[getChar(v[i], curIndex)]++;
//
//	prefix[0] = l;
//	for (int i = 1; i < chars; i++)
//		prefix[i] = prefix[i - 1] + count[i - 1];
//
//	for (int i = l; i < r; i++)
//		tmp[prefix[getChar(v[i], curIndex)]++] = move(v[i]);
//
//	move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);
//
//	n--;
//	curIndex++;
//
//	if (n == 0)
//		return;
//
//	for (int i = 1, start = l + count[0]; i < chars; i++)
//	{
//		if (count[i] > 1)
//			radixSortMSDRecursion(v, tmp, start, start + count[i], n, curIndex);
//		start += count[i];
//	}
//}

inline int radixsort_testing::getChar(const string& s, int index)
{
	return (index < s.length()) ? static_cast<unsigned char>(s[index]) : 256;
}

void radixsort_testing::insertionSort(vector<string>& v, int l, int r)
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

void radixsort_testing::radixSortMSDRecursion(vector<string>& v, vector<string>& tmp, int l, int r, int n, int curIndex)
{
	if (r - l < 2 || n == 0)
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

	for (int i = l; i < r; i++)
		count[getChar(v[i], curIndex)]++;

	prefix[256] = l;
	prefix[0] = prefix[256] + count[256];
	for (int i = 1; i < chars; i++)
		prefix[i] = prefix[i - 1] + count[i - 1];

	for (int i = l; i < r; i++)
		tmp[prefix[getChar(v[i], curIndex)]++] = move(v[i]);

	vector<int>().swap(prefix);
	move(tmp.begin() + l, tmp.begin() + r, v.begin() + l);

	n--;
	curIndex++;

	if (n == 0)
		return;

	for (int i = 0, start = l + count[256]; i < chars; i++)
	{
		if (count[i] > 1)
			radixSortMSDRecursion(v, tmp, start, start + count[i], n, curIndex);
		start += count[i];
	}
}

void radixsort_testing::radixSortLSD(vector<string>& v, vector<string>& tmp, int n)
{
	while (n--)
	{
		int chars = 257;

		vector<int> count(chars);
		vector<int> prefix(chars);

		chars--;

		for (const auto& s : v)
			count[getChar(s, n)]++;

		prefix[256] = 0;
		prefix[0] = prefix[256] + count[256];
		for (int i = 1; i < chars; i++)
			prefix[i] = prefix[i - 1] + count[i - 1];

		for (auto& s : v)
			tmp[prefix[getChar(s, n)]++] = move(s);

		swap(v, tmp);
	}
}

void radixsort_testing::radixSortMSD(vector<string>& v)
{
	if (v.size() <= 10)
	{
		insertionSort(v, 0, v.size());
		return;
	}

	vector<string> tmp(v.size());

	const int lsdIdealLength = 5;
	const int lsdIdealSize = 10000000;

	int n = (*max_element(v.begin(), v.end(),
		[](const string& a, const string& b) { 
			return a.length() < b.length();
		})).length();

	//if (v.size() >= lsdIdealSize && n <= lsdIdealLength)
	//{
	//	bool isFixedLength = all_of(v.begin(), v.end(),
	//		[&](const string& s) {
	//			return s.length() == n;
	//		});

	//	if (isFixedLength)
	//	{
	//		radixSortLSD(v, tmp, n);
	//		return;
	//	}
	//}
	
	radixSortMSDRecursion(v, tmp, 0, v.size(), n, 0);
}

long long radixsort_testing::useMethod(vector<string>& v, MethodString method)
{
	auto start = chrono::high_resolution_clock::now();

	switch (method)
	{
		case STD_SORT:
			std::sort(v.begin(), v.end());
			break;
		case RADIX_MSD:
			radixSortMSD(v);
			break;
	}

	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	return time;
}

void radixsort_testing::addOutput(string& output, MethodString method, long long time)
{
	switch (method)
	{
		case STD_SORT:
			output += "Sort: " + to_string(time) + "\n";
			break;
		case RADIX_MSD:
			output += "RadixMSD: " + to_string(time) + "\n";
			break;
	}
}

void radixsort_testing::sortCompare(vector<string>& v)
{
	string output = "";

	vector<string> vCompare(v);
	auto sortTime = useMethod(vCompare, MethodString::STD_SORT);
	addOutput(output, MethodString::STD_SORT, sortTime);

	for (int method = RADIX_MSD; method <= RADIX_MSD; method++)
	{
		if (methodsString[method])
		{
			vector<string> vSort(v);
			auto time = useMethod(vSort, static_cast<MethodString>(method));

			if (equal(vSort.begin(), vSort.end(), vCompare.begin(), vCompare.end()))
				addOutput(output, static_cast<MethodString>(method), time);
			else
				output += "ERROR: Arrays aren't the same!\n";
		}
	}

	cout << output;
}
#pragma endregion
