#include "generators.hpp"
#include "radixsort_testing.hpp"
#include "radix_sort.hpp"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <execution>
#include <thread>
#include <compare>
#include <bit>
#include "show_off.hpp"

using namespace std;

void runTest()
{
	int n = 100000000;
	int iterations = 10;
	long long totalTime = 0;

	vector<float> v(generators::generateFloats(n));

	for (int i = 0; i < iterations; i++)
	{
		vector<float> vRadix(v);
		auto start = chrono::high_resolution_clock::now();
		radix_sort::sort(vRadix);
		auto end = chrono::high_resolution_clock::now();
		totalTime += chrono::duration_cast<chrono::milliseconds>(end - start).count();
	}

	cout << "radix_sort = " << totalTime/iterations << "\n";
}

void runShitFloat()
{
	int n = 100000000;

	vector<float> v(generators::generateFloats(n));
	vector<float> vSort(v);
	vector<float> vRadix(v);

	//this_thread::sleep_for(chrono::seconds(3));
	auto start = chrono::high_resolution_clock::now();

	//std::sort(vSort.begin(), vSort.end());
	//std::sort(std::execution::par, vSort.begin(), vSort.end());
	//std::sort(vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	std::sort(std::execution::par, vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	//std::stable_sort(std::execution::par, vSort.begin(), vSort.end());
	//std::stable_sort(vSort.begin(), vSort.end());
	//std::stable_sort(vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });

	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::microseconds>(end - start).count();
	cout << "std::sort = " << time << "\n";

	//this_thread::sleep_for(chrono::seconds(1));

	start = chrono::high_resolution_clock::now();
	radix_sort::sort(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::microseconds>(end - start).count();
	cout << "radix_sort = " << time << "\n";

	for (int i = 0; i < n; i++)
	{
		if (vSort[i] != vRadix[i])
		{
			cout << i << ' ' << vSort[i] << ' ' << vRadix[i] << ' ' << hex << bit_cast<unsigned int>(vSort[i]) << ' ' << bit_cast<unsigned int>(vRadix[i]) << dec << '\n';
		}
	}
}

void runShitDouble()
{
	int n = 10000000;

	vector<double> v(generators::generateDoubles(n));
	vector<double> vSort(v);
	vector<double> vRadix(v);

	//this_thread::sleep_for(chrono::seconds(3));
	auto start = chrono::high_resolution_clock::now();

	//std::sort(vSort.begin(), vSort.end());
	//std::sort(std::execution::par, vSort.begin(), vSort.end());
	//std::sort(vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	std::sort(std::execution::par, vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	//std::stable_sort(std::execution::par, vSort.begin(), vSort.end());
	//std::stable_sort(vSort.begin(), vSort.end());
	//std::stable_sort(vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	//std::stable_sort(std::execution::par, vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });

	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::microseconds>(end - start).count();
	cout << "std::sort = " << time << "\n";

	//this_thread::sleep_for(chrono::seconds(1));

	start = chrono::high_resolution_clock::now();
	radix_sort::sort(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::microseconds>(end - start).count();
	cout << "radix_sort = " << time << "\n";

	for (int i = 0; i < n; i++)
	{
		if (vSort[i] != vRadix[i])
		{
			cout << i << ' ' << vSort[i] << ' ' << vRadix[i] << ' ' << hex << bit_cast<unsigned long long>(vSort[i]) << ' ' << bit_cast<unsigned long long>(vRadix[i]) << dec << '\n';
		}
	}
}

void runShitString()
{
	int n = 3e5;

	vector<string> vSort(generators::generateStrings(n, 20));
	vector<string> vRadix(vSort);

	//this_thread::sleep_for(chrono::seconds(3));
	auto start = chrono::high_resolution_clock::now();

	//std::sort(vSort.begin(), vSort.end());
	std::sort(std::execution::par, vSort.begin(), vSort.end());
	//std::sort(vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	//std::sort(std::execution::par, vSort.begin(), vSort.end());
	//std::stable_sort(std::execution::par, vSort.begin(), vSort.end());
	//std::stable_sort(vSort.begin(), vSort.end());
	//std::stable_sort(vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });

	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::microseconds>(end - start).count();
	cout << "std::sort = " << time << "\n";

	//this_thread::sleep_for(chrono::seconds(1));

	start = chrono::high_resolution_clock::now();
	radix_sort::sortM(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::microseconds>(end - start).count();
	cout << "radix_sort = " << time << "\n";

	for (int i = 0; i < n; i++)
	{
		if (vSort[i] != vRadix[i])
		{
			cout << i << ' ' << vSort[i] << ' ' << vRadix[i] << '\n';
		}
	}
}

void runShitULL()
{
	int n = 1e8;

	//vector<unsigned long long> vSort(generators::generateULLs(n));
	//vector<unsigned long long> vRadix(vSort);

	vector<unsigned long long> vSort(generators::generateULLs(n));
	vector<unsigned long long> vRadix(vSort);

	//this_thread::sleep_for(chrono::seconds(3));
	auto start = chrono::high_resolution_clock::now();

	//std::sort(vSort.begin(), vSort.end());
	//std::sort(std::execution::par, vSort.begin(), vSort.end());
	//std::sort(vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	std::sort(std::execution::par, vSort.begin(), vSort.end());
	//std::stable_sort(std::execution::par, vSort.begin(), vSort.end());
	//std::stable_sort(vSort.begin(), vSort.end());
	//std::stable_sort(vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });

	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::microseconds>(end - start).count();
	cout << "std::sort = " << time << "\n";

	//this_thread::sleep_for(chrono::seconds(1));

	start = chrono::high_resolution_clock::now();
	radix_sort::sortTest(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::microseconds>(end - start).count();
	cout << "radix_sort = " << time << "\n";

	for (int i = 0; i < n; i++)
	{
		if (vSort[i] != vRadix[i])
		{
			cout << i << ' ' << vSort[i] << ' ' << vRadix[i] << '\n';
		}
	}
}

void radixSort(vector<int>& v)
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

void radixSort256Counting(vector<int>& v)
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

int main()
{
	//runTest();
	//runShit();
	//runShitString();
	//runShitDouble();
	runShitULL();
	//show_off::showOff(1, 1, 1, 1);
}