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
#include "zeep/xml/parser.hpp"
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
int TRACE;
int failed_tests, dubious_tests, error_tests, should_have_failed, total_tests;

bool run_valid_test(istream& is, fs::path& outfile)
{
	bool result = true;
	
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
//			if (VERBOSE)
//			{
//				cout << "output differs: " << endl
//					 << s1 << endl
//					 << s2 << endl
//					 << endl;
//			}
			
			xml::document a(s1);
			xml::document b(s2);
			
			if (a == b)
				++dubious_tests;
			else
			{
				stringstream s;
				s	 << "output differs: " << endl
					 << s1 << endl
					 << s2 << endl
					 << endl;

				throw zeep::exception(s.str());
			}
		}
	}
	else
		cout << "skipped output compare for " << outfile << endl;
	
	return result;
}

bool run_test(xml::node& test, fs::path base_dir)
{
	bool result = true;
	
	++total_tests;
	
	fs::path input(base_dir / test.get_attribute("URI"));
	fs::path output(base_dir / test.get_attribute("OUTPUT"));

	if (not fs::exists(input))
	{
		cout << "test file " << input << " does not exist" << endl;
		return false;
	}
	
	fs::current_path(input.branch_path());

	fs::ifstream is(input);
	if (not is.is_open())
		throw zeep::exception("test file not open");
	
	try
	{
		fs::current_path(input.branch_path());
		
		if (test.get_attribute("TYPE") == "valid")
			result = run_valid_test(is, output);
		else if (test.get_attribute("TYPE") == "not-wf" or test.get_attribute("TYPE") == "invalid")
		{
			bool failed = false;
			try
			{
				DOC doc(is);
				++should_have_failed;
				result = false;
				
			}
			catch (zeep::xml::not_wf_exception& e)
			{
				if (test.get_attribute("TYPE") == "not-wf")
					failed = true;
				else
					throw zeep::exception(string("Wrong exception (should have been invalid):\n\t") + e.what());
			}
			catch (zeep::xml::invalid_exception& e)
			{
				if (test.get_attribute("TYPE") == "invalid")
					failed = true;
				else
					throw zeep::exception(string("Wrong exception (should have been not-wf):\n\t") + e.what());
			}
			catch (std::exception& e)
			{
				throw zeep::exception(string("Wrong exception:\n\t") + e.what());
			}

			if (VERBOSE and not failed)
				throw zeep::exception("invalid document, should have failed");
		}
		else // if (test.get_attribute("TYPE") == "not-wf" or test.get_attribute("TYPE") == "error" )
		{
			bool failed = false;
			try
			{
				DOC doc(is);
				++should_have_failed;
				result = false;
			}
			catch (std::exception& e)
			{
				if (VERBOSE)
					cout << e.what() << endl;
				
				failed = true;
			}

			if (VERBOSE and not failed)
				throw zeep::exception("invalid document, should have failed");
		}
	}
	catch (std::exception& e)
	{
		if (test.get_attribute("TYPE") == "valid")
			++error_tests;
		result = false;
		
		if (VERBOSE)
		{
			cout << "=======================================================" << endl
				 << "test " << test.get_attribute("ID") << " failed:" << endl
				 << "\t" << fs::system_complete(input) << endl
				 << test.text() << endl
				 << endl
				 << test.get_attribute("SECTIONS") << endl
				 << test.get_attribute("TYPE") << endl
				 << endl
				 << "exception: " << e.what() << endl
				 << endl;
		}
	}
	
	return result;
}

void run_test_case(xml::node& testcase, const string& id,
	const string& type, fs::path base_dir, vector<string>& failed_ids)
{
//	if (VERBOSE)
//		cout << "Running testcase " << testcase.get_attribute("PROFILE") << endl;

	if (not testcase.get_attribute("xml:base").empty())
	{
		base_dir /= testcase.get_attribute("xml:base");
		fs::current_path(base_dir);
	}
	
	foreach (xml::node& testcasenode, testcase.children())
	{
		if (testcasenode.name() == "TEST")
		{
			if ((id.empty() or id == testcasenode.get_attribute("ID")) and
				(type.empty() or type == testcasenode.get_attribute("TYPE")))
			{
				if (not run_test(testcasenode, base_dir))
					failed_ids.push_back(testcasenode.get_attribute("ID"));
			}
		}
		else if (testcasenode.name() == "TESTCASE" or testcasenode.name() == "TESTCASES")
			run_test_case(testcasenode, id, type, base_dir, failed_ids);
		else
			throw zeep::exception("invalid testcases file: unknown node %s",
				testcasenode.name().c_str());
	}
}

void test_testcases(const fs::path& testFile, const string& id,
	const string& type, vector<string>& failed_ids)
{
	fs::ifstream file(testFile);
	
	int saved_verbose = VERBOSE;
	VERBOSE = 0;
	
	int saved_trace = TRACE;
	TRACE = 0;
	
	fs::path base_dir = fs::system_complete(testFile.branch_path());
	fs::current_path(base_dir);

	xml::document doc(file);

	VERBOSE = saved_verbose;
	TRACE = saved_trace;
	
	xml::node_ptr root = doc.root();
	if (root->name() != "TESTSUITE")
		throw zeep::exception("Invalid test case file");

	cout << "Running testsuite: " << root->get_attribute("PROFILE") << endl;

	foreach (xml::node& test, root->children())
	{
		run_test_case(test, id, type, base_dir, failed_ids);
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
	    ("trace", "Trace productions in parser")
	    ("type", po::value<string>(), "Type of test to run (valid|not-wf|invalid|error)")
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
		fs::path xmlconfFile("XML-Test-Suite/xmlconf/xmlconf.xml");
		if (vm.count("test"))
			xmlconfFile = vm["test"].as<string>();
		
		string id;
		if (vm.count("id"))
			id = vm["id"].as<string>();
		
		string type;
		if (vm.count("type"))
			type = vm["type"].as<string>();
		
		vector<string> failed_ids;
		
		test_testcases(xmlconfFile, id, type, failed_ids);
		
		cout << endl
			 << "summary: " << endl
			 << "  ran " << total_tests << " tests" << endl
			 << "  " << error_tests << " threw an exception" << endl
			 << "  " << failed_tests << " failed" << endl
			 << "  " << should_have_failed << " should have failed but didn't" << endl
			 << "  " << dubious_tests << " had a dubious output" << endl;
			
		if (id.empty())
		{
			cout << endl
				 << "ID's for the failed tests: " << endl;
			
			copy(failed_ids.begin(), failed_ids.end(), ostream_iterator<string>(cout, "\n"));
			
			cout << endl;
		}
	}
	catch (std::exception& e)
	{
		cout << e.what() << endl;
		return 1;
	}
	
	return 0;
}
