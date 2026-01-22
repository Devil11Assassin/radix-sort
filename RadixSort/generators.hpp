#pragma once
#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <vector>

struct Employee
{
	int32_t age = 0;
	long long id = 0L;
	float salary_f = 0.f;
	double salary = 0.0;
	std::string name = "";

	Employee() = default;

	Employee(
		decltype(Employee::age) age, decltype(Employee::id) id,
		decltype(Employee::salary_f) salary_f, decltype(Employee::salary) salary,
		decltype(Employee::name) name
	) : age(age), id(id), salary_f(salary_f), salary(salary), name(name) {}

	bool operator==(const Employee& other) const
	{
		return age == other.age 
			&& id == other.id
			&& std::strong_order(salary_f, other.salary_f) == 0
			&& std::strong_order(salary, other.salary) == 0
			&& name == other.name;
	}
};

namespace generators
{
	namespace internal 
	{
		template <typename T>
		concept unknown = !std::integral<T> 
			&& !std::floating_point<T> 
			&& !std::same_as<T, std::string>
			&& !std::same_as<T, Employee>;

		inline constexpr int SEED = 69;

		template <size_t S> struct fp2i_impl;
		template <> struct fp2i_impl<2> { using type = std::uint16_t; static constexpr type mask = 0x7C00; };
		template <> struct fp2i_impl<4> { using type = std::uint32_t; static constexpr type mask = 0x7F800000; };
		template <> struct fp2i_impl<8> { using type = std::uint64_t; static constexpr type mask = 0x7FF0000000000000; };

		template <typename T>
		using fp2i = fp2i_impl<sizeof(T)>;

		template <size_t S, typename T> struct gen_t_impl;
		template <typename T> struct gen_t_impl<1, T> { using type = std::int32_t; };
		template <typename T> struct gen_t_impl<2, T> { using type = std::int32_t; };
		template <typename T> struct gen_t_impl<4, T> { using type = T; };
		template <typename T> struct gen_t_impl<8, T> { using type = T; };

		template <typename T>
		using gen_t = gen_t_impl<sizeof(T), T>::type;

		template <std::integral T>
		inline std::vector<T> generate_impl(std::size_t n)
		{
			std::vector<T> v;
			v.reserve(n);

			std::mt19937_64 gen(SEED);
			std::uniform_int_distribution<gen_t<T>> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());

			while (n--)
				v.emplace_back(dist(gen));

			return v;
		}

		template <std::floating_point T>
		inline std::vector<T> generate_impl(std::size_t n, bool testStrongOrder = true)
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
	
		template <typename T> requires std::same_as<T, std::string>
		inline std::vector<T> generate_impl(std::size_t n, std::size_t maxLen = 20, const bool fixed = false)
		{
			std::vector<T> v;
			v.reserve(n);

			std::mt19937_64 gen(69);
			std::uniform_int_distribution<std::size_t> lenDist(0, maxLen);
			std::uniform_int_distribution<int> charDist(0, 255); // or (32, 126)

			while (n--)
			{
				std::size_t len = (fixed) ? maxLen : lenDist(gen);

				T s(len, '\0');

				for (std::size_t i = 0; i < len; i++)
					s[i] = static_cast<char>(charDist(gen));

				v.emplace_back(std::move(s));
			}

			return v;
		}

		template <typename T> requires std::same_as<T, Employee>
		inline std::vector<T> generate_impl(std::size_t n)
		{
			std::vector<T> v;
			v.reserve(n);

			std::vector<decltype(Employee::age)> ages = generate_impl<decltype(Employee::age)>(n);
			std::vector<decltype(Employee::id)> ids = generate_impl<decltype(Employee::id)>(n);
			std::vector<decltype(Employee::salary_f)> salaries_f = generate_impl<decltype(Employee::salary_f)>(n);
			std::vector<decltype(Employee::salary)> salaries = generate_impl<decltype(Employee::salary)>(n);
			std::vector<decltype(Employee::name)> names = generate_impl<decltype(Employee::name)>(n);

			for (size_t i = 0; i < n; i++)
				v.emplace_back(ages[i], ids[i], salaries_f[i], salaries[i], names[i]);

			return v;
		}

		template<unknown T>
		inline std::vector<T> generate_impl(std::size_t n)
		{
			static_assert(sizeof(T) == 0, "ERROR: Unable to generate vector!\nCAUSE: Unsupported type!\n");
		}
	}

	template <typename T>
	inline std::vector<T> generate(std::size_t n)
	{
		return internal::generate_impl<T>(n);
	}
};
