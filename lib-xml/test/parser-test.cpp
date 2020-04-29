#include <iostream>

#if defined(_MSC_VER)
#include <conio.h>
#include <ctype.h>
#endif

#include <string>
#include <list>
#include <regex>
#include <fstream>
#include <filesystem>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <zeep/xml/document.hpp>
#include <zeep/exception.hpp>
#include <zeep/xml/parser.hpp>
// #include <zeep/xml/writer.hpp>
#include <zeep/xml/xpath.hpp>
#include <zeep/xml/character-classification.hpp>

using namespace std;
using namespace zeep;

namespace po = boost::program_options;
namespace fs = std::filesystem;
namespace ba = boost::algorithm;

int VERBOSE;
int TRACE;
int error_tests, should_have_failed, total_tests, wrong_exception, skipped_tests;

bool run_valid_test(istream& is, fs::path& outfile)
{
	bool result = true;
	
	xml::document indoc;
	is >> indoc;
	
	stringstream s;
	indoc.collapse_empty_tags(false);
	indoc.suppress_comments(true);
	indoc.escape_white_space(true);
	indoc.wrap_prolog(false);
	s << indoc;

	string s1 = s.str();
	ba::trim(s1);

	if (TRACE)
		cout << s1 << endl;
	
	if (fs::is_directory(outfile))
		;
	else if (fs::exists(outfile))
	{
		std::ifstream out(outfile, ios::binary);
		string s2, line;
		while (not out.eof())
		{
			getline(out, line);
			s2 += line + "\n";
		}
		ba::trim(s2);

		if (s1 != s2)
		{
			stringstream s;
			s	 << "output differs: " << endl
				 << endl
				 << s1 << endl
				 << endl
				 << s2 << endl
				 << endl;

			throw zeep::exception(s.str());
		}
	}
	else
		cout << "skipped output compare for " << outfile << endl;
	
	return result;
}

void dump(xml::element& e, int level = 0)
{
	cout << level << "> " << e.qname() << endl;
	for (auto& [name, ignore]: e.attributes())
		cout << level << " (a)> " << name << endl;
	for (auto& c: e)
		dump(c, level + 1);
}

