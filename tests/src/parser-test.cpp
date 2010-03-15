#include <iostream>

#include <string>
#include <list>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "zeep/xml/document.hpp"
#include "zeep/exception.hpp"
#include "zeep/xml/expat_doc.hpp"
#include "zeep/xml/libxml2_doc.hpp"

using namespace std;
using namespace zeep;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

#define DOC		xml::document
//#define DOC		xml::expat_doc
//#define DOC		xml::libxml2_doc

int VERBOSE;
int failed_tests, dubious_tests, error_tests, total_tests;

void run_valid_test(istream& is, fs::path& outfile)
{
	DOC indoc(is);
	
	stringstream s;
	if (indoc.root() != NULL)
		s << *indoc.root();
	string s1 = s.str();
	ba::trim(s1);

	if (fs::is_directory(outfile))
		;
	else if (fs::exists(outfile))
	{
		fs::ifstream out(outfile);
		string s2, line;
		while (not out.eof())
		{
			getline(out, line);
			s2 += line;
		}
		ba::trim(s2);
		
		if (s1 != s2)
		{
			if (VERBOSE)
			{
				cout << "output differs: " << endl\
					 << s1 << endl
					 << s2 << endl
					 << endl;
			}
			
			xml::document a(s1);
			xml::document b(s2);
			
			if (a == b)
				++dubious_tests;
			else
				++failed_tests;
		}
	}
	else
		cout << "skipped output compare for " << outfile << endl;
}


void run_test(xml::node& test, fs::path base_dir)
{
	++total_tests;
	
	fs::path input(base_dir / test.get_attribute("URI"));
	fs::path output(base_dir / test.get_attribute("OUTPUT"));

	if (not fs::exists(input))
	{
		cout << "test file " << input << " does not exist" << endl;
		return;
	}
	
	fs::current_path(input.branch_path());

	fs::ifstream is(input);
	if (not is.is_open())
		throw zeep::exception("test file not open");
	
	try
	{
		fs::current_path(input.branch_path());
		
		if (test.get_attribute("TYPE") == "valid")
			run_valid_test(is, output);
		else // if (test.get_attribute("TYPE") == "not-wf" or test.get_attribute("TYPE") == "error" )
		{
			try
			{
				DOC doc(is);
				throw zeep::exception("invalid document, should have failed");
			}
			catch (...) {}
		}
	}
	catch (std::exception& e)
	{
		++error_tests;
		
		if (VERBOSE)
		{
			cout << "test " << test.get_attribute("ID") << " failed:" << endl
				 << "\t" << fs::system_complete(input) << endl
				 << test.content() << endl
				 << endl
				 << test.get_attribute("SECTIONS") << endl
				 << endl
				 << "exception: " << e.what() << endl
				 << endl;
		}
	}
}

void run_test_case(xml::node& testcase, const string& id, fs::path base_dir)
{
	if (VERBOSE)
		cout << "Running testcase " << testcase.get_attribute("PROFILE") << endl;

	if (not testcase.get_attribute("xml:base").empty())
	{
		base_dir /= testcase.get_attribute("xml:base");
		fs::current_path(base_dir);
	}
	
	foreach (xml::node& testcasenode, testcase.children())
	{
		if (testcasenode.name() == "TEST")
		{
			if (id.empty() or id == testcasenode.get_attribute("ID"))
				run_test(testcasenode, base_dir);
		}
		else if (testcasenode.name() == "TESTCASE" or testcasenode.name() == "TESTCASES")
			run_test_case(testcasenode, id, base_dir);
		else
			throw zeep::exception("invalid testcases file: unknown node %s",
				testcasenode.name().c_str());
	}
}

void test_testcases(const fs::path& testFile, const string& id)
{
	fs::ifstream file(testFile);
	
	int saved_verbose = VERBOSE;
//	VERBOSE = 0;
	
	fs::path base_dir = fs::system_complete(testFile.branch_path());
	fs::current_path(base_dir);

	xml::document doc(file);

	VERBOSE = saved_verbose;
	
	xml::node_ptr root = doc.root();
	if (root->name() != "TESTSUITE")
		throw zeep::exception("Invalid test case file");

	cout << "Running testsuite: " << root->get_attribute("PROFILE") << endl;

	foreach (xml::node& test, root->children())
	{
		run_test_case(test, id, base_dir);
	}
}

int main(int argc, char* argv[])
{
	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help", "produce help message")
	    ("verbose", "verbose output")
//	    ("input-file", "input file")
		("id", po::value<string>(), "ID for the test to run from the test suite")
	    ("test", "Run SUN test suite")
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
	
	try
	{
		string id;
		if (vm.count("id"))
			id = vm["id"].as<string>();
		
		if (vm.count("test"))
		{
			fs::path xmlTestFile(vm["test"].as<string>());
			test_testcases(xmlTestFile, id);
			
			cout << endl
				 << "summary: " << endl
				 << "  ran " << total_tests << " tests" << endl
				 << "  " << error_tests << " threw an exception" << endl
				 << "  " << failed_tests << " failed" << endl
				 << "  " << dubious_tests << " had a dubious output" << endl;
		}
//		else if (vm.count("input-file"))
//		{
//			fs::path p(vm["input-file"].as<string>());
//			
//			if (fs::is_directory(p))
//			{
//				fs::current_path(p);
//
//				fs::directory_iterator end_itr; // default construction yields past-the-end
//				for (fs::directory_iterator itr("."); itr != end_itr; ++itr)
//				{
//					if (ba::ends_with(itr->path().filename(), ".xml"))
//						test(fs::system_complete(itr->path()));
//				}
//			}
//			else
//				test(p);
//		}
	}
	catch (std::exception& e)
	{
		cout << e.what() << endl;
		return 1;
	}
	
	return 0;
}
