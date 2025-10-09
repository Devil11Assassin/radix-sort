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

	static void showOff(std::vector<int>& v, Method method, std::string& output);
	static void showOff(std::vector<unsigned int>& v, Method method, std::string& output);
	static void showOff(std::vector<float>& v, Method method, std::string& output);
	static void showOff(std::vector<std::string>& v, Method method, std::string& output);
public:
	static void showOff(int INT, int UINT, int FLOAT, int STRING);
};