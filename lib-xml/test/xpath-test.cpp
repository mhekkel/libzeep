#include <iostream>
#include <string>
#include <list>
#include <fstream>
#include <filesystem>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <zeep/xml/document.hpp>
#include <zeep/exception.hpp>
#include <zeep/xml/parser.hpp>
#include <zeep/xml/xpath.hpp>

using namespace std;
using namespace zeep;
namespace po = boost::program_options;
namespace fs = std::filesystem;
namespace ba = boost::algorithm;

#define foreach BOOST_FOREACH

int VERBOSE;
int TRACE;

ostream& operator<<(ostream& os, const xml::node& n)
{
	n.write(os , {});
	return os;
}


bool run_test(const xml::element& test)
{
	cout << "----------------------------------------------------------" << endl
		 << "ID: " << test.attr("ID")
		 << endl
		 << "xpath: " << test.attr("xpath") << endl
//		 << "data: " << test.content() << endl
//		 << "expected-size: " << test.attr("expected-size") << endl
		 << endl;

	fs::path data_file = fs::current_path() / test.attr("data");
	if (not fs::exists(data_file))
		throw zeep::exception("file does not exist");
	
	std::ifstream file(data_file, ios::binary);

	xml::document doc;
	file >> doc;
	
	if (VERBOSE)
		cout << "test doc:" << endl << doc << endl;
	
	xml::xpath xp(test.attr("xpath"));

	xml::context context;
	for (const xml::element* e: test.find("var"))
		context.set(e->attr("name"), e->attr("value"));
	
	xml::node_set ns = xp.evaluate<xml::node>(*doc.root(), context);

	if (VERBOSE)
	{
		int nr = 1;
		for (const xml::node* n: ns)
			cout << nr++ << ">> " << *n << endl;
	}
	
	bool result = true;
	
	if (ns.size() != boost::lexical_cast<unsigned int>(test.attr("expected-size")))
	{
		cout << "incorrect number of nodes in returned node-set" << endl
			 << "expected: " << test.attr("expected-size") << endl;

		result = false;
	}

	string test_attr_name = test.attr("test-name");
	string attr_test = test.attr("test-attr");

	if (not attr_test.empty())
	{
		if (VERBOSE)
			cout << "testing attribute " << test_attr_name << " for " << attr_test << endl;
		
		for (const xml::node* n: ns)
		{
			const xml::element* e = dynamic_cast<const xml::element*>(n);
			if (e == NULL)
				continue;
			
			if (e->attr(test_attr_name) != attr_test)
			{
				cout << "expected attribute content is not found for node " << e->get_qname() << endl;
				result = false;
			}
		}
	}
	
	if (result)
		cout << "Test passed" << endl;
	else
	{
		cout << "Test failed" << endl;
		
		int nr = 1;
		for (const xml::node* n: ns)
			cout << nr++ << ") " << *n << endl;
	}
	
	return result;
}

void run_tests(const fs::path& file)
{
	if (not fs::exists(file))
		throw zeep::exception("test file does not exist");
	
	std::ifstream input(file, ios::binary);

	xml::document doc;
	input >> doc;

	fs::path dir = fs::absolute(file).parent_path().parent_path();
	if (not dir.empty())
		fs::current_path(dir);

	string base = doc.front().attr("xml:base");
	if (not base.empty())
		fs::current_path(base);
	
	int nr_of_tests = 0, failed_nr_of_tests = 0;
	
	for (const xml::element* test: doc.find("//xpath-test"))
	{
		++nr_of_tests;
		if (run_test(*test) == false)
			++failed_nr_of_tests;
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
	    ("help,h", "produce help message")
	    ("verbose,v", "verbose output")
	    ("test,t", "Run SUN test suite")
	    ("trace,T", "Trace productions in parser")
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
