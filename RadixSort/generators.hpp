#pragma once
#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <vector>

class generators
{
	static constexpr int SEED = 69;

	template <size_t S> struct fp2i_imp;
	template <> struct fp2i_imp<2> { using type = std::uint16_t; static constexpr type mask = 0x7C00; };
	template <> struct fp2i_imp<4> { using type = std::uint32_t; static constexpr type mask = 0x7F800000; };
	template <> struct fp2i_imp<8> { using type = std::uint64_t; static constexpr type mask = 0x7FF0000000000000; };

	template <typename T>
	using fp2i = fp2i_imp<sizeof(T)>;

	static std::vector<std::string> generateStrings(int n, int maxLen = 20, const bool fixed = false);

public:
	template <std::integral T>
	static std::vector<T> generate(int n)
	{
		std::vector<T> v;
		v.reserve(n);

		std::mt19937_64 gen(SEED);
		std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());

		while (n--)
			v.emplace_back(dist(gen));

		return v;
	}

	template <std::floating_point T>
	static std::vector<T> generate(int n, bool testStrongOrder = true)
	{
		static_assert(
			std::numeric_limits<T>::is_iec559,
			"ERROR: generators.hpp requires IEEE-754/IEC-559 compliance.\n"
		);

		std::vector<T> v;
		v.reserve(n);

		using U = fp2i<T>::type;
		constexpr U MASK = fp2i<T>::mask;

		std::mt19937_64 gen(SEED);
		std::uniform_int_distribution<U> dist(std::numeric_limits<U>::min(), std::numeric_limits<U>::max());

		while (n--)
		{
			U num = 0;
			while (true)
			{
				num = dist(gen);

				if (testStrongOrder || (num & MASK) != MASK)
					break;
			}
			v.emplace_back(bit_cast<T>(num));
		}

		return v;
	}
	
	template<typename T> requires (!std::integral<T> && !std::floating_point<T>)
	static std::vector<T> generate(int n)
	{
		if constexpr (std::same_as<T, std::string>)
			return generateStrings(n);
		else
			static_assert(sizeof(T) == 0, "ERROR: Unable to generate vector!\nCAUSE: Unsupported type!\n");
	}
};
