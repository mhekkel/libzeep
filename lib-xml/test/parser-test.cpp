#include <iostream>

#if defined(_WIN32)
#include <conio.h>
#include <ctype.h>
#endif

#include <string>
#include <list>
#include <regex>
#include <fstream>
#include <filesystem>
#include <set>

#include <mcfp/mcfp.hpp>

#include <zeep/xml/document.hpp>
#include <zeep/exception.hpp>
#include <zeep/xml/parser.hpp>
// #include <zeep/xml/writer.hpp>
#include <zeep/xml/xpath.hpp>
#include <zeep/xml/character-classification.hpp>

using namespace std;
using namespace zeep;

namespace fs = std::filesystem;

int VERBOSE;
int TRACE;
int error_tests, should_have_failed, total_tests, wrong_exception, skipped_tests;

bool run_valid_test(istream& is, fs::path& outfile)
{
	bool result = true;
	
	xml::document indoc;
	is >> indoc;
	
	stringstream s;
	indoc.set_collapse_empty_tags(false);
	indoc.set_suppress_comments(true);
	indoc.set_escape_white_space(true);
	indoc.set_wrap_prolog(false);
	s << indoc;

	string s1 = s.str();
	trim(s1);

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
		trim(s2);

		if (s1 != s2)
		{
			stringstream ss;
			ss	 << "output differs: " << endl
				 << endl
				 << s1 << endl
				 << endl
				 << s2 << endl
				 << endl;

			throw zeep::exception(ss.str());
		}
	}
	else
		cout << "skipped output compare for " << outfile << endl;
	
	return result;
}

void dump(xml::element& e, int level = 0)
{
	cout << level << "> " << e.get_qname() << endl;
	for (auto& [name, ign]: e.attributes())
		cout << level << " (a)> " << name << endl;
	for (auto& c: e)
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
		
		if (test.get_attribute("TYPE") == "valid")
			result = run_valid_test(is, output);
		else if (test.get_attribute("TYPE") == "not-wf" or test.get_attribute("TYPE") == "invalid")
		{
			bool failed = false;
			try
			{
				xml::document doc;
				doc.set_validating(test.get_attribute("TYPE") == "invalid");
				doc.set_validating_ns(test.get_attribute("RECOMMENDATION") == "NS1.0");
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
				if (VERBOSE > 1)
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
				if (VERBOSE > 1)
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
				if (VERBOSE > 1)
					cout << e.what() << endl;
				
				failed = true;
			}

			if (VERBOSE and not failed)
			{
				if (test.get_attribute("TYPE") == "not-wf")
					throw zeep::exception("document should have been not well formed");
				else // or test.attr("TYPE") == "error" 
					throw zeep::exception("document should have been invalid");
			}
		}
	}
	catch (std::exception& e)
	{
		if (test.get_attribute("TYPE") == "valid")
			++error_tests;
		result = false;
		error = e.what();
	}

	if ((result == false and VERBOSE == 1) or (VERBOSE > 1))
	{
		cout << "-----------------------------------------------" << endl
			 << "ID:             " << test.get_attribute("ID") << endl
			 << "FILE:           " << /*fs::system_complete*/(input) << endl
			 << "TYPE:           " << test.get_attribute("TYPE") << endl
			 << "SECTION:        " << test.get_attribute("SECTIONS") << endl
			 << "EDITION:        " << test.get_attribute("EDITION") << endl
			 << "RECOMMENDATION: " << test.get_attribute("RECOMMENDATION") << endl;
		
		istringstream s(test.get_content());
		for (;;)
		{
			string line;
			getline(s, line);
			
			trim(line);
			
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
			istringstream iss(error);
			for (;;)
			{
				string line;
				getline(iss, line);
				
				trim(line);
				
				if (line.empty() and iss.eof())
					break;
				
				cout << "  " << line << endl;
			}
			
			cout << endl;
			
			
//			cout << "exception: " << error << endl;
		}
	}
	
	return result;
}

void run_test_case(const xml::element& testcase, const string& id, const set<string>& skip,
	const string& type, int edition, fs::path base_dir, vector<string>& failed_ids)
{
	if (VERBOSE > 1 and id.empty())
		cout << "Running testcase " << testcase.get_attribute("PROFILE") << endl;

	if (not testcase.get_attribute("xml:base").empty())
	{
		base_dir /= testcase.get_attribute("xml:base");

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
		auto testID = n->get_attribute("ID");
		if (skip.count(testID))
			continue;

		if (not id.empty() and testID != id)
			continue;

		if (not type.empty() and type != n->get_attribute("TYPE"))
			continue;

		if (edition != 0)
		{
			auto es = n->get_attribute("EDITION");
			if (not es.empty())
			{
				auto b = sregex_token_iterator(es.begin(), es.end(), ws_re, -1);
				auto e = sregex_token_iterator();
				auto ei = find_if(b, e, [edition](const string& e) { return stoi(e) == edition; });

				if (ei == e)
					continue;
			}
		}

		if (fs::exists(base_dir / n->get_attribute("URI")) and
			not run_test(*n, base_dir))
		{
			failed_ids.push_back(n->get_attribute("ID"));
		}
	}
}

