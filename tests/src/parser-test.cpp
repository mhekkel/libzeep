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
#include "zeep/xml/writer.hpp"
#include "zeep/xml/xpath.hpp"
#include "zeep/xml/unicode_support.hpp"

using namespace std;
using namespace zeep;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

#define foreach BOOST_FOREACH

int VERBOSE;
int TRACE;
int dubious_tests, error_tests, should_have_failed, total_tests, wrong_exception, skipped_tests;

bool run_valid_test(istream& is, fs::path& outfile)
{
	bool result = true;
	
	xml::document indoc;
	is >> indoc;
	
	stringstream s;

	xml::writer w(s);
	w.set_xml_decl(false);
	w.set_indent(0);
	w.set_wrap(false);
	w.set_collapse_empty_elements(false);
	w.set_escape_whitespace(true);
	w.set_no_comment(true);
	indoc.write(w);

	string s1 = s.str();
	ba::trim(s1);

	if (TRACE)
		cout << s1 << endl;
	
	if (fs::is_directory(outfile))
		;
	else if (fs::exists(outfile))
	{
		fs::ifstream out(outfile, ios::binary);
		string s2, line;
		while (not out.eof())
		{
			getline(out, line);
			s2 += line;
		}
		ba::trim(s2);

		if (s1 != s2)
		{
			xml::document a;
			a.set_validating(false);
			a.read(s1);

			xml::document b;
			b.set_validating(false);
			b.read(s2);
			
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

void dump(xml::element& e, int level = 0)
{
	cout << level << "> " << e.qname() << endl;
	for (xml::element::attribute_iterator a = e.attr_begin(); a != e.attr_end(); ++a)
		cout << level << " (a)> " << a->qname() << endl;
	foreach (xml::element& c, e)
		dump(c, level + 1);
}

bool run_test(const xml::element& test, fs::path base_dir)
{
	bool result = true;

	fs::path input(base_dir / test.get_attribute("URI"));
	fs::path output(base_dir / test.get_attribute("OUTPUT"));

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

	fs::ifstream is(input, ios::binary);
	if (not is.is_open())
		throw zeep::exception("test file not open");
	
	string error;
	
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
				xml::document doc;
				doc.set_validating(test.get_attribute("TYPE") == "invalid");
				doc.read(is);
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
				xml::document doc;
				doc.read(is);
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
		error = e.what();
	}

	if (VERBOSE or result == false)
	{
		cout << "-----------------------------------------------" << endl
			 << "ID:      " << test.get_attribute("ID") << endl
			 << "TYPE:    " << test.get_attribute("TYPE") << endl
			 << "FILE:    " << fs::system_complete(input) << endl
			 << "SECTION: " << test.get_attribute("SECTIONS") << endl
			 << "TYPE:    " << test.get_attribute("TYPE") << endl
			 << test.content() << endl
			 << endl;

		if (result == false)
		{
			cout << "exception: " << error << endl
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
	fs::ifstream file(testFile, ios::binary);
	
	int saved_verbose = VERBOSE;
	VERBOSE = 0;
	
	int saved_trace = TRACE;
	TRACE = 0;
	
	fs::path base_dir = fs::system_complete(testFile.branch_path());
	fs::current_path(base_dir);

	xml::document doc;
	doc.set_validating(false);
	doc.read(file);

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
#if SOAP_XML_HAS_EXPAT_SUPPORT
	    ("expat", "Use expat parser")
#endif
	    ("trace", "Trace productions in parser")
	    ("type", po::value<string>(), "Type of test to run (valid|not-wf|invalid|error)")
	    ("single", po::value<string>(), "Test a single XML file")
	    ("dump", po::value<string>(), "Dump the structure of a single XML file")
	    ("print-ids", "Print the ID's of failed tests")
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

	fs::path savedwd = fs::current_path();
	
	try
	{
#if SOAP_XML_HAS_EXPAT_SUPPORT
		if (vm.count("expat"))
			xml::document::set_parser_type(xml::parser_expat);
#endif
		
		if (vm.count("single"))
		{
			fs::path path(vm["single"].as<string>());

			fs::ifstream file(path, ios::binary);
			if (not file.is_open())
				throw zeep::exception("could not open file");

			fs::path dir(path.branch_path());
			fs::current_path(dir);

			run_valid_test(file, dir);
		}
		else if (vm.count("dump"))
		{
			fs::path path(vm["dump"].as<string>());

			fs::ifstream file(path, ios::binary);
			if (not file.is_open())
				throw zeep::exception("could not open file");

			fs::path dir(path.branch_path());
			fs::current_path(dir);

			xml::document doc;
			file >> doc;
			dump(*doc.child());
		}
		else
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
				 << "  ran " << total_tests - skipped_tests << " out of " << total_tests << " tests" << endl
				 << "  " << error_tests << " threw an exception" << endl
				 << "  " << wrong_exception << " wrong exception" << endl
				 << "  " << should_have_failed << " should have failed but didn't" << endl
				 << "  " << dubious_tests << " had a dubious output" << endl;

			if (vm.count("print-ids"))
			{
				cout << endl
					 << "ID's for the failed tests: " << endl;
				
				copy(failed_ids.begin(), failed_ids.end(), ostream_iterator<string>(cout, "\n"));
				
				cout << endl;
			}
		}
	}
	catch (std::exception& e)
	{
		cout << e.what() << endl;
		return 1;
	}
	
	fs::current_path(savedwd);
	
	return 0;
}
