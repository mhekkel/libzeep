#include <iostream>
#include <filesystem>

#include <boost/program_options.hpp>

#include <zeep/xml/document.hpp>

int VERBOSE, TRACE;

using namespace std;
using namespace zeep;
namespace po = boost::program_options;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help", "produce help message")
	    ("verbose", "verbose output")
	    ("trace", "trace parser steps")
	    ("test", "Run a test")
	;
	
	po::positional_options_description p;
	p.add("test", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).
	          options(desc).positional(p).run(), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		cout << desc << endl;
		return 1;
	}
	
	VERBOSE = vm.count("verbose");
	TRACE = vm.count("trace");
	
	if (vm.count("test"))
	{
		std::ifstream file(vm["test"].as<string>());
		
		xml::document doc(file);
	
		xml::writer w(cout);
		w.set_indent(2);
		w.set_wrap(true);
		
		doc.write(w);
	}
	
	return 0;
}