void test_testcases(const fs::path& testFile, const string& id, const set<string>& skip,
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
		if (test->get_qname() != "TESTCASES")
			continue;
		run_test_case(*test, id, skip, type, edition, base_dir, failed_ids);
	}
}

int main(int argc, char* argv[])
{
	int result = 0;

	auto &config = mcfp::config::instance();

	config.init(
		"usage: parser-test [options]",
		mcfp::make_option("help,h", "produce help message"),
		mcfp::make_option("verbose,v", "verbose output"),
		mcfp::make_option<string>("id", "ID for the test to run from the test suite"),
		mcfp::make_option<vector<string>>("skip", "Skip this test, can be specified multiple times"),
		mcfp::make_option<vector<string>>("questionable", "Questionable tests, do not consider failure of these to be an error"),
		mcfp::make_option<int>("edition", "XML 1.0 specification edition to test, default is 5, 0 which means run all tests"),
		mcfp::make_option("trace", "Trace productions in parser"),
		mcfp::make_option<string>("type", "Type of test to run (valid|not-wf|invalid|error)"),
		mcfp::make_option<string>("single", "Test a single XML file"),
		mcfp::make_option<string>("dump", "Dump the structure of a single XML file"),
		mcfp::make_option("print-ids", "Print the ID's of failed tests"),
		mcfp::make_option<string>("conf", "Configuration file")
	);

	std::error_code ec;
	config.parse(argc, argv, ec);
	if (ec)
	{
		std::clog << "error parsing arguments: " << ec.message() << '\n';
		exit(1);
	}

	if (config.count("help"))
	{
		cout << config << endl;
		return 1;
	}
	
	VERBOSE = config.count("verbose");
	TRACE = config.count("trace");

	fs::path savedwd = fs::current_path();
	
	try
	{
		if (config.count("single"))
		{
			fs::path path(config.get<string>("single"));

			std::ifstream file(path, ios::binary);
			if (not file.is_open())
				throw zeep::exception("could not open file");

			fs::path dir(path.parent_path());
			fs::current_path(dir);

			run_valid_test(file, dir);
		}
		else if (config.count("dump"))
		{
			fs::path path(config.get<string>("dump"));

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
			if (config.operands().size() == 1)
				xmlconfFile = config.operands().front();
			
			if (not fs::exists(xmlconfFile))
				throw std::runtime_error("Config file not found: " + xmlconfFile.string());
			
			string id;
			if (config.has("id"))
				id = config.get<string>("id");
			
			vector<string> skip;
			if (config.has("skip"))
				skip = config.get<vector<string>>("skip");

			string type;
			if (config.count("type"))
				type = config.get<string>("type");

			int edition = 5;
			if (config.count("edition"))
				edition = config.get<int>("edition");
			
			vector<string> failed_ids;
			
			test_testcases(xmlconfFile, id, {skip.begin(), skip.end()}, type, edition, failed_ids);
			
			cout << endl
				 << "summary: " << endl
				 << "  ran " << total_tests - skipped_tests << " out of " << total_tests << " tests" << endl
				 << "  " << error_tests << " threw an exception" << endl
				 << "  " << wrong_exception << " wrong exception" << endl
				 << "  " << should_have_failed << " should have failed but didn't" << endl;

			vector<string> questionable;
			if (config.count("questionable"))
				questionable = config.get<vector<string>>("questionable");
			
			set<string> erronous;
			for (auto fid: failed_ids)
			{
				if (std::find(questionable.begin(), questionable.end(), fid) == questionable.end())
					erronous.insert(fid);
			}
			
			if (not erronous.empty())
				result = 1;

			if (config.count("print-ids") and not failed_ids.empty())
			{
				cout << endl;
				if (erronous.empty())
					cout << "All the failed tests were questionable" << endl;
				else
				{
					cout << endl
						<< "ID's for the failed, non-questionable tests: " << endl;

					copy(erronous.begin(), erronous.end(), ostream_iterator<string>(cout, "\n"));
					
					cout << endl;
				}
			}
		}
	}
	catch (std::exception& e)
	{
		cout << e.what() << endl;
		return 1;
	}
	
	fs::current_path(savedwd);

// #if defined(_WIN32)
// 	cout << "press any key to continue...";
// 	char ch = _getch();
// #endif

	return result;
}
