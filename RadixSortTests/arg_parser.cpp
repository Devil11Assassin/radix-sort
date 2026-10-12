#include "arg_parser.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <iostream>
#include <optional>
#include <string_view>

#include "generators.hpp"
#include "test.hpp"

// ============================
// -----Arg Parser Helpers-----
// ============================

namespace
{
	constexpr std::string_view kHelpArg = "-meow";

	void printIntro()
	{
		constexpr std::string_view kColorCyan = "\033[36m";
		constexpr std::string_view kColorDefault = "\033[0m";
		
		std::cout << kColorCyan << "\n[radix_sort] " << kColorDefault;
		std::cout << std::format("run with \"{}\" for options\n", kHelpArg);
	}

	struct DoctestArg
	{
		std::string_view name;
		std::string_view info;
	};

	constexpr std::array kDoctestArgMap = {
		DoctestArg("-h",           "prints all doctest args"),
		DoctestArg("-ltc",         "lists matching tests"),
		DoctestArg("-lts",         "lists matching test suites"),
		DoctestArg("-tc=<filter>", "filters test cases by name"),
		DoctestArg("-ts=<filter>", "filters test cases by suite"),
	};

	enum class ArgType
	{
		MEOW,
		SEQ,
		PAR,
		SMALL,
		LARGE,
		SHAPE,
		VERBOSE,
		UNKNOWN // keep this last yo
	};

	struct ArgMap
	{
		ArgType argType;
		std::string_view name;
		std::string_view info;
	};

	constexpr std::array kArgMap = {
		ArgMap(ArgType::MEOW,    kHelpArg,         "prints all custom args"),
		ArgMap(ArgType::SEQ,     "-seq",           "runs radix_sort in sequential mode only"),
		ArgMap(ArgType::PAR,     "-par",           "runs radix_sort in parallel mode only"),
		ArgMap(ArgType::SMALL,   "-small",         "runs tests on small data only [0 - 1m]"),
		ArgMap(ArgType::LARGE,   "-large",         "runs tests on large data only [10m, 100m]"),
		ArgMap(ArgType::SHAPE,   "-shape <shape>", "specifies test data shape"),
		ArgMap(ArgType::VERBOSE, "-verbose",       "prints extra info during test"),
	};

	ArgType getArgType(std::string_view argName)
	{
		auto it = std::find_if(kArgMap.begin(), kArgMap.end(), [argName](const auto& item) 
		{
			return item.name == argName;
		});
		
		return (it != kArgMap.end()) ? it->argType : ArgType::UNKNOWN;
	}

	void printArgs()
	{
		std::cout << "\nDoctest Args:\n";
		for (const auto& arg : kDoctestArgMap)
			std::cout << std::format("\t{:<14} | {}\n", arg.name, arg.info);
		std::cout << "\nCustom Args:\n";
		for (const auto& arg : kArgMap)
			std::cout << std::format("\t{:<14} | {}\n", arg.name, arg.info);
		std::cout << "\n";
	}
}

// ======================
// -----Shape Parser-----
// ======================

namespace
{
	using Shape = generators::Shape;

	struct ShapeMap
	{
		Shape shape;
		std::string_view name;
	};

	constexpr std::array kShapeMap = {
		ShapeMap(Shape::RANDOMIZED, "rand"),
		ShapeMap(Shape::SORTED, "sorted"),
		ShapeMap(Shape::REVERSE_SORTED, "rsorted"),
		ShapeMap(Shape::NEARLY_SORTED, "nsorted"),
		ShapeMap(Shape::DUPLICATES, "dup"),
	};

	std::optional<Shape> getShape(std::string_view shapeName)
	{
		auto it = std::find_if(kShapeMap.begin(), kShapeMap.end(), [shapeName](const auto& item)
		{
			return item.name == shapeName;
		});

		if (it != kShapeMap.end())
			return it->shape;
		return std::nullopt;
	}

	void printShapes()
	{
		std::cout << "\nWARN: -shape arg passed but no valid shape is given.\n";
		std::cout << "USAGE: -shape <shape>\n";
		std::cout << "Shapes:\n";

		for (const auto& shape : kShapeMap)
			std::cout << std::format("\t{}\n", shape.name);
		std::cout << "\n";
	}

	bool parseShape(int argc, char* argv[], std::size_t& i)
	{
		if (i + 1 >= static_cast<std::size_t>(argc))
		{
			printShapes();
			return false;
		}

		std::optional<Shape> shape = getShape(argv[i + 1]);
		if (!shape.has_value())
		{
			printShapes();
			return false;
		}

		i++;
		test_params::testAllShapes = false;
		test_params::initShape = shape.value();
		
		return true;
	}
}

// ====================
// -----Arg Parser-----
// ====================

namespace arg_parser
{
	bool parse(int argc, char* argv[])
	{
		printIntro();

		for (std::size_t i = 1; i < static_cast<std::size_t>(argc); i++)
		{
			switch (getArgType(argv[i]))
			{
				case ArgType::MEOW:
				{
					printArgs();
					return false;
				}
				case ArgType::SEQ:
				{
					test_params::parConfig = test_params::ParConfig::SEQ;
					break;
				}
				case ArgType::PAR:
				{
					test_params::parConfig = test_params::ParConfig::PAR;
					break;
				}
				case ArgType::SMALL:
				{
					test_params::sizeConfig = test_params::SizeConfig::SMALL;
					break;
				}
				case ArgType::LARGE:
				{
					test_params::sizeConfig = test_params::SizeConfig::LARGE;
					break;
				}
				case ArgType::SHAPE:
				{
					if (!parseShape(argc, argv, i))
						return false;
					break;
				}
				case ArgType::VERBOSE:
				{
					test_params::loggingEnabled = true;
					break;
				}
			}
		}

		return true;
	}
}
