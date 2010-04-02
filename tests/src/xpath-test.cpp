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

bool run_test(const xml::element& test)
{
	cout << "----------------------------------------------------------" << endl
		 << "ID: " << test.get_attribute("ID")
		 << endl
		 << "xpath: " << test.get_attribute("xpath") << endl
//		 << "data: " << test.content() << endl
		 << "expected-size: " << test.get_attribute("expected-size") << endl
		 << endl;

	fs::path data_file = fs::current_path() / test.get_attribute("data");
	if (not fs::exists(data_file))
		throw zeep::exception("file does not exist");
	
	xml::document doc;
	doc.set_validating(false);
	
	fs::ifstream file(data_file);
	file >> doc;
	
//	if (VERBOSE)
//		cout << "test doc:" << endl << doc << endl;
	
	xml::xpath xp(test.get_attribute("xpath"));
	xml::node_set ns = xp.evaluate(doc);

	if (VERBOSE)
	{
		int nr = 1;
		foreach (const xml::node* n, ns)
			cout << nr++ << ">> " << *n << endl;
	}
	
	bool result = true;
	
	if (ns.size() != boost::lexical_cast<unsigned int>(test.get_attribute("expected-size")))
	{
		cout << "incorrect number of nodes in returned node-set: " << endl;

		int nr = 1;
		foreach (const xml::node* n, ns)
			cout << nr++ << ">> " << *n << endl;

		result = false;
	}

	string test_attr_name = test.get_attribute("test-name");
	string attr_test = test.get_attribute("test-attr");

	if (not attr_test.empty())
	{
		if (VERBOSE)
			cout << "testing attribute " << test_attr_name << " for " << attr_test << endl;
		
		foreach (const xml::node* n, ns)
		{
			const xml::element* e = dynamic_cast<const xml::element*>(n);
			if (e == NULL)
				continue;
			
			if (e->get_attribute(test_attr_name) != attr_test)
			{
				cout << "expected attribute content is not found for node " << e->name() << endl;
				result = false;
			}
		}
	}
	
	if (result)
		cout << "Test passed" << endl;
	else
		cout << "Test failed" << endl;
	
	return result;
}

void run_tests(const fs::path& file)
{
	if (not fs::exists(file))
		throw zeep::exception("test file does not exist");
	
	xml::document doc;
	doc.set_validating(false);
	fs::ifstream input(file);
	input >> doc;

	string base = doc.root()->get_attribute("xml:base");
	if (not base.empty())
		fs::current_path(base);
	
	int nr_of_tests = 0, failed_nr_of_tests = 0;
	
	xml::node_set tests = doc.root()->children();
	foreach (const xml::node* n, tests)
	{
		const xml::element* test = dynamic_cast<const xml::element*>(n);
		if (test != NULL)
		{
			++nr_of_tests;
			if (run_test(*test) == false)
				++failed_nr_of_tests;
		}
	}
	
	cout << endl;
	if (failed_nr_of_tests == 0)
		cout << "All tests passed successfully" << endl;
	else
		cout << failed_nr_of_tests << " out of " << nr_of_tests << " failed" << endl;
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
		fs::path xmlconfFile("XPath-Test-Suite/xpath-tests.xml");
		if (vm.count("test"))
			xmlconfFile = vm["test"].as<string>();
		
		run_tests(xmlconfFile);
	}
	catch (std::exception& e)
	{
		cout << "exception: " << e.what() << endl;
		return 1;
	}
	
	return 0;
}
