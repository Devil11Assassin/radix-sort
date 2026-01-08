#pragma region HEADERS
#include "show_off.hpp"

#include <algorithm>
#include <chrono>
#include <concepts>
#include <execution>
#include <format>
#include <iostream>
#include <locale>
#include <type_traits>

#include "generators.hpp"
#include "radix_sort.hpp"

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
constexpr bool ENABLE_MULTITHREADING = true;

locale lnum = locale("en_US.UTF-8");

const vector<show_off::DataTypeRun> show_off::RUN_DATATYPE =
{
	DataTypeRun(  CHAR,		RUN_SIZE, format(lnum,   "CHAR\nSIZE = {:L}\n\n",     RUN_SIZE)),
	DataTypeRun( UCHAR,		RUN_SIZE, format(lnum,  "UCHAR\nSIZE = {:L}\n\n",     RUN_SIZE)),
	DataTypeRun( SHORT,		RUN_SIZE, format(lnum,	"SHORT\nSIZE = {:L}\n\n",     RUN_SIZE)),
	DataTypeRun(USHORT,		RUN_SIZE, format(lnum, "USHORT\nSIZE = {:L}\n\n",     RUN_SIZE)),
	DataTypeRun(   INT,		RUN_SIZE, format(lnum,    "INT\nSIZE = {:L}\n\n",     RUN_SIZE)),
	DataTypeRun(  UINT,		RUN_SIZE, format(lnum,   "UINT\nSIZE = {:L}\n\n",     RUN_SIZE)),
	DataTypeRun(    LL,		RUN_SIZE, format(lnum,     "LL\nSIZE = {:L}\n\n",     RUN_SIZE)),
	DataTypeRun(   ULL,		RUN_SIZE, format(lnum,    "ULL\nSIZE = {:L}\n\n",     RUN_SIZE)),
	DataTypeRun( FLOAT,		RUN_SIZE, format(lnum,	"FLOAT\nSIZE = {:L}\n\n",     RUN_SIZE)),
	DataTypeRun(DOUBLE,		RUN_SIZE, format(lnum, "DOUBLE\nSIZE = {:L}\n\n",     RUN_SIZE)),
	DataTypeRun(STRING, RUN_SIZE_STR, format(lnum, "STRING\nSIZE = {:L}\n\n", RUN_SIZE_STR)),
};
#pragma endregion

#pragma region SHOW OFF METHODS
template<typename T>
void show_off::showOff(vector<T>& v, Method method, string& output)
{
	vector<T> vSort(v);

	auto start = chrono::steady_clock::now();
	if constexpr (floating_point<T>)
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
				radix_sort::sort(vSort, ENABLE_MULTITHREADING);
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
				radix_sort::sort(vSort, ENABLE_MULTITHREADING);
				break;
		}
	}
	auto end = chrono::steady_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	switch (method)
	{
		case SORT:
			output += format(lnum, "sort = {:L} ms\n", time);
			break;
		case SORT_PAR:
			output += format(lnum, "sort_par = {:L} ms\n", time);
			break;
		case STABLE_SORT:
			output += format(lnum, "stable_sort = {:L} ms\n", time);
			break;
		case STABLE_SORT_PAR:
			output += format(lnum, "stable_sort_par = {:L} ms\n", time);
			break;
		case RADIX_SORT:
			output += format(lnum, "radix_sort = {:L} ms\n\n", time);
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

void show_off::showOff(RunParams params)
{
	string output = "=========================\n\n";
	const vector<int> RUN_TYPE = {
		params.CHAR, params.UCHAR,
		params.SHORT, params.USHORT,
		params.INT, params.UINT,
		params.LL, params.ULL,
		params.FLOAT, params.DOUBLE,
		params.STRING };

	for (const auto& instance : RUN_DATATYPE)
	{
		if (RUN_TYPE[instance.type])
		{
			output += instance.output;
			switch (instance.type)
			{
				case DataType::CHAR:
					showOff<char>(instance.n, output);
					break;
				case DataType::UCHAR:
					showOff<unsigned char>(instance.n, output);
					break;
				case DataType::SHORT:
					showOff<short>(instance.n, output);
					break;
				case DataType::USHORT:
					showOff<unsigned short>(instance.n, output);
					break;
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

	auto start = chrono::steady_clock::now();
	if constexpr (floating_point<T>)
		std::sort(std::execution::par, vExpected.begin(), vExpected.end(), [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; });
	else
		std::sort(std::execution::par, vExpected.begin(), vExpected.end());
	auto end = chrono::steady_clock::now();
	auto time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += format(lnum, "sort_par = {:L} ms\n", time);

	start = chrono::steady_clock::now();
	radix_sort::sort(vRadix, ENABLE_MULTITHREADING);
	end = chrono::steady_clock::now();
	time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
	output += format(lnum, "radix_sort = {:L} ms\n\n", time);

	bool isEqual = (floating_point<T>) ? 
		ranges::equal(vRadix, vExpected, [](const auto& a, const auto& b) { return std::strong_order(a, b) == 0; }) :
		ranges::equal(vRadix, vExpected);

	if (isEqual)
		output += "Sort is valid!\n";
	else
	{
		int size = v.size();
		output += "ERROR: Outputs are different!\n";

		if constexpr (floating_point<T>)
		{
			output += "index: (radix value, expected value) hex(radix value, expected value)\n";
			
			using U = fp2i<T>;
			
			for (int i = 0; i < size; i++)
			{
				if (vRadix[i] != vExpected[i])
					output += format("{}:\t({}, {})\thex({}, {})\n", i, vRadix[i], vExpected[i],
						bit_cast<U>(vRadix[i]), bit_cast<U>(vExpected[i]));
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

void show_off::validate(RunParams params)
{
	string output = "=========================\n\n";
	const vector<int> RUN_TYPE = { 
		params.CHAR, params.UCHAR,
		params.SHORT, params.USHORT,
		params.INT, params.UINT,
		params.LL, params.ULL,
		params.FLOAT, params.DOUBLE,
		params.STRING };

	for (const auto& instance : RUN_DATATYPE)
	{
		if (RUN_TYPE[instance.type])
		{
			output += instance.output;
			switch (instance.type)
			{
				case DataType::CHAR:
					validate<char>(instance.n, output);
					break;
				case DataType::UCHAR:
					validate<unsigned char>(instance.n, output);
					break;
				case DataType::SHORT:
					validate<short>(instance.n, output);
					break;
				case DataType::USHORT:
					validate<unsigned short>(instance.n, output);
					break;
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