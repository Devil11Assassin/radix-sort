#include <format>
#include <iostream>
#include "radix_sort.hpp"
#include "show_off.hpp"

int main()
{
	//show_off::showOff
	show_off::validate
	({
		//.CHAR	 = 1,
		//.UCHAR	 = 1,
		//.SHORT	 = 1,
		//.USHORT	 = 1,
		//.INT	 = 1,
		//.UINT	 = 1,
		//.LL		 = 1,
		//.ULL	 = 1,
		//.FLOAT	 = 1,
		//.DOUBLE	 = 1,
		//.STRING	 = 1,
		.COMPLEX = 1,
	});

	std::cout << "meow\n";

	//struct Employee
	//{
	//	long long id = 0L;
	//	std::string name = "";
	//	float salary = 0.f;
	//	int age = 0;

	//	Employee() = default;

	//	Employee(long long id, std::string name, float salary, int age)
	//		: id(id), name(name), salary(salary), age(age) {}
	//};

	//std::vector<Employee> employees = {
	//	Employee(5, "Meow", 5000, 18),
	//	Employee(2, "psps",  500, 19),
	//	Employee(3, "BRRR", 6000, 18),
	//	Employee(1, "Hmmm",  750, 20),
	//	Employee(6, "oooo", 1000, 16)
	//};

	//radix_sort::sort(employees, [](const Employee& e) { return e.age; });
	////radix_sort::sort(employees, [](const Employee& e) { return e; });

	//for (const auto& e : employees)
	//{
	//	std::cout << std::format("ID: {}, Age: {}, Salary: {}, Name: {}\n",
	//		e.id, e.age, e.salary, e.name);
	//}
}