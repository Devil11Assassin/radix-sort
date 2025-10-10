#pragma region HEADERS
#include "show_off.hpp"
#include "radix_sort.hpp"
#include "generators.hpp"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <execution>
#include <format>
#include <type_traits>

using namespace std;
#pragma endregion

#pragma region FIELDS DEFINITION
const vector<int> show_off::RUN_METHOD = 
{ 
	1, // SORT 
	1, // SORT_PAR
	1, // STABLE_SORT
	1, // STABLE_SORT_PAR
	1  // RADIX_SORT
};

constexpr int RUN_SIZE = 1e8;
constexpr int RUN_SIZE_STR = 5e7;

const vector<show_off::DataTypeRun> show_off::RUN_DATATYPE =
{
	DataTypeRun(INT, RUN_SIZE, "INT\nSIZE = 100m\n\n"),
	DataTypeRun(UINT, RUN_SIZE, "UNSIGNED INT\nSIZE = 100m\n\n"),
	DataTypeRun(LL, RUN_SIZE, "LONG LONG\nSIZE = 100m\n\n"),
	DataTypeRun(ULL, RUN_SIZE, "UNSIGNED LONG LONG\nSIZE = 100m\n\n"),
	DataTypeRun(FLOAT, RUN_SIZE, "FLOAT\nSIZE = 100m\n\n"),
	DataTypeRun(DOUBLE, RUN_SIZE, "DOUBLE\nSIZE = 100m\n\n"),
	DataTypeRun(STRING, RUN_SIZE_STR, "STRING\nSIZE = 50m\n\n"),

};
#pragma endregion

#pragma region SHOW OFF METHODS
template<typename T>
void show_off::showOff(vector<T>& v, Method method, string& output)
{
	vector<T> vSort(v);

	auto start = chrono::high_resolution_clock::now();
	if constexpr (is_same_v<T, float> || is_same_v<T, double>)
	{
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
	}
	else
	{
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

template<typename T>
void show_off::showOff(int n, string& output)
{
	vector<T> v(generators::generate<T>(n));
	
	for (int i = Method::SORT; i <= Method::RADIX_SORT; i++)
	{
		if (RUN_METHOD[i])
			showOff(v, static_cast<Method>(i), output);
	}
}

void show_off::showOff(int INT, int UINT, int LL, int ULL, int FLOAT, int DOUBLE, int STRING)
{
	string output = "=========================\n\n";
	const vector<int> RUN_TYPE = { INT, UINT, LL, ULL, FLOAT, DOUBLE, STRING };

	for (const auto& instance : RUN_DATATYPE)
	{
		if (RUN_TYPE[instance.type])
		{
			output += instance.output;
			switch (instance.type)
			{
				case DataType::INT:
					showOff<int>(instance.n, output);
					break;
				case DataType::UINT:
					showOff<unsigned int>(instance.n, output);
					break;
				case DataType::LL:
					showOff<long long>(instance.n, output);
					break;
				case DataType::ULL:
					showOff<unsigned long long>(instance.n, output);
					break;
				case DataType::FLOAT:
					showOff<float>(instance.n, output);
					break;
				case DataType::DOUBLE:
					showOff<double>(instance.n, output);
					break;
				case DataType::STRING:
					showOff<string>(instance.n, output);
					break;
			}
			output += "=========================\n\n";
			cout << output;
			output.clear();
		}
	}
}
#pragma endregion

#pragma region VALIDATION METHODS
template<typename T>
void show_off::validate(vector<T>& v, string& output)
{
	vector<T> vExpected(v);
	vector<T> vRadix(v);

	constexpr bool isFloatOrDouble = is_same_v<T, float> || is_same_v<T, double>;

	auto start = chrono::high_resolution_clock::now();
	if constexpr (isFloatOrDouble)
		std::sort(std::execution::par, vExpected.begin(), vExpected.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	else
		std::sort(std::execution::par, vExpected.begin(), vExpected.end());
	auto end = chrono::high_resolution_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "sort_par = " + to_string(time) + " ms\n";

	start = chrono::high_resolution_clock::now();
	radix_sort::sort(vRadix);
	end = chrono::high_resolution_clock::now();
	time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += "radix_sort = " + to_string(time) + " ms\n\n";

	bool isEqual = (isFloatOrDouble) ? 
		ranges::equal(vRadix, vExpected, [](const auto& a, const auto& b) { return std::strong_order(a, b) == 0; }) :
		ranges::equal(vRadix, vExpected);

	if (isEqual)
		output += "Sort is valid!\n";
	else
	{
		int size = v.size();
		output += "ERROR: Outputs are different!\n";

		if constexpr (isFloatOrDouble)
		{
			output += "index: (radix value, expected value) hex(radix value, expected value)\n";
			
			if constexpr (is_same_v<T, float>)
			{
				for (int i = 0; i < size; i++)
				{
					if (vRadix[i] != vExpected[i])
						output += format("{}:\t({}, {})\thex({}, {})\n", i, vRadix[i], vExpected[i],
							bit_cast<unsigned int>(vRadix[i]), bit_cast<unsigned int>(vExpected[i]));
				}
			}
			else
			{
				for (int i = 0; i < size; i++)
				{
					if (vRadix[i] != vExpected[i])
						output += format("{}:\t({}, {})\thex({}, {})\n", i, vRadix[i], vExpected[i],
							bit_cast<unsigned long long>(vRadix[i]), bit_cast<unsigned long long>(vExpected[i]));
				}
			}
		}
		else
		{
			output += "index: (radix value, expected value)\n";

			for (int i = 0; i < size; i++)
			{
				if (vRadix[i] != vExpected[i])
					output += format("{}:\t({}, {})\n", i, vRadix[i], vExpected[i]);
			}
		}
	}

	output += "\n";
}

template<typename T>
void show_off::validate(int n, string& output)
{
	vector<T> v(generators::generate<T>(n));
	validate(v, output);
}

void show_off::validate(int INT, int UINT, int LL, int ULL, int FLOAT, int DOUBLE, int STRING)
{
	string output = "=========================\n\n";
	const vector<int> RUN_TYPE = { INT, UINT, LL, ULL, FLOAT, DOUBLE, STRING };

	for (const auto& instance : RUN_DATATYPE)
	{
		if (RUN_TYPE[instance.type])
		{
			output += instance.output;
			switch (instance.type)
			{
				case DataType::INT:
					validate<int>(instance.n, output);
					break;
				case DataType::UINT:
					validate<unsigned int>(instance.n, output);
					break;
				case DataType::LL:
					validate<long long>(instance.n, output);
					break;
				case DataType::ULL:
					validate<unsigned long long>(instance.n, output);
					break;
				case DataType::FLOAT:
					validate<float>(instance.n, output);
					break;
				case DataType::DOUBLE:
					validate<double>(instance.n, output);
					break;
				case DataType::STRING:
					validate<string>(instance.n, output);
					break;
			}
			output += "=========================\n\n";
			cout << output;
			output.clear();
		}
	}
}
#pragma endregion