#pragma region HEADERS
#include "show_off.hpp"

#include <algorithm>
#include <chrono>
#include <concepts>
#include <execution>
#include <iostream>
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
	1, // RADIX_SORT
	1, // RADIX_SORT_PAR
};

constexpr int RUN_SIZE = 1e8;
constexpr int RUN_SIZE_STR = 5e7;
constexpr int RUN_SIZE_CLX = 1e7;
constexpr bool ENABLE_MULTITHREADING = true;

const locale show_off::lnum = locale("en_US.UTF-8");

const vector<string> show_off::method2str =
{
	"sort", "sort_par", "stable_sort", "stable_sort_par", "radix_sort", "radix_sort_par",
};

const vector<string> show_off::type2str =
{
	"CHAR", "UCHAR", "SHORT", "USHORT",	"INT", "UINT", "LL", "ULL", "FLOAT", "DOUBLE", "STRING", 
	"COMPLEX_I32", "COMPLEX_LL", "COMPLEX_FP", "COMPLEX_STR",
};

struct Timer
{
	chrono::steady_clock::time_point start_point;
	chrono::steady_clock::time_point end_point;

	Timer()
	{
		start_point = chrono::steady_clock::now();
		end_point = start_point;
	};

	void start()
	{
		start_point = chrono::steady_clock::now();
	}

	long long stop()
	{
		end_point = chrono::steady_clock::now();
		auto time = chrono::duration_cast<chrono::milliseconds>(end_point - start_point).count();
		start_point = end_point;
		return time;
	}
};

const vector<show_off::DataTypeRun> show_off::RUN_DATATYPE =
{
	DataTypeRun(       CHAR,     RUN_SIZE),
	DataTypeRun(      UCHAR,     RUN_SIZE),
	DataTypeRun(      SHORT,     RUN_SIZE),
	DataTypeRun(     USHORT,     RUN_SIZE),
	DataTypeRun(        INT,     RUN_SIZE),
	DataTypeRun(       UINT,     RUN_SIZE),
	DataTypeRun(         LL,     RUN_SIZE),
	DataTypeRun(        ULL,     RUN_SIZE),
	DataTypeRun(      FLOAT,     RUN_SIZE),
	DataTypeRun(     DOUBLE,     RUN_SIZE),
	DataTypeRun(     STRING, RUN_SIZE_STR),
	DataTypeRun(COMPLEX_I32, RUN_SIZE_CLX),
	DataTypeRun( COMPLEX_LL, RUN_SIZE_CLX),
	DataTypeRun( COMPLEX_FP, RUN_SIZE_CLX),
	DataTypeRun(COMPLEX_STR, RUN_SIZE_CLX),
};

template <typename T, typename U>
constexpr auto getLambdaSTD()
{
	if constexpr (floating_point<T>)
		return [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; };
	else if constexpr (same_as<T, Employee>)
	{
		if constexpr (same_as<U, int32_t>)
			return [](const Employee& a, const Employee& b) -> const auto& { return a.age < b.age; };
		else if constexpr (same_as<U, long long>)
			return [](const Employee& a, const Employee& b) -> const auto& { return a.id < b.id; };
		else if constexpr (floating_point<U>)
			return [](const Employee& a, const Employee& b) -> const auto& { return std::strong_order(a.salary, b.salary) < 0; };
		else if constexpr (same_as<U, string>)
			return [](const Employee& a, const Employee& b) -> const auto& { return a.name < b.name; };
	}
	else
		return std::less<>();
}

template <typename T, typename U>
constexpr auto getLambdaRDX()
{
	if constexpr (same_as<T, Employee>)
	{
		if constexpr (same_as<U, int32_t>)
			return [](const Employee& e) -> const auto& { return e.age; };
		else if constexpr (same_as<U, long long>)
			return [](const Employee& e) -> const auto& { return e.id; };
		else if constexpr (floating_point<U>)
			return [](const Employee& e) -> const auto& { return e.salary; };
		else if constexpr (same_as<U, string>)
			return [](const Employee& e) -> const auto& { return e.name; };
	}
	else
		return std::identity{};

}
#pragma endregion

#pragma region SHOW OFF METHODS
template <typename T, typename U>
void show_off::showOff(vector<T>& v, Method method, string& output)
{
	vector<T> vSort(v);
	constexpr auto LAMBDA_STD = getLambdaSTD<T, U>();
	constexpr auto LAMBDA_RDX = getLambdaRDX<T, U>();

	Timer timer;
	timer.start();
	switch (method)
	{
		case SORT:
			std::sort(vSort.begin(), vSort.end(), LAMBDA_STD);
			break;
		case SORT_PAR:
			std::sort(std::execution::par, vSort.begin(), vSort.end(), LAMBDA_STD);
			break;
		case STABLE_SORT:
			std::stable_sort(vSort.begin(), vSort.end(), LAMBDA_STD);
			break;
		case STABLE_SORT_PAR:
			std::stable_sort(std::execution::par, vSort.begin(), vSort.end(), LAMBDA_STD);
			break;
		case RADIX_SORT:
			radix_sort::sort(vSort, LAMBDA_RDX, false);
			break;
		case RADIX_SORT_PAR:
			radix_sort::sort(vSort, LAMBDA_RDX, true);
			break;
	}
	auto time = timer.stop();

	output += format(lnum, "{} = {:L} ms\n", method2str[method], time);
}

