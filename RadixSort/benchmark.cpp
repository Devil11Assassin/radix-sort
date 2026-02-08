#include "benchmark.hpp"

#include <chrono>
#include <concepts>
#include <cstddef>
#include <execution>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>

#include "generators.hpp"
#include "radix_sort.hpp"

using namespace std;

namespace benchmark
{
	enum Method
	{
		SORT,
		SORT_PAR,
		STABLE_SORT,
		STABLE_SORT_PAR,
		RADIX_SORT,
		RADIX_SORT_PAR
	};

	enum Type
	{
		CHAR,
		UCHAR,
		SHORT,
		USHORT,
		INT,
		UINT,
		LL,
		ULL,
		FLOAT,
		DOUBLE,
		STRING,
		COMPLEX_I32,
		COMPLEX_LL,
		COMPLEX_FLT,
		COMPLEX_DBL,
		COMPLEX_STR,
	};

	const locale lnum = locale("en_US.UTF-8");
	const vector<string> method2str = {
		"sort           ", "sort_par       ", 
		"stable_sort    ", "stable_sort_par", 
		"radix_sort     ", "radix_sort_par ",
	};
	const vector<string> shape2str = {
		"randomized", "sorted", "reverse sorted", "nearly sorted", "duplicates"
	};
	const vector<string> type2str = {
		"CHAR", "UCHAR", "SHORT", "USHORT",	"INT", "UINT", "LL", "ULL", "FLOAT", "DOUBLE", "STRING",
		"COMPLEX_INT", "COMPLEX_LL", "COMPLEX_FLOAT", "COMPLEX_DOUBLE", "COMPLEX_STR",
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
			auto time = chrono::duration_cast<chrono::microseconds>(end_point - start_point).count();
			start_point = end_point;
			return time;
		}
	};

	template <size_t S> struct fp2i_imp;
	template <> struct fp2i_imp<2> { using type = std::uint16_t; };
	template <> struct fp2i_imp<4> { using type = std::uint32_t; };
	template <> struct fp2i_imp<8> { using type = std::uint64_t; };

	template <typename T>
	using fp2i = fp2i_imp<sizeof(T)>::type;

	template <typename T, typename U = T>
	constexpr auto getLambdaStd()
	{
		if constexpr (floating_point<T>)
			return [](const auto& a, const auto& b) { return std::strong_order(a, b) < 0; };
		else if constexpr (same_as<T, Employee>)
		{
			if constexpr (same_as<U, decltype(Employee::age)>)
				return [](const Employee& a, const Employee& b) { return a.age < b.age; };
			else if constexpr (same_as<U, decltype(Employee::id)>)
				return [](const Employee& a, const Employee& b) { return a.id < b.id; };
			else if constexpr (same_as<U, decltype(Employee::salary_f)>)
				return [](const Employee& a, const Employee& b) { return std::strong_order(a.salary_f, b.salary_f) < 0; };
			else if constexpr (same_as<U, decltype(Employee::salary)>)
				return [](const Employee& a, const Employee& b) { return std::strong_order(a.salary, b.salary) < 0; };
			else if constexpr (same_as<U, decltype(Employee::name)>)
				return [](const Employee& a, const Employee& b) { return a.name < b.name; };
		}
		else
			return std::less<>();
	}

	template <typename T, typename U = T>
	constexpr auto getLambdaRadix()
	{
		if constexpr (same_as<T, Employee>)
		{
			if constexpr (same_as<U, decltype(Employee::age)>)
				return &Employee::age;
				//return [](const Employee& e) -> const auto& { return e.age; };
			else if constexpr (same_as<U, decltype(Employee::id)>)
				return &Employee::id;
				//return [](const Employee& e) -> const auto& { return e.id; };
			else if constexpr (same_as<U, decltype(Employee::salary_f)>)
				return &Employee::salary_f;
				//return [](const Employee& e) -> const auto& { return e.salary_f; };
			else if constexpr (same_as<U, decltype(Employee::salary)>)
				return &Employee::salary;
				//return [](const Employee& e) -> const auto& { return e.salary; };
			else if constexpr (same_as<U, decltype(Employee::name)>)
				return &Employee::name;
				//return [](const Employee& e) -> const auto& { return e.name; };
		}
		else
			return std::identity{};
	}

	template <typename T, typename U = T>
	void benchmark(vector<T>& v, int iterations, Method method, string& output)
	{
		constexpr auto LAMBDA_STD = getLambdaStd<T, U>();
		constexpr auto LAMBDA_RDX = getLambdaRadix<T, U>();

		Timer timer;
		long long timeTotal = 0;

		for (int i = 0; i < iterations; i++)
		{
			vector<T> vSort(v);

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
			timeTotal += timer.stop();
		}

		output += format(lnum, "{} = {:L} us\n", method2str[method], timeTotal/iterations);
	}

	template <typename T, typename U = T>
	void benchmark(size_t n, int shape, int iterations, const vector<int> METHODS, string& output)
	{
		vector<T> v(generators::generate<T>(n, static_cast<generators::Shape>(shape)));

		for (int i = Method::SORT; i <= Method::RADIX_SORT_PAR; i++)
		{
			if (METHODS[i])
				benchmark<T, U>(v, iterations, static_cast<Method>(i), output);
		}
	}

	void benchmark(RunParams params)
	{
		const vector<int> METHODS = 
		{
			params.SORT, params.SORT_PAR,
			params.STABLE_SORT, params.STABLE_SORT_PAR,
			params.RADIX_SORT, params.RADIX_SORT_PAR
		};

		const vector<int> SHAPES =
		{
			params.RANDOMIZED, params.SORTED,
			params.REVERSE_SORTED, params.NEARLY_SORTED,
			params.DUPLICATES
		};

		const vector<vector<size_t>> SIZES =
		{
			params.RUN_SIZE, params.RUN_SIZE, 
			params.RUN_SIZE, params.RUN_SIZE,
			params.RUN_SIZE, params.RUN_SIZE, 
			params.RUN_SIZE, params.RUN_SIZE,
			params.RUN_SIZE, params.RUN_SIZE,
			params.RUN_SIZE_STR,
			params.RUN_SIZE_CLX, params.RUN_SIZE_CLX,
			params.RUN_SIZE_CLX, params.RUN_SIZE_CLX,
			params.RUN_SIZE_CLX
		};

		const vector<int> TYPES =
		{
			params.CHAR, params.UCHAR,
			params.SHORT, params.USHORT,
			params.INT, params.UINT,
			params.LL, params.ULL,
			params.FLOAT, params.DOUBLE,
			params.STRING,
			params.CLX_I32, params.CLX_LL,
			params.CLX_FLT, params.CLX_DBL,
			params.CLX_STR,
		};

		int iterations = params.ITERATIONS;

		string curDateTime = format(
			"{:%Y-%m-%d %H-%M-%S}", 
			chrono::zoned_time{ chrono::current_zone(), floor<chrono::seconds>(chrono::system_clock::now()) }
		);
		ofstream file;
		filesystem::create_directory("benchmarks");
		file.open(format("benchmarks//{}.txt", curDateTime));

		string output = "";
		output += "===============================\n";
		output += "-------BENCHMARK STARTED-------\n";
		output += "===============================\n\n";

		cout << output;
		file << output;

		output.clear();
		output += "===============================\n\n";

		for (int shape = generators::Shape::RANDOMIZED; shape <= generators::Shape::DUPLICATES; shape++)
		{
			if (!SHAPES[shape])
				continue;

			for (int type = Type::CHAR; type <= Type::COMPLEX_STR; type++)
			{
				if (!TYPES[type])
					continue;

				for (const size_t& n : SIZES[type])
				{
					output += std::format(lnum, "{}\nSIZE = {:L} ({})\n\n", type2str[type], n, shape2str[shape]);
					switch (type)
					{
						case Type::CHAR:
							benchmark<char>(n, shape, iterations, METHODS, output);
							break;
						case Type::UCHAR:
							benchmark<unsigned char>(n, shape, iterations, METHODS, output);
							break;
						case Type::SHORT:
							benchmark<short>(n, shape, iterations, METHODS, output);
							break;
						case Type::USHORT:
							benchmark<unsigned short>(n, shape, iterations, METHODS, output);
							break;
						case Type::INT:
							benchmark<int>(n, shape, iterations, METHODS, output);
							break;
						case Type::UINT:
							benchmark<unsigned int>(n, shape, iterations, METHODS, output);
							break;
						case Type::LL:
							benchmark<long long>(n, shape, iterations, METHODS, output);
							break;
						case Type::ULL:
							benchmark<unsigned long long>(n, shape, iterations, METHODS, output);
							break;
						case Type::FLOAT:
							benchmark<float>(n, shape, iterations, METHODS, output);
							break;
						case Type::DOUBLE:
							benchmark<double>(n, shape, iterations, METHODS, output);
							break;
						case Type::STRING:
							benchmark<string>(n, shape, iterations, METHODS, output);
							break;
						case Type::COMPLEX_I32:
							benchmark<Employee, decltype(Employee::age)>(n, shape, iterations, METHODS, output);
							break;
						case Type::COMPLEX_LL:
							benchmark<Employee, decltype(Employee::id)>(n, shape, iterations, METHODS, output);
							break;
						case Type::COMPLEX_FLT:
							benchmark<Employee, decltype(Employee::salary_f)>(n, shape, iterations, METHODS, output);
							break;
						case Type::COMPLEX_DBL:
							benchmark<Employee, decltype(Employee::salary)>(n, shape, iterations, METHODS, output);
							break;
						case Type::COMPLEX_STR:
							benchmark<Employee, decltype(Employee::name)>(n, shape, iterations, METHODS, output);
							break;
					}
					output += "\n===============================\n\n";
					cout << output;
					file << output;
					output.clear();
				}
			}
		}

		output += "===============================\n";
		output += "-------BENCHMARK STOPPED-------\n";
		output += "===============================\n\n";

		cout << output;
		file << output;

		file.close();
	}

	template <typename T>
	bool checkEquality(vector<T> vRadix, vector<T> vExpected, string& output, bool detailed = false)
	{
		bool isEqual = false;
		if constexpr (floating_point<T>)
			isEqual = ranges::equal(vRadix, vExpected, [](const auto& a, const auto& b) { return std::strong_order(a, b) == 0; });
		else
			isEqual = ranges::equal(vRadix, vExpected);

		if (isEqual)
			output += "\t(correct)\n";
		else
		{
			output += "\t(wrong, different outputs)\n";

			if (!detailed)
				return isEqual;

			size_t size = vExpected.size();
			if constexpr (floating_point<T>)
			{
				output += "\nindex: (radix value, expected value) hex(radix value, expected value)\n";

				using TU = fp2i<T>;

				for (size_t i = 0; i < size; i++)
				{
					if (vRadix[i] != vExpected[i])
						output += format("{}:\t({}, {})\thex({}, {})\n", i, vRadix[i], vExpected[i],
							bit_cast<TU>(vRadix[i]), bit_cast<TU>(vExpected[i]));
				}
			}
			else if constexpr (!same_as<T, Employee>)
			{
				output += "\nindex: (radix value, expected value)\n";

				for (size_t i = 0; i < size; i++)
				{
					if (vRadix[i] != vExpected[i])
						output += format("{}:\t({}, {})\n", i, vRadix[i], vExpected[i]);
				}
			}

			output += '\n';
		}

		return isEqual;
	}

	template <typename T, typename U = T>
	void testing(vector<T>& v, string& output, size_t& wrongCounter)
	{
		vector<T> vExpected(v);
		vector<T> vRadix(v);
		constexpr auto LAMBDA_STD = getLambdaStd<T, U>();
		constexpr auto LAMBDA_RDX = getLambdaRadix<T, U>();

		Timer timer;
		bool correct = true;

		timer.start();
		if constexpr (same_as<T, Employee>)
			std::stable_sort(std::execution::par, vExpected.begin(), vExpected.end(), LAMBDA_STD);
		else
			std::sort(std::execution::par, vExpected.begin(), vExpected.end(), LAMBDA_STD);
		auto time = timer.stop();

		const string METHOD_NAME = (same_as<T, Employee>) ? "stable_sort_par" : "sort_par       ";
		output += format(lnum, "{} = {:L} us\n", METHOD_NAME, time);

		timer.start();
		radix_sort::sort(vRadix, LAMBDA_RDX, false);
		time = timer.stop();
		output += format(lnum, "radix_sort      = {:L} us ", time);
		correct &= checkEquality(vRadix, vExpected, output);

		vRadix = move(v);
		timer.start();
		radix_sort::sort(vRadix, LAMBDA_RDX, true);
		time = timer.stop();
		output += format(lnum, "radix_sort_par  = {:L} us ", time);
		correct &= checkEquality(vRadix, vExpected, output);

		wrongCounter += !correct;
	}

	template <typename T, typename U = T>
	void testing(size_t n, int shape, string& output, size_t& wrongCounter)
	{
		vector<T> v(generators::generate<T>(n, static_cast<generators::Shape>(shape)));

		testing<T, U>(v, output, wrongCounter);
	}

	void testing(RunParams params)
	{
		const vector<int> SHAPES =
		{
			params.RANDOMIZED, params.SORTED,
			params.REVERSE_SORTED, params.NEARLY_SORTED,
			params.DUPLICATES
		};

		const vector<vector<size_t>> SIZES =
		{
			params.RUN_SIZE, params.RUN_SIZE,
			params.RUN_SIZE, params.RUN_SIZE,
			params.RUN_SIZE, params.RUN_SIZE,
			params.RUN_SIZE, params.RUN_SIZE,
			params.RUN_SIZE, params.RUN_SIZE,
			params.RUN_SIZE_STR,
			params.RUN_SIZE_CLX, params.RUN_SIZE_CLX,
			params.RUN_SIZE_CLX, params.RUN_SIZE_CLX,
			params.RUN_SIZE_CLX
		};

		const vector<int> TYPES =
		{
			params.CHAR, params.UCHAR,
			params.SHORT, params.USHORT,
			params.INT, params.UINT,
			params.LL, params.ULL,
			params.FLOAT, params.DOUBLE,
			params.STRING,
			params.CLX_I32, params.CLX_LL,
			params.CLX_FLT, params.CLX_DBL,
			params.CLX_STR,
		};

		size_t wrongCounter = 0;

		string curDateTime = format(
			"{:%Y-%m-%d %H-%M-%S}",
			chrono::zoned_time{ chrono::current_zone(), floor<chrono::seconds>(chrono::system_clock::now()) }
		);
		ofstream file;
		filesystem::create_directory("tests");
		file.open(format("tests//{}.txt", curDateTime));

		string output = "";
		output += "===============================\n";
		output += "--------TESTING STARTED--------\n";
		output += "===============================\n\n";

		cout << output;
		file << output;

		output.clear();
		output += "===============================\n\n";

		for (int shape = generators::Shape::RANDOMIZED; shape <= generators::Shape::DUPLICATES; shape++)
		{
			if (!SHAPES[shape])
				continue;

			for (int type = Type::CHAR; type <= Type::COMPLEX_STR; type++)
			{
				if (!TYPES[type])
					continue;

				for (const size_t& n : SIZES[type])
				{
					output += std::format(lnum, "{}\nSIZE = {:L} ({})\n\n", type2str[type], n, shape2str[shape]);
					switch (type)
					{
						case Type::CHAR:
							testing<char>(n, shape, output, wrongCounter);
							break;
						case Type::UCHAR:
							testing<unsigned char>(n, shape, output, wrongCounter);
							break;
						case Type::SHORT:
							testing<short>(n, shape, output, wrongCounter);
							break;
						case Type::USHORT:
							testing<unsigned short>(n, shape, output, wrongCounter);
							break;
						case Type::INT:
							testing<int>(n, shape, output, wrongCounter);
							break;
						case Type::UINT:
							testing<unsigned int>(n, shape, output, wrongCounter);
							break;
						case Type::LL:
							testing<long long>(n, shape, output, wrongCounter);
							break;
						case Type::ULL:
							testing<unsigned long long>(n, shape, output, wrongCounter);
							break;
						case Type::FLOAT:
							testing<float>(n, shape, output, wrongCounter);
							break;
						case Type::DOUBLE:
							testing<double>(n, shape, output, wrongCounter);
							break;
						case Type::STRING:
							testing<string>(n, shape, output, wrongCounter);
							break;
						case Type::COMPLEX_I32:
							testing<Employee, decltype(Employee::age)>(n, shape, output, wrongCounter);
							break;
						case Type::COMPLEX_LL:
							testing<Employee, decltype(Employee::id)>(n, shape, output, wrongCounter);
							break;
						case Type::COMPLEX_FLT:
							testing<Employee, decltype(Employee::salary_f)>(n, shape, output, wrongCounter);
							break;
						case Type::COMPLEX_DBL:
							testing<Employee, decltype(Employee::salary)>(n, shape, output, wrongCounter);
							break;
						case Type::COMPLEX_STR:
							testing<Employee, decltype(Employee::name)>(n, shape, output, wrongCounter);
							break;
					}
					output += "\n===============================\n\n";
					cout << output;
					file << output;
					output.clear();
				}
			}
		}

		output += format("WRONG SORTS = {}\n\n", wrongCounter);

		output += "===============================\n";
		output += "--------TESTING STOPPED--------\n";
		output += "===============================\n\n";

		cout << output;
		file << output;

		file.close();
	}
}
