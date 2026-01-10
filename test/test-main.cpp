//          Copyright Maarten L. Hekkelman 2025
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define CATCH_CONFIG_RUNNER

#include <catch2/catch_session.hpp>

#include "test-main.hpp"

std::filesystem::path gTestDir;
std::filesystem::path gDocrootDir;

int main(int argc, char *argv[])
{
	gTestDir = std::filesystem::current_path();

	Catch::Session session; // There must be exactly one instance

	// Build a new parser on top of Catch2's
	using namespace Catch::Clara;

	auto cli = session.cli();                                   // Get Catch2's command line parser
	cli |= Opt(gTestDir, "data-dir")                            // bind variable to a new option, with a hint string
	           ["-t"]["--data-dir"]                             // the option names it will respond to
	       ("The directory containing the tests")               // description string for the help output
	       | Opt(gDocrootDir, "docroot")                        // bind variable to a new option, with a hint string
	             ["-d"]["--docroot"]                            // the option names it will respond to
	       ("The directory containing the docroot data files"); // description string for the help output

	// Now pass the new composite back to Catch2 so it uses that
	session.cli(cli);

	// Let Catch2 (using Clara) parse the command line
	int returnCode = session.applyCommandLine(argc, argv);
	if (returnCode != 0) // Indicates a command line error
		return returnCode;

	return session.run();
}
