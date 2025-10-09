#include "show_off.hpp"
#include "radix_sort.hpp"
#include "generators.hpp"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <execution>
#include <format>

using namespace std;

const vector<int> show_off::RUN_METHOD = { 
	0, // SORT 
	1, // SORT_PAR
	0, // STABLE_SORT
	0, // STABLE_SORT_PAR
	1  // RADIX_SORT
};

//
// SHOW OFF METHODS
//

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
			std::sort(vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
			break;
		case SORT_PAR:
			std::sort(std::execution::par, vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
			break;
		case STABLE_SORT:
			std::stable_sort(vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
			break;
		case STABLE_SORT_PAR:
			std::stable_sort(std::execution::par, vSort.begin(), vSort.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
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
		int n = 1e8;
		output += "INT\nSIZE = 100m\n\n";
		vector<int> v(generators::generateInts(n));

		for (int i = Method::SORT; i <= Method::RADIX_SORT; i++)
		{
			if (RUN_METHOD[i])
				showOff(v, static_cast<Method>(i), output);
		}

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (RUN_UINT)
	{
		int n = 1e8;
		output += "UNSIGNED INT\nSIZE = 100m\n\n";
		vector<unsigned int> v(generators::generateUnsignedInts(n));

		for (int i = Method::SORT; i <= Method::RADIX_SORT; i++)
		{
			if (RUN_METHOD[i])
				showOff(v, static_cast<Method>(i), output);
		}

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (RUN_FLOAT)
	{
		int n = 1e8;
		output += "FLOAT\nSIZE = 100m\n\n";
		vector<float> v(generators::generateFloats(n));

		for (int i = Method::SORT; i <= Method::RADIX_SORT; i++)
		{
			if (RUN_METHOD[i])
				showOff(v, static_cast<Method>(i), output);
		}

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (RUN_STRING)
	{
		int n = 5e7;
		output += "STRING\nLEN = [0, 20]\nSIZE = 50m\n\n";
		vector<string> v(generators::generateStrings(n, 20));

		for (int i = Method::SORT; i <= Method::RADIX_SORT; i++)
		{
			if (RUN_METHOD[i])
				showOff(v, static_cast<Method>(i), output);
		}

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}
}

//
// VALIDATATION METHODS
//

void show_off::validate(vector<int>& v, string& output)
{
	vector<int> vExpected(v);
	vector<int> vRadix(v);

	auto start = chrono::high_resolution_clock::now();
	std::sort(std::execution::par, vExpected.begin(), vExpected.end());
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "sort_par = " + to_string(time) + " ms\n";

	start = chrono::high_resolution_clock::now();
	radix_sort::sort(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "radix_sort = " + to_string(time) + " ms\n\n";

	if (ranges::equal(vRadix, vExpected))
	{
		output += "Sort is valid!\n";
	}
	else
	{
		output += "ERROR: Outputs are different!\n";
		output += "index: (radix value, expected value)\n";
		int size = v.size();
		
		for (int i = 0; i < size; i++)
		{
			if (vRadix[i] != vExpected[i])
				output += format("{}:\t({}, {})\n", i, vRadix[i], vExpected[i]);
		}
	}

	output += "\n";
}

void show_off::validate(vector<unsigned int>& v, string& output)
{
	vector<unsigned int> vExpected(v);
	vector<unsigned int> vRadix(v);

	auto start = chrono::high_resolution_clock::now();
	std::sort(std::execution::par, vExpected.begin(), vExpected.end());
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "sort_par = " + to_string(time) + " ms\n";

	start = chrono::high_resolution_clock::now();
	radix_sort::sort(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "radix_sort = " + to_string(time) + " ms\n\n";

	if (ranges::equal(vRadix, vExpected))
	{
		output += "Sort is valid!\n";
	}
	else
	{
		output += "ERROR: Outputs are different!\n";
		output += "index: (radix value, expected value)\n";
		int size = v.size();
		
		for (int i = 0; i < size; i++)
		{
			if (vRadix[i] != vExpected[i])
				output += format("{}:\t({}, {})\n", i, vRadix[i], vExpected[i]);
		}
	}

	output += "\n";
}

void show_off::validate(vector<long long>& v, string& output)
{
	vector<long long> vExpected(v);
	vector<long long> vRadix(v);

	auto start = chrono::high_resolution_clock::now();
	std::sort(std::execution::par, vExpected.begin(), vExpected.end());
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "sort_par = " + to_string(time) + " ms\n";

	start = chrono::high_resolution_clock::now();
	radix_sort::sort(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "radix_sort = " + to_string(time) + " ms\n\n";

	if (ranges::equal(vRadix, vExpected))
	{
		output += "Sort is valid!\n";
	}
	else
	{
		output += "ERROR: Outputs are different!\n";
		output += "index: (radix value, expected value)\n";
		int size = v.size();
		
		for (int i = 0; i < size; i++)
		{
			if (vRadix[i] != vExpected[i])
				output += format("{}:\t({}, {})\n", i, vRadix[i], vExpected[i]);
		}
	}

	output += "\n";
}

void show_off::validate(vector<unsigned long long>& v, string& output)
{
	vector<unsigned long long> vExpected(v);
	vector<unsigned long long> vRadix(v);

	auto start = chrono::high_resolution_clock::now();
	std::sort(std::execution::par, vExpected.begin(), vExpected.end());
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "sort_par = " + to_string(time) + " ms\n";

	start = chrono::high_resolution_clock::now();
	radix_sort::sort(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "radix_sort = " + to_string(time) + " ms\n\n";

	if (ranges::equal(vRadix, vExpected))
	{
		output += "Sort is valid!\n";
	}
	else
	{
		output += "ERROR: Outputs are different!\n";
		output += "index: (radix value, expected value)\n";
		int size = v.size();
		
		for (int i = 0; i < size; i++)
		{
			if (vRadix[i] != vExpected[i])
				output += format("{}:\t({}, {})\n", i, vRadix[i], vExpected[i]);
		}
	}

	output += "\n";
}

void show_off::validate(vector<float>& v, string& output)
{
	vector<float> vExpected(v);
	vector<float> vRadix(v);

	auto start = chrono::high_resolution_clock::now();
	std::sort(std::execution::par, vExpected.begin(), vExpected.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "sort_par = " + to_string(time) + " ms\n";

	start = chrono::high_resolution_clock::now();
	radix_sort::sort(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "radix_sort = " + to_string(time) + " ms\n\n";

	if (ranges::equal(vRadix, vExpected, [](const auto& a, const auto& b) { return std::strong_order(a, b) == 0; }))
	{
		output += "Sort is valid!\n";
	}
	else
	{
		output += "ERROR: Outputs are different!\n";
		output += "index: (radix value, expected value) hex(radix value, expected value)\n";
		int size = v.size();
		
		for (int i = 0; i < size; i++)
		{
			if (vRadix[i] != vExpected[i])
				output += format("{}:\t({}, {})\thex({}, {})\n", i, vRadix[i], vExpected[i], 
					bit_cast<unsigned int>(vRadix[i]), bit_cast<unsigned int>(vExpected[i]));
		}
	}

	output += "\n";
}

void show_off::validate(vector<double>& v, string& output)
{
	vector<double> vExpected(v);
	vector<double> vRadix(v);

	auto start = chrono::high_resolution_clock::now();
	std::sort(std::execution::par, vExpected.begin(), vExpected.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "sort_par = " + to_string(time) + " ms\n";

	start = chrono::high_resolution_clock::now();
	radix_sort::sort(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "radix_sort = " + to_string(time) + " ms\n\n";

	if (ranges::equal(vRadix, vExpected, [](const auto& a, const auto& b) { return std::strong_order(a, b) == 0; }))
	{
		output += "Sort is valid!\n";
	}
	else
	{
		output += "ERROR: Outputs are different!\n";
		output += "index: (radix value, expected value) hex(radix value, expected value)\n";
		int size = v.size();
		
		for (int i = 0; i < size; i++)
		{
			if (vRadix[i] != vExpected[i])
				output += format("{}:\t({}, {})\thex({}, {})\n", i, vRadix[i], vExpected[i], 
					bit_cast<unsigned long long>(vRadix[i]), bit_cast<unsigned long long>(vExpected[i]));
		}
	}

	output += "\n";
}

void show_off::validate(vector<string>& v, string& output)
{
	vector<string> vExpected(v);
	vector<string> vRadix(v);

	auto start = chrono::high_resolution_clock::now();
	std::sort(std::execution::par, vExpected.begin(), vExpected.end());
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "sort_par = " + to_string(time) + " ms\n";

	start = chrono::high_resolution_clock::now();
	radix_sort::sort(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "radix_sort = " + to_string(time) + " ms\n\n";

	if (ranges::equal(vRadix, vExpected))
	{
		output += "Sort is valid!\n";
	}
	else
	{
		output += "ERROR: Outputs are different!\n";
		output += "index: (radix value, expected value)\n";
		int size = v.size();
		
		for (int i = 0; i < size; i++)
		{
			if (vRadix[i] != vExpected[i])
				output += format("{}:\t({}, {})\n", i, vRadix[i], vExpected[i]);
		}
	}

	output += "\n";
}

void show_off::validate(int INT, int UINT, int LL, int ULL, int FLOAT, int DOUBLE, int STRING)
{
	string output = "";

	if (INT)
	{
		int n = 1e8;
		output += "INT\nSIZE = 100m\n\n";
		vector<int> v(generators::generateInts(n));

		validate(v, output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (UINT)
	{
		int n = 1e8;
		output += "UNSIGNED INT\nSIZE = 100m\n\n";
		vector<unsigned int> v(generators::generateUnsignedInts(n));

		validate(v, output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (LL)
	{
		int n = 1e8;
		output += "LONG LONG\nSIZE = 100m\n\n";
		vector<long long> v(generators::generateLLs(n));

		validate(v, output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (ULL)
	{
		int n = 1e8;
		output += "UNSIGNED LONG LONG\nSIZE = 100m\n\n";
		vector<unsigned long long> v(generators::generateULLs(n));

		validate(v, output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (FLOAT)
	{
		int n = 1e8;
		output += "FLOAT\nSIZE = 100m\n\n";
		vector<float> v(generators::generateFloats(n));

		validate(v, output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (DOUBLE)
	{
		int n = 1e8;
		output += "DOUBLE\nSIZE = 100m\n\n";
		vector<double> v(generators::generateDoubles(n));

		validate(v, output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}

	if (STRING)
	{
		int n = 5e7;
		output += "STRING\nLEN = [0, 20]\nSIZE = 50m\n\n";
		vector<string> v(generators::generateStrings(n, 20));

		validate(v, output);

		output += "=========================\n\n";
		cout << output;
		output.clear();
	}
}
