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
//#include "zeep/xml/expat_doc.hpp"
//#include "zeep/xml/libxml2_doc.hpp"

using namespace std;
using namespace zeep;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

int VERBOSE;

void run_valid_test(istream& is, const string& outfile)
{
	xml::document indoc(is);
	
	stringstream s;
	s << *indoc.root();
	string s1 = s.str();
	ba::trim(s1);

	if (not outfile.empty() and fs::exists(outfile))
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
			throw zeep::exception("output differs:\n%s\n%s\n",
				s1.c_str(), s2.c_str());
		}
	}
}


void run_test(xml::node& test, fs::path& base_dir)
{
	fs::path input(base_dir / test.get_attribute("URI"));

	if (not fs::exists(input))
		throw zeep::exception("test file %s does not exist", input.string().c_str());

	fs::ifstream is(input);
	if (not is.is_open())
		throw zeep::exception("test file not open");
	
	try
	{
		fs::current_path(input.branch_path());
		
		if (test.get_attribute("TYPE") == "valid")
			run_valid_test(is, test.get_attribute("OUTPUT"));
		else if (test.get_attribute("TYPE") == "not-wf")
		{
			try
			{
				xml::document doc(is);
				throw zeep::exception("invalid document, should have failed");
			}
			catch (...) {}
		}
	}
	catch (std::exception& e)
	{
		cout << "test " << test.get_attribute("ID") << " failed:" << endl
			 << "\t" << fs::system_complete(input) << endl
			 << test.content() << endl
			 << endl
			 << "exception: " << e.what() << endl
			 << endl;
	}
}

void run_test_case(xml::node& testcase, const string& id, fs::path& base_dir)
{
	foreach (xml::node& testcasenode, testcase.children())
	{
		if (testcasenode.name() == "TEST")
		{
			if (id.empty() or id == testcasenode.get_attribute("ID"))
				run_test(testcasenode, base_dir);
		}
		else if (testcasenode.name() == "TESTCASE")
			run_test_case(testcasenode, id, base_dir);
		else
			throw zeep::exception("invalid testcases file: unknown node %s",
				testcasenode.name().c_str());
	}
}

//void run_test_set(const fs::path& p)
//{
//	if (VERBOSE)
//		cout << "+++ PROCESSING " << p << " +++" << endl;
//	
//	fs::path path = fs::system_complete(p);
//	
//	try
//	{
//		fs::ifstream file1(path);
//		xml::document doc1(file1);
//		
//		try
//		{
//			fs::ifstream file2(path);
//			xml::expat_doc doc2(file2);
//		
//			if (doc1.root() and doc2.root() and not (*doc1.root() == *doc2.root()))
//			{
//				cout << "zeep and expat disagree on document " << p << endl
//					 << *doc1.root() << endl << *doc2.root() << endl;
//			}
//		}
//		catch (exception& e)
//		{
//			cout << "Expat failed to parse document " << p << endl
//				 << "  Exception: " << e.what() << endl;
//		}
//	
////		try
////		{
////			fs::ifstream file3(p);
////			xml::libxml2_doc doc3(file3);
////			
////			if (doc1.root() and doc3.root() and not (*doc1.root() == *doc3.root()))
////			{
////				cout << "zeep and libxml2 disagree on document " << p << endl
////					 << doc1 << endl << endl << doc3 << endl;
////			}
////		}
////		catch (exception& e)
////		{
////			cout << "libxml2 failed to parse document " << p << endl
////				 << "  Exception: " << e.what() << endl;
////		}
//	}
//	catch (exception& e)
//	{
//		cout << "Error while processing " << p << endl
//			 << "exception: " << e.what() << endl;
////		throw;
//	}
//}

void test_testcases(const fs::path& testFile, const string& id)
{
	fs::ifstream file(testFile);
	
	int saved_verbose = VERBOSE;
	VERBOSE = 0;
	
	xml::document doc(file);
	
	VERBOSE = saved_verbose;
	
	fs::path base_dir = fs::system_complete(testFile.branch_path());
	
	xml::node_ptr root = doc.root();
	if (root->name() == "TESTCASES")
		run_test_case(*root, id, base_dir);
	else if (root->name() == "TESTSUITE")
	{
		foreach (xml::node& test, root->children())
		{
			run_test_case(test, id, base_dir);
		}
	}
	else
		throw zeep::exception("Invalid test case file");
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