bool run_test(const xml::element& test, fs::path base_dir)
{
	bool result = true;

	fs::path input(base_dir / test.attr("URI"));
	fs::path output(base_dir / test.attr("OUTPUT"));

	++total_tests;
	
	if (not fs::exists(input))
	{
		cout << "test file " << input << " does not exist" << endl;
		return false;
	}
	
	// if (test.attr("SECTIONS") == "B.")
	// {
	// 	if (VERBOSE)
	// 		cout << "skipping unicode character validation tests" << endl;
	// 	++skipped_tests;
	// 	return true;
	// }
	
	fs::current_path(input.parent_path());

	std::ifstream is(input, ios::binary);
	if (not is.is_open())
		throw zeep::exception("test file not open");
	
	string error;
	
	try
	{
		fs::current_path(input.parent_path());
		
		if (test.attr("TYPE") == "valid")
			result = run_valid_test(is, output);
		else if (test.attr("TYPE") == "not-wf" or test.attr("TYPE") == "invalid")
		{
			bool failed = false;
			try
			{
				xml::document doc;
				doc.validating(test.attr("TYPE") == "invalid");
				is >> doc;
				++should_have_failed;
				result = false;
				
			}
			catch (zeep::xml::not_wf_exception& e)
			{
				if (test.attr("TYPE") != "not-wf")
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
				if (test.attr("TYPE") != "invalid")
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
		else
		{
			bool failed = false;
			try
			{
				xml::document doc;
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
			{
				if (test.attr("TYPE") == "not-wf")
					throw zeep::exception("document should have been not well formed");
				else // or test.attr("TYPE") == "error" 
					throw zeep::exception("document should have been invalid");
			}
		}
	}
	catch (std::exception& e)
	{
		if (test.attr("TYPE") == "valid")
			++error_tests;
		result = false;
		error = e.what();
	}

	if (VERBOSE or result == false)
	{
		cout << "-----------------------------------------------" << endl
			 << "ID:             " << test.attr("ID") << endl
			 << "FILE:           " << /*fs::system_complete*/(input) << endl
			 << "TYPE:           " << test.attr("TYPE") << endl
			 << "SECTION:        " << test.attr("SECTIONS") << endl
			 << "EDITION:        " << test.attr("EDITION") << endl
			 << "RECOMMENDATION: " << test.attr("RECOMMENDATION") << endl;
		
		istringstream s(test.content());
		for (;;)
		{
			string line;
			getline(s, line);
			
			ba::trim(line);
			
			if (line.empty())
			{
				if (s.eof())
					break;
				continue;
			}
			
			cout << "DESCR:          " << line << endl;
		}
		
		cout << endl;

		if (result == false)
		{
			istringstream s(error);
			for (;;)
			{
				string line;
				getline(s, line);
				
				ba::trim(line);
				
				if (line.empty() and s.eof())
					break;
				
				cout << "  " << line << endl;
			}
			
			cout << endl;
			
			
//			cout << "exception: " << error << endl;
		}
	}
	
	return result;
}

void run_test_case(const xml::element& testcase, const string& id,
	const string& type, int edition, fs::path base_dir, vector<string>& failed_ids)
{
	if (VERBOSE and id.empty())
		cout << "Running testcase " << testcase.attr("PROFILE") << endl;

	if (not testcase.attr("xml:base").empty())
	{
		base_dir /= testcase.attr("xml:base");

		if (fs::exists(base_dir))
			fs::current_path(base_dir);
	}
	
	string path;
	if (id.empty())
		path = ".//TEST";
	else
		path = string(".//TEST[@ID='") + id + "']";
	
	regex ws_re(" "); // whitespace

	for (const xml::element* n: xml::xpath(path).evaluate<xml::element>(testcase))
	{
		if ((id.empty() or id == n->attr("ID")) and
			(type.empty() or type == n->attr("TYPE")))
		{
			if (edition != 0)
			{
				auto es = n->attr("EDITION");
				if (not es.empty())
				{
					auto b = sregex_token_iterator(es.begin(), es.end(), ws_re, -1);
					auto e = sregex_token_iterator();
					auto ei = find_if(b, e, [edition](const string& e) { return stoi(e) == edition; });

					if (ei == e)
						continue;
				}
			}

			if (fs::exists(base_dir / n->attr("URI")) and
				not run_test(*n, base_dir))
			{
				failed_ids.push_back(n->attr("ID"));
			}
		}
	}
}

void test_testcases(const fs::path& testFile, const string& id,
	const string& type, int edition, vector<string>& failed_ids)
{
	std::ifstream file(testFile, ios::binary);
	
	int saved_verbose = VERBOSE;
	VERBOSE = 0;
	
	int saved_trace = TRACE;
	TRACE = 0;
	
	fs::path base_dir = fs::weakly_canonical(testFile.parent_path());
	fs::current_path(base_dir);

	xml::document doc(file);

	VERBOSE = saved_verbose;
	TRACE = saved_trace;
	
	for (auto test: doc.find("//TESTCASES"))
	{
		if (test->qname() != "TESTCASES")
			continue;
		run_test_case(*test, id, type, edition, base_dir, failed_ids);
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
		("edition", po::value<int>(), "XML 1.0 specification edition to test, default is 5, 0 which means run all tests")
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
		if (vm.count("single"))
		{
			fs::path path(vm["single"].as<string>());

			std::ifstream file(path, ios::binary);
			if (not file.is_open())
				throw zeep::exception("could not open file");

			fs::path dir(path.parent_path());
			fs::current_path(dir);

			run_valid_test(file, dir);
		}
		else if (vm.count("dump"))
		{
			fs::path path(vm["dump"].as<string>());

			std::ifstream file(path, ios::binary);
			if (not file.is_open())
				throw zeep::exception("could not open file");

			fs::path dir(path.parent_path());
			fs::current_path(dir);

			xml::document doc;
			file >> doc;
			dump(doc.front());
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

			int edition = 5;
			if (vm.count("edition"))
				edition = vm["edition"].as<int>();
			
			vector<string> failed_ids;
			
			test_testcases(xmlconfFile, id, type, edition, failed_ids);
			
			cout << endl
				 << "summary: " << endl
				 << "  ran " << total_tests - skipped_tests << " out of " << total_tests << " tests" << endl
				 << "  " << error_tests << " threw an exception" << endl
				 << "  " << wrong_exception << " wrong exception" << endl
				 << "  " << should_have_failed << " should have failed but didn't" << endl;

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

#if defined(_MSC_VER)
	cout << "press any key to continue...";
	char ch = _getch();
#endif

	return 0;
}
