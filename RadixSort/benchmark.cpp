#include "benchmark.hpp"

#include <chrono>
#include <execution>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

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
	const vector<string> method2str =
	{
		"sort           ", 
		"sort_par       ", 
		"stable_sort    ", 
		"stable_sort_par", 
		"radix_sort     ", 
		"radix_sort_par ",
	};
	const vector<string> type2str =
	{
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
			else if constexpr (same_as<U, decltype(Employee::id)>)
				return &Employee::id;
			else if constexpr (same_as<U, decltype(Employee::salary_f)>)
				return &Employee::salary_f;
			else if constexpr (same_as<U, decltype(Employee::salary)>)
				return &Employee::salary;
			else if constexpr (same_as<U, decltype(Employee::name)>)
				return &Employee::name;
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
	void benchmark(size_t n, int iterations, const vector<int> METHODS, string& output)
	{
		vector<T> v(generators::generate<T>(n));

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
		output += "===========================\n";
		output += "-----BENCHMARK STARTED-----\n";
		output += "===========================\n\n";

		cout << output;
		file << output;

		output.clear();
		output = "=========================\n\n";

		for (int type = Type::CHAR; type <= Type::COMPLEX_STR; type++)
		{
			if (TYPES[type])
			{
				for (const size_t& n : SIZES[type])
				{
					output += std::format(lnum, "{}\nSIZE = {:L}\n\n", type2str[type], n);
					switch (type)
					{
						case Type::CHAR:
							benchmark<char>(n, iterations, METHODS, output);
							break;
						case Type::UCHAR:
							benchmark<unsigned char>(n, iterations, METHODS, output);
							break;
						case Type::SHORT:
							benchmark<short>(n, iterations, METHODS, output);
							break;
						case Type::USHORT:
							benchmark<unsigned short>(n, iterations, METHODS, output);
							break;
						case Type::INT:
							benchmark<int>(n, iterations, METHODS, output);
							break;
						case Type::UINT:
							benchmark<unsigned int>(n, iterations, METHODS, output);
							break;
						case Type::LL:
							benchmark<long long>(n, iterations, METHODS, output);
							break;
						case Type::ULL:
							benchmark<unsigned long long>(n, iterations, METHODS, output);
							break;
						case Type::FLOAT:
							benchmark<float>(n, iterations, METHODS, output);
							break;
						case Type::DOUBLE:
							benchmark<double>(n, iterations, METHODS, output);
							break;
						case Type::STRING:
							benchmark<string>(n, iterations, METHODS, output);
							break;
						case Type::COMPLEX_I32:
							benchmark<Employee, decltype(Employee::age)>(n, iterations, METHODS, output);
							break;
						case Type::COMPLEX_LL:
							benchmark<Employee, decltype(Employee::id)>(n, iterations, METHODS, output);
							break;
						case Type::COMPLEX_FLT:
							benchmark<Employee, decltype(Employee::salary_f)>(n, iterations, METHODS, output);
							break;
						case Type::COMPLEX_DBL:
							benchmark<Employee, decltype(Employee::salary)>(n, iterations, METHODS, output);
							break;
						case Type::COMPLEX_STR:
							benchmark<Employee, decltype(Employee::name)>(n, iterations, METHODS, output);
							break;
					}
					output += "\n=========================\n\n";
					cout << output;
					file << output;
					output.clear();
				}
			}
		}

		output += "===========================\n";
		output += "-----BENCHMARK STOPPED-----\n";
		output += "===========================\n\n";

		cout << output;
		file << output;

		file.close();
	}
}
