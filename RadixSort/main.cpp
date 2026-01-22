#include <iostream>
#include "radix_sort.hpp"
#include "show_off.hpp"

int main()
{
	//show_off::showOff
	show_off::validate
	({
		.CHAR        = 1,
		.UCHAR       = 1,
		.SHORT       = 1,
		.USHORT      = 1,
		.INT         = 1,
		.UINT        = 1,
		.LL          = 1,
		.ULL         = 1,
		.FLOAT       = 1,
		.DOUBLE      = 1,
		.STRING      = 1,
		.COMPLEX_I32 = 1,
		.COMPLEX_LL  = 1,
		.COMPLEX_FLT = 1,
		.COMPLEX_DBL = 1,
		.COMPLEX_STR = 1,
	});

	std::cout << "meow\n";
}