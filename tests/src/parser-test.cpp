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

#define DOC		xml::document
//#define DOC		xml::expat_doc
//#define DOC		xml::libxml2_doc

#define foreach BOOST_FOREACH

int VERBOSE;
int TRACE;
int dubious_tests, error_tests, should_have_failed, total_tests, wrong_exception, skipped_tests;

bool run_valid_test(istream& is, fs::path& outfile)
{
	bool result = true;
	
	DOC indoc;
	is >> indoc;
	
	stringstream s;

	xml::writer w(s);
	w.set_xml_decl(false);
	w.set_indent(0);
	w.set_wrap(false);
	w.set_collapse_empty_elements(false);
	w.set_escape_whitespace(true);
	indoc.write(w);

	string s1 = s.str();
	ba::trim(s1);

	if (TRACE)
		cout << s1 << endl;
	
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
			cout << "output differs" << endl
				 << "generated:      " << s1 << endl
				 << "expected:       " << s2 << endl
				 << endl;
			
			xml::document a; a.set_validating(false); a.read(s1);
			xml::document b; b.set_validating(false); b.read(s2);
			
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

bool run_test(const xml::element& test, fs::path base_dir)
{
	bool result = true;

	fs::path input(base_dir / test.get_attribute("URI"));
	fs::path output(base_dir / test.get_attribute("OUTPUT"));

	if (VERBOSE)
	{
		cout << "-----------------------------------------------" << endl
			 << "ID: " << test.get_attribute("ID") << endl
			 << "TYPE: " << test.get_attribute("TYPE") << endl
			 << "FILE: " << fs::system_complete(input) << endl
			 << test.content() << endl
			 << endl;
	}
	
	++total_tests;
	
	if (not fs::exists(input))
	{
		cout << "test file " << input << " does not exist" << endl;
		return false;
	}
	
	if (test.get_attribute("SECTIONS") == "B.")
	{
		if (VERBOSE)
			cout << "skipping unicode character validation tests" << endl;
		++skipped_tests;
		return true;
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
				DOC doc;
				doc.set_validating(test.get_attribute("TYPE") == "invalid");
				is >> doc;
				++should_have_failed;
				result = false;
				
			}
			catch (zeep::xml::not_wf_exception& e)
			{
				if (test.get_attribute("TYPE") != "not-wf")
				{
					++wrong_exception;
					throw zeep::exception(string("Wrong exception (should have been invalid):\n\t") + e.what());
				}

				failed = true;
				if (VERBOSE)
					cout << e.what() << endl;
			}
			catch (zeep::xml::invalid_exception& e)
			{
				if (test.get_attribute("TYPE") != "invalid")
				{
					++wrong_exception;
					throw zeep::exception(string("Wrong exception (should have been not-wf):\n\t") + e.what());
				}

				failed = true;
				if (VERBOSE)
					cout << e.what() << endl;
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
				DOC doc;
				is >> doc;
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
				 << test.content() << endl
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

void run_test_case(const xml::element* testcase, const string& id,
	const string& type, fs::path base_dir, vector<string>& failed_ids)
{
	if (VERBOSE and id.empty())
		cout << "Running testcase " << testcase->get_attribute("PROFILE") << endl;

	if (not testcase->get_attribute("xml:base").empty())
	{
		base_dir /= testcase->get_attribute("xml:base");
		fs::current_path(base_dir);
	}
	
	string path;
	if (id.empty())
		path = ".//TEST";
	else
		path = string(".//TEST[@ID='") + id + "']";
	
	foreach (const xml::element* n, xml::xpath(path).evaluate<xml::element>(*testcase))
	{
		if ((id.empty() or id == n->get_attribute("ID")) and
			(type.empty() or type == n->get_attribute("TYPE")))
		{
			if (fs::exists(base_dir / n->get_attribute("URI")) and
				not run_test(*n, base_dir))
			{
				failed_ids.push_back(n->get_attribute("ID"));
			}
		}
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

	xml::document doc;
	doc.set_validating(false);
	file >> doc;

	VERBOSE = saved_verbose;
	TRACE = saved_trace;
	
	foreach (const xml::element* test, doc.find("//TESTCASES"))
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
		("id", po::value<string>(), "ID for the test to run from the test suite")
	    ("test", "Run SUN test suite")
	    ("trace", "Trace productions in parser")
	    ("type", po::value<string>(), "Type of test to run (valid|not-wf|invalid|error)")
	    ("single", po::value<string>(), "Test a single XML file")
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
		if (vm.count("single"))
		{
			fs::path path(vm["single"].as<string>());

			fs::path dir(path.branch_path());
			fs::current_path(dir);

			fs::ifstream file(path);

			run_valid_test(file, dir);
			return 0;
		}
		
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
			 << "  ran " << total_tests - skipped_tests << " out of " << total_tests << " tests" << endl
			 << "  " << error_tests << " threw an exception" << endl
			 << "  " << wrong_exception << " wrong exception" << endl
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