template<typename T, typename U>
void show_off::showOff(int n, string& output)
{
	vector<T> v(generators::generate<T>(n));
	
	for (int i = Method::SORT; i <= Method::RADIX_SORT_PAR; i++)
	{
		if (RUN_METHOD[i])
			showOff<T, U>(v, static_cast<Method>(i), output);
	}
}

void show_off::showOff(RunParams params)
{
	string output = "=========================\n\n";
	const vector<int> RUN_TYPE = 
	{
		params.CHAR, params.UCHAR,
		params.SHORT, params.USHORT,
		params.INT, params.UINT,
		params.LL, params.ULL,
		params.FLOAT, params.DOUBLE,
		params.STRING, 
		params.COMPLEX_I32, params.COMPLEX_LL,
		params.COMPLEX_FP, params.COMPLEX_STR,
	};

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
				case DataType::COMPLEX_I32:
					showOff<Employee, decltype(Employee::age)>(instance.n, output);
					break;
				case DataType::COMPLEX_LL:
					showOff<Employee, decltype(Employee::id)>(instance.n, output);
					break;
				case DataType::COMPLEX_FP:
					showOff<Employee, decltype(Employee::salary)>(instance.n, output);
					break;
				case DataType::COMPLEX_STR:
					showOff<Employee, decltype(Employee::name)>(instance.n, output);
					break;
			}
			output += "\n=========================\n\n";
			cout << output;
			output.clear();
		}
	}
}
#pragma endregion

#pragma region VALIDATION METHODS
template<typename T, typename U>
void show_off::validate(vector<T>& v, string& output)
{
	vector<T> vExpected(v);
	vector<T> vRadix(v);
	constexpr auto LAMBDA_STD = getLambdaSTD<T, U>();
	constexpr auto LAMBDA_RDX = getLambdaRDX<T, U>();

	Timer timer;

	timer.start();
	if constexpr (same_as<T, Employee>)
		std::stable_sort(std::execution::par, vExpected.begin(), vExpected.end(), LAMBDA_STD);
	else
		std::sort(std::execution::par, vExpected.begin(), vExpected.end(), LAMBDA_STD);
	auto time = timer.stop();
	
	constexpr string METHOD_NAME = (same_as<T, Employee>) ? "stable_sort_par" : "sort_par";
	output += format(lnum, "{} = {:L} ms\n", METHOD_NAME, time);

	timer.start();
	radix_sort::sort(vRadix, LAMBDA_RDX, ENABLE_MULTITHREADING);
	time = timer.stop();
	output += format(lnum, "radix_sort = {:L} ms\n\n", time);

	bool isEqual = false;
	if constexpr (floating_point<T>)
		isEqual = ranges::equal(vRadix, vExpected, [](const auto& a, const auto& b) { return std::strong_order(a, b) == 0; });
	else
		isEqual = ranges::equal(vRadix, vExpected);

	if (isEqual)
		output += "Sort is valid!\n";
	else
	{
		int size = v.size();
		output += "ERROR: Outputs are different!\n";

		if constexpr (floating_point<T>)
		{
			output += "index: (radix value, expected value) hex(radix value, expected value)\n";
			
			using TU = fp2i<T>;
			
			for (int i = 0; i < size; i++)
			{
				if (vRadix[i] != vExpected[i])
					output += format("{}:\t({}, {})\thex({}, {})\n", i, vRadix[i], vExpected[i],
						bit_cast<TU>(vRadix[i]), bit_cast<TU>(vExpected[i]));
			}
		}
		else if constexpr (!same_as<T, Employee>)
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

template<typename T, typename U>
void show_off::validate(int n, string& output)
{
	vector<T> v(generators::generate<T>(n));
	validate<T, U>(v, output);
}

void show_off::validate(RunParams params)
{
	string output = "=========================\n\n";
	const vector<int> RUN_TYPE = 
	{ 
		params.CHAR, params.UCHAR,
		params.SHORT, params.USHORT,
		params.INT, params.UINT,
		params.LL, params.ULL,
		params.FLOAT, params.DOUBLE,
		params.STRING, 
		params.COMPLEX_I32, params.COMPLEX_LL,
		params.COMPLEX_FP, params.COMPLEX_STR
	};

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
				case DataType::COMPLEX_I32:
					validate<Employee, decltype(Employee::age)>(instance.n, output);
					break;
				case DataType::COMPLEX_LL:
					validate<Employee, decltype(Employee::id)>(instance.n, output);
					break;
				case DataType::COMPLEX_FP:
					validate<Employee, decltype(Employee::salary)>(instance.n, output);
					break;
				case DataType::COMPLEX_STR:
					validate<Employee, decltype(Employee::name)>(instance.n, output);
					break;
			}
			output += "=========================\n\n";
			cout << output;
			output.clear();
		}
	}
}
#pragma endregion