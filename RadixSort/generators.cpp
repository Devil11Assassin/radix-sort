#include "generators.hpp"

using namespace std;

vector<string> generators::generateStrings(int n, int maxLen, const bool fixed)
{
	vector<string> v;
	v.reserve(n);

	mt19937_64 gen(69);
	uniform_int_distribution<int> lenDist(0, maxLen);
	uniform_int_distribution<int> charDist(0, 255); // or (32, 126)

	while (n--)
	{
		int len = (fixed)? maxLen : lenDist(gen);
		
		string s;
		s.resize(len);

		for (int i = 0; i < len; i++)
			s[i] = static_cast<char>(charDist(gen));

		v.emplace_back(move(s));
	}

	return v;
}
