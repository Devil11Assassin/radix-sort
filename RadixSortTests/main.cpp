#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include "arg_parser.hpp"

int main(int argc, char* argv[])
{
	bool startTesting = arg_parser::parse(argc, argv);
	if (!startTesting)
		return 0;

	doctest::Context context;
	context.setOption("-ni", true);
	context.setOption("-nv", true);
	context.applyCommandLine(argc, argv);
	return context.run();
}
