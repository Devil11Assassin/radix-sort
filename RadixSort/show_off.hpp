#pragma once
#pragma region HEADERS
#include <format>
#include <locale>
#include <string>
#include <vector>
#pragma endregion

class show_off
{
#pragma region TYPES & FIELDS DECLARATION
	enum Method
	{
		SORT,
		SORT_PAR,
		STABLE_SORT,
		STABLE_SORT_PAR,
		RADIX_SORT,
		RADIX_SORT_PAR
	};

	enum DataType
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
		COMPLEX_FP,
		COMPLEX_STR,
	};

	struct RunParams
	{
		const int CHAR	      = 0;
		const int UCHAR	      = 0;
		const int SHORT	      = 0;
		const int USHORT      = 0;
		const int INT	      = 0;
		const int UINT	      = 0;
		const int LL	      = 0;
		const int ULL	      = 0;
		const int FLOAT	      = 0;
		const int DOUBLE      = 0;
		const int STRING      = 0;
		const int COMPLEX_I32 = 0;
		const int COMPLEX_LL  = 0;
		const int COMPLEX_FP  = 0;
		const int COMPLEX_STR = 0;
	};

	static const std::vector<std::string> method2str;
	static const std::vector<std::string> type2str;
	static const std::locale lnum;

	struct DataTypeRun 
	{
		DataType type;
		int n;
		std::string output;

		DataTypeRun(DataType type, int n) : type(type), n(n) 
		{
			output = std::format(lnum, "{}\nSIZE = {:L}\n\n", type2str[type], n);
		}
	};

	static const std::vector<int> RUN_METHOD;
	static const std::vector<DataTypeRun> RUN_DATATYPE;

	template <size_t S> struct fp2i_imp;
	template <> struct fp2i_imp<2> { using type = std::uint16_t; };
	template <> struct fp2i_imp<4> { using type = std::uint32_t; };
	template <> struct fp2i_imp<8> { using type = std::uint64_t; };

	template <typename T>
	using fp2i = fp2i_imp<sizeof(T)>::type;
#pragma endregion

#pragma region SHOW OFF METHODS
	template<typename T, typename U = T>
	static void showOff(std::vector<T>& v, Method method, std::string& output);
	template<typename T, typename U = T>
	static void showOff(int n, std::string& output);
public:	
	static void showOff(RunParams params);
#pragma endregion

#pragma region VALIDATION METHODS
private:
	template<typename T, typename U = T>
	static void validate(std::vector<T>& v, std::string& output);
	template<typename T, typename U = T>
	static void validate(int n, std::string& output);
public:
	static void validate(RunParams params);
#pragma endregion
};