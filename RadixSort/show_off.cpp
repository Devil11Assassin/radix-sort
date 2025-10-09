#include "show_off.hpp"
#include "radix_sort.hpp"
#include "generators.hpp"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <execution>

using namespace std;

void show_off::showOff(vector<int>& v, Method method, string& output)
{
	vector<int> vSort(v);

	auto start = chrono::high_resolution_clock::now();
	switch (method)
	{
		case SORT:
			std::sort(vSort.begin(), vSort.end());
			break;
		case SORT_PAR:
			std::sort(std::execution::par, vSort.begin(), vSort.end());
			break;
		case STABLE_SORT:
			std::stable_sort(vSort.begin(), vSort.end());
			break;
		case STABLE_SORT_PAR:
			std::stable_sort(std::execution::par, vSort.begin(), vSort.end());
			break;
		case RADIX_SORT:
			radix_sort::sort(vSort);
			break;
	}

	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	switch (method)
	{
		case SORT:
			output += "sort = " + to_string(time) + " ms\n";
			break;
		case SORT_PAR:
			output += "sort_par = " + to_string(time) + " ms\n";
			break;
		case STABLE_SORT:
			output += "stable_sort = " + to_string(time) + " ms\n";
			break;
		case STABLE_SORT_PAR:
			output += "stable_sort_par = " + to_string(time) + " ms\n";
			break;
		case RADIX_SORT:
			output += "radix_sort = " + to_string(time) + " ms\n\n";
			break;
	}
}

void show_off::showOff(vector<unsigned int>& v, Method method, string& output)
{
	vector<unsigned int> vSort(v);

	auto start = chrono::high_resolution_clock::now();
	switch (method)
	{
		case SORT:
			std::sort(vSort.begin(), vSort.end());
			break;
		case SORT_PAR:
			std::sort(std::execution::par, vSort.begin(), vSort.end());
			break;
		case STABLE_SORT:
			std::stable_sort(vSort.begin(), vSort.end());
			break;
		case STABLE_SORT_PAR:
			std::stable_sort(std::execution::par, vSort.begin(), vSort.end());
			break;
		case RADIX_SORT:
			radix_sort::sort(vSort);
			break;
	}
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	switch (method)
	{
		case SORT:
			output += "sort = " + to_string(time) + " ms\n";
			break;
		case SORT_PAR:
			output += "sort_par = " + to_string(time) + " ms\n";
			break;
		case STABLE_SORT:
			output += "stable_sort = " + to_string(time) + " ms\n";
			break;
		case STABLE_SORT_PAR:
			output += "stable_sort_par = " + to_string(time) + " ms\n";
			break;
		case RADIX_SORT:
			output += "radix_sort = " + to_string(time) + " ms\n\n";
			break;
	}
}

void show_off::showOff(vector<float>& v, Method method, string& output)
{
	vector<float> vSort(v);

	auto start = chrono::high_resolution_clock::now();
	switch (method)
	{
		case SORT:
			std::sort(vSort.begin(), vSort.end());
			break;
		case SORT_PAR:
			std::sort(std::execution::par, vSort.begin(), vSort.end());
			break;
		case STABLE_SORT:
			std::stable_sort(vSort.begin(), vSort.end());
			break;
		case STABLE_SORT_PAR:
			std::stable_sort(std::execution::par, vSort.begin(), vSort.end());
			break;
		case RADIX_SORT:
			radix_sort::sort(vSort);
			break;
	}
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	switch (method)
	{
		case SORT:
			output += "sort = " + to_string(time) + " ms\n";
			break;
		case SORT_PAR:
			output += "sort_par = " + to_string(time) + " ms\n";
			break;
		case STABLE_SORT:
			output += "stable_sort = " + to_string(time) + " ms\n";
			break;
		case STABLE_SORT_PAR:
			output += "stable_sort_par = " + to_string(time) + " ms\n";
			break;
		case RADIX_SORT:
			output += "radix_sort = " + to_string(time) + " ms\n\n";
			break;
	}
}

void show_off::showOff(vector<string>& v, Method method, string& output)
{
	vector<string> vSort(v);

	auto start = chrono::high_resolution_clock::now();
	switch (method)
	{
		case SORT:
			std::sort(vSort.begin(), vSort.end());
			break;
		case SORT_PAR:
			std::sort(std::execution::par, vSort.begin(), vSort.end());
			break;
		case STABLE_SORT:
			std::stable_sort(vSort.begin(), vSort.end());
			break;
		case STABLE_SORT_PAR:
			std::stable_sort(std::execution::par, vSort.begin(), vSort.end());
			break;
		case RADIX_SORT:
			radix_sort::sort(vSort);
			break;
	}
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	switch (method)
	{
		case SORT:
			output += "sort = " + to_string(time) + " ms\n";
			break;
		case SORT_PAR:
			output += "sort_par = " + to_string(time) + " ms\n";
			break;
		case STABLE_SORT:
			output += "stable_sort = " + to_string(time) + " ms\n";
			break;
		case STABLE_SORT_PAR:
			output += "stable_sort_par = " + to_string(time) + " ms\n";
			break;
		case RADIX_SORT:
			output += "radix_sort = " + to_string(time) + " ms\n\n";
			break;
	}
}

void show_off::showOff(int INT, int UINT, int FLOAT, int STRING)
{
	string output = "";
	const int RUN_INT = INT;
	const int RUN_UINT = UINT;
	const int RUN_FLOAT = FLOAT;
	const int RUN_STRING = STRING;

	if (RUN_INT)
	{
		int n = 100000000;
		output += "INT\nSIZE = 100m\n\n";
		vector<int> v(generators::generateInts(n));

		for (int i = Method::SORT; i <= Method::RADIX_SORT; i++)
			showOff(v, static_cast<Method>(i), output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (RUN_UINT)
	{
		int n = 100000000;
		output += "UNSIGNED INT\nSIZE = 100m\n\n";
		vector<unsigned int> v(generators::generateUnsignedInts(n));

		for (int i = Method::SORT; i <= Method::RADIX_SORT; i++)
			showOff(v, static_cast<Method>(i), output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (RUN_FLOAT)
	{
		int n = 100000000;
		output += "FLOAT\nSIZE = 100m\n\n";
		vector<float> v(generators::generateFloats(n));

		for (int i = Method::SORT; i <= Method::RADIX_SORT; i++)
			showOff(v, static_cast<Method>(i), output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (RUN_STRING)
	{
		int n = 50000000;
		output += "STRING\nLEN = [0, 20]\nSIZE = 50m\n\n";
		vector<string> v(generators::generateStrings(n, 20));

		for (int i = Method::SORT; i <= Method::RADIX_SORT; i++)
			showOff(v, static_cast<Method>(i), output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}
}