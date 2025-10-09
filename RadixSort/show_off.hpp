#pragma once
#include <vector>
#include <string>

class show_off
{
#pragma region TYPES & FIELDS DECLARATION
	enum Method
	{
		SORT,
		SORT_PAR,
		STABLE_SORT,
		STABLE_SORT_PAR,
		RADIX_SORT
	};

	enum DataType
	{
		INT,
		UINT,
		LL,
		ULL,
		FLOAT,
		DOUBLE,
		STRING,
	};

	struct DataTypeRun {
		DataType type;
		int n;
		std::string output;

		DataTypeRun(DataType type, int n, std::string output) : type(type), n(n), output(output) {}
	};

	static const std::vector<int> RUN_METHOD;
	static const std::vector<DataTypeRun> RUN_DATATYPE;
#pragma endregion

#pragma region SHOW OFF METHODS
	template<typename T>
	static void showOff(std::vector<T>& v, Method method, std::string& output);
	template<typename T>
	static void showOff(int n, std::string& output);
public:	
	static void showOff(int INT, int UINT, int LL, int ULL, int FLOAT, int DOUBLE, int STRING);
#pragma endregion

#pragma region VALIDATION METHODS
private:
	template<typename T>
	static void validate(std::vector<T>& v, std::string& output);
	template<typename T>
	static void validate(int n, std::string& output);
public:
	static void validate(int INT, int UINT, int LL, int ULL, int FLOAT, int DOUBLE, int STRING);
#pragma endregion
};