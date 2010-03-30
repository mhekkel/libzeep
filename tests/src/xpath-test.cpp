#include <iostream>

#include <string>
#include <list>

#include <boost/foreach.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "zeep/xml/document.hpp"
#include "zeep/exception.hpp"
#include "zeep/xml/parser.hpp"
#include "zeep/xml/expat_doc.hpp"
#include "zeep/xml/libxml2_doc.hpp"
#include "zeep/xml/writer.hpp"
#include "zeep/xml/xpath.hpp"

using namespace std;
using namespace zeep;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

#define foreach BOOST_FOREACH

int VERBOSE;
int TRACE;

void run_test(const xml::element& test)
{
	cout << "----------------------------------------------------------" << endl
		 << "ID: " << test.get_attribute("ID")
		 << endl
		 << "xpath: " << test.get_attribute("xpath") << endl
//		 << "data: " << test.content() << endl
		 << "expected-size: " << test.get_attribute("expected-size") << endl
		 << endl;
	
	xml::document doc;
	doc.set_validating(false);
	istringstream s(test.content());
	s >> doc;
	
	xml::xpath xp(test.get_attribute("xpath"));
	xml::node_set result = xp.evaluate(doc);
	
	if (result.size() != boost::lexical_cast<unsigned int>(test.get_attribute("expected-size")))
		cout << "Test failed" << endl;
}

void run_tests(const fs::path& file)
{
	xml::document doc;
	doc.set_validating(false);
	fs::ifstream input(file);
	input >> doc;
	
	xml::node_set tests = doc.root()->children();
	foreach (const xml::node& n, tests)
	{
		const xml::element* test = dynamic_cast<const xml::element*>(&n);
		if (test != NULL)
			run_test(*test);
	}
}

int main(int argc, char* argv[])
{
	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help", "produce help message")
	    ("verbose", "verbose output")
	    ("test", "Run SUN test suite")
	    ("trace", "Trace productions in parser")
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
	
	try
	{
		fs::path xmlconfFile("xpath-tests.xml");
		if (vm.count("test"))
			xmlconfFile = vm["test"].as<string>();
		
		run_tests(xmlconfFile);
	}
	catch (std::exception& e)
	{
		cout << e.what() << endl;
		return 1;
	}
	
	return 0;
}
