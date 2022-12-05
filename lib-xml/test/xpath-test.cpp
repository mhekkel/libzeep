#include <iostream>
#include <string>
#include <list>
#include <fstream>
#include <filesystem>

#include <zeep/xml/document.hpp>
#include <zeep/exception.hpp>
#include <zeep/xml/parser.hpp>
#include <zeep/xml/xpath.hpp>

using namespace std;
using namespace zeep;
namespace fs = std::filesystem;

#define foreach BOOST_FOREACH

int VERBOSE;

ostream& operator<<(ostream& os, const xml::node& n)
{
	n.write(os , {});
	return os;
}


bool run_test(const xml::element& test)
{
	if (VERBOSE)
	{
		cout << "----------------------------------------------------------" << endl
			<< "ID: " << test.get_attribute("ID")
			<< endl
			<< "xpath: " << test.get_attribute("xpath") << endl
	//		 << "data: " << test.content() << endl
	//		 << "expected-size: " << test.attr("expected-size") << endl
			<< endl;
	}

	fs::path data_file = fs::current_path() / test.get_attribute("data");
	if (not fs::exists(data_file))
		throw zeep::exception("file does not exist");
	
	std::ifstream file(data_file, ios::binary);

	xml::document doc;
	file >> doc;
	
	if (VERBOSE)
		cout << "test doc:" << endl << doc << endl;
	
	xml::xpath xp(test.get_attribute("xpath"));

	xml::context context;
	for (const xml::element* e: test.find("var"))
		context.set(e->get_attribute("name"), e->get_attribute("value"));
	
	xml::node_set ns = xp.evaluate<xml::node>(*doc.root(), context);

	if (VERBOSE)
	{
		int nr = 1;
		for (const xml::node* n: ns)
			cout << nr++ << ">> " << *n << endl;
	}
	
	bool result = true;
	
	if (ns.size() != std::stoul(test.get_attribute("expected-size")))
	{
		cout << "incorrect number of nodes in returned node-set" << endl
			 << "expected: " << test.get_attribute("expected-size") << endl;

		result = false;
	}

	string test_attr_name = test.get_attribute("test-name");
	string attr_test = test.get_attribute("test-attr");

	if (not attr_test.empty())
	{
		if (VERBOSE)
			cout << "testing attribute " << test_attr_name << " for " << attr_test << endl;
		
		for (const xml::node* n: ns)
		{
			const xml::element* e = dynamic_cast<const xml::element*>(n);
			if (e == NULL)
				continue;
			
			if (e->get_attribute(test_attr_name) != attr_test)
			{
				cout << "expected attribute content is not found for node " << e->get_qname() << endl;
				result = false;
			}
		}
	}
	
	if (VERBOSE)
	{
		if (result)
			cout << "Test passed" << endl;
		else
		{
			cout << "Test failed" << endl;
			
			int nr = 1;
			for (const xml::node* n: ns)
				cout << nr++ << ") " << *n << endl;
		}
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

	string base = doc.front().get_attribute("xml:base");
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
		cout << "*** No errors detected" << endl;
	else
	{
		cout << failed_nr_of_tests << " out of " << nr_of_tests << " failed" << endl;
		if (not VERBOSE)
			cout << "Run with --verbose to see the errors" << endl;
	}
}

int main(int argc, char* argv[])
{
	using namespace std::literals;
	fs::path xmlconfFile("XPath-Test-Suite/xpath-tests.xml");

	for (int i = 1; i < argc; ++i)
	{
		if (argv[i] == "-v"s)
		{
			++VERBOSE;
			continue;
		}

		xmlconfFile = argv[i];
	}

	try
	{
		run_tests(xmlconfFile);
	}
	catch (std::exception& e)
	{
		cout << "exception: " << e.what() << endl;
		return 1;
	}
	
	return 0;
}
