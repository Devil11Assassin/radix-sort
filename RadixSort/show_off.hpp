#pragma once
#include <vector>
#include <string>

class show_off
{
	enum Method
	{
		SORT,
		SORT_PAR,
		STABLE_SORT,
		STABLE_SORT_PAR,
		RADIX_SORT
	};

	static const std::vector<int> RUN_METHOD;

	static void showOff(std::vector<int>& v, Method method, std::string& output);
	static void showOff(std::vector<unsigned int>& v, Method method, std::string& output);
	static void showOff(std::vector<float>& v, Method method, std::string& output);
	static void showOff(std::vector<std::string>& v, Method method, std::string& output);
public:	
	static void showOff(int INT, int UINT, int FLOAT, int STRING);

private:
	static void validate(std::vector<int>& v, std::string& output);
	static void validate(std::vector<unsigned int>& v, std::string& output);
	static void validate(std::vector<long long>& v, std::string& output);
	static void validate(std::vector<unsigned long long>& v, std::string& output);
	static void validate(std::vector<float>& v, std::string& output);
	static void validate(std::vector<double>& v, std::string& output);
	static void validate(std::vector<std::string>& v, std::string& output);
public:
	static void validate(int INT, int UINT, int LL, int ULL, int FLOAT, int DOUBLE, int STRING);
};