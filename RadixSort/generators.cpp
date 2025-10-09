#include "generators.hpp"
#include <random>
#include <limits>
#include <iostream>
#include <bit>
using namespace std;

vector<int> generators::generateInts(int n)
{
	vector<int> v;
	v.reserve(n);

	mt19937 gen(69);
	//uniform_int_distribution<int> dist(0, INT_MAX);
	uniform_int_distribution<int> dist(INT_MIN, INT_MAX);

	while (n--)
		v.emplace_back(dist(gen));

	return v;
}

vector<unsigned int> generators::generateUnsignedInts(int n)
{
	vector<unsigned int> v;
	v.reserve(n);

	mt19937 gen(69);
	uniform_int_distribution<int> dist(0, UINT_MAX);

	while (n--)
		v.emplace_back(dist(gen));

	return v;
}

vector<long long> generators::generateLLs(int n)
{
	vector<long long> v;
	v.reserve(n);

	mt19937 gen(69);
	uniform_int_distribution<long long> dist(LLONG_MIN, LLONG_MAX);

	while (n--)
		v.emplace_back(dist(gen));

	return v;
}

vector<unsigned long long> generators::generateULLs(int n)
{
	vector<unsigned long long> v;
	v.reserve(n);

	mt19937 gen(69);
	uniform_int_distribution<unsigned long long> dist(0, ULLONG_MAX);

	while (n--)
		v.emplace_back(dist(gen));

	return v;
}

vector<float> generators::generateFloats(int n) 
{
	vector<float> v;
	v.reserve(n);

	mt19937 gen(69);
	uniform_int_distribution<unsigned int> dist(0, UINT_MAX);

	while (n--)
	{
		unsigned int num = 0;
		while (true) 
		{
			num = dist(gen);

			if ((num & 0x7F800000) != 0x7F800000) 
				break;
		}
		v.emplace_back(bit_cast<float>(num));
	}

	return v;
}

vector<double> generators::generateDoubles(int n) 
{
	vector<double> v;
	v.reserve(n);

	mt19937 gen(69);
	uniform_int_distribution<unsigned long long> dist(0, ULLONG_MAX);

	while (n--)
	{
		unsigned long long num = 0;
		while (true) 
		{
			num = dist(gen);
			if ((num & 0x7F80000000000000) != 0x7F80000000000000) break;
		}
		v.emplace_back(bit_cast<double>(num));
	}

	return v;
}

vector<string> generators::generateStringsFixed(int n, int len)
{
	vector<string> v;
	v.reserve(n);

	mt19937 gen(69);
	//uniform_int_distribution<int> charDist(32, 126);
	uniform_int_distribution<int> charDist(0, 255);

	while (n--)
	{
		string s;
		s.resize(len);

		for (int i = 0; i < len; i++)
			s[i] = static_cast<char>(charDist(gen));

		v.emplace_back(move(s));
	}

	return v;
}

vector<string> generators::generateStrings(int n, int maxLen)
{
	vector<string> v;
	v.reserve(n);

	mt19937 gen(69);
	uniform_int_distribution<int> lenDist(0, maxLen);
	//uniform_int_distribution<int> charDist(32, 126);
	uniform_int_distribution<int> charDist(0, 255);

	while (n--)
	{
		int len = lenDist(gen);
		string s;
		s.resize(len);

		for (int i = 0; i < len; i++)
			s[i] = static_cast<char>(charDist(gen));

		v.emplace_back(move(s));
	}

	return v;
}
