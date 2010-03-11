#include <iostream>

#include <string>
#include <list>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "zeep/xml/document.hpp"
#include "zeep/xml/expat_doc.hpp"
#include "zeep/xml/libxml2_doc.hpp"

using namespace std;
using namespace zeep;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

int VERBOSE;

void test(const fs::path& p)
{
	if (VERBOSE)
		cout << "+++ PROCESSING " << p << " +++" << endl;
	
	fs::path path = fs::system_complete(p);
	
	try
	{
		fs::ifstream file1(path);
		xml::document doc1(file1);
		
		try
		{
			fs::ifstream file2(path);
			xml::expat_doc doc2(file2);
		
			if (doc1.root() and doc2.root() and not (*doc1.root() == *doc2.root()))
			{
				cerr << "zeep and expat disagree on document " << p << endl
					 << *doc1.root() << endl << *doc2.root() << endl;
			}
		}
		catch (exception& e)
		{
			cerr << "Expat failed to parse document " << p << endl
				 << "  Exception: " << e.what() << endl;
		}
	
//		try
//		{
//			fs::ifstream file3(p);
//			xml::libxml2_doc doc3(file3);
//			
//			if (doc1.root() and doc3.root() and not (*doc1.root() == *doc3.root()))
//			{
//				cerr << "zeep and libxml2 disagree on document " << p << endl
//					 << doc1 << endl << endl << doc3 << endl;
//			}
//		}
//		catch (exception& e)
//		{
//			cerr << "libxml2 failed to parse document " << p << endl
//				 << "  Exception: " << e.what() << endl;
//		}
	}
	catch (exception& e)
	{
		cerr << "Error while processing " << p << endl
			 << "exception: " << e.what() << endl;
//		throw;
	}
}

void sun_test(const fs::path& testFile)
{
	fs::ifstream file(testFile);
	xml::document doc(file);
	
	fs::path base_dir = fs::system_complete(testFile.branch_path());
	
	fs::current_path(base_dir);
	
	xml::node_ptr root = doc.root();
	foreach (xml::node& test, root->children())
	{
		// only valid tests for now
		if (test.get_attribute("TYPE") != "valid")
			continue;
		
		fs::path input = base_dir / test.get_attribute("URI");

		if (not fs::exists(input))
			cerr << "input file does not exist: " << input << endl;

		fs::path output = base_dir / test.get_attribute("OUTPUT");

		if (not fs::exists(output))
			cerr << "output file does not exist: " << output << endl;
		
		cout << "test: " << test.get_attribute("ID")
			 << " section: " << test.get_attribute("SECTIONS")
			 << endl;

		fs::current_path(input.branch_path());
		
		try
		{
			fs::ifstream in(input);
			xml::document indoc(in);
			
			stringstream s;
			s << *indoc.root();
			string s1 = s.str();
			ba::trim(s1);
			
			fs::ifstream out(output);
			string s2, line;
			while (not out.eof())
			{
				getline(out, line);
				s2 += line;
			}
			ba::trim(s2);
			
			if (s1 != s2)
			{
				cout << "output different:" << endl
					 << test.content() << endl
					 << s1 << endl
					 << s2 << endl;
			}
		}
		catch (exception& e)
		{
			cerr << "test failed: " << e.what() << endl;
		}
	}
}

int main(int argc, char* argv[])
{
	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help", "produce help message")
	    ("verbose", "verbose output")
	    ("input-file", "input file")
	    ("test", po::value<string>(), "Run SUN test suite")
	;
	
	po::positional_options_description p;
	p.add("input-file", -1);

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
		if (vm.count("test"))
		{
			fs::path xmlTestFile(vm["test"].as<string>());
			
			sun_test(xmlTestFile);
		}
		else if (vm.count("input-file"))
		{
			fs::path p(vm["input-file"].as<string>());
			
			if (fs::is_directory(p))
			{
				fs::current_path(p);

				fs::directory_iterator end_itr; // default construction yields past-the-end
				for (fs::directory_iterator itr("."); itr != end_itr; ++itr)
				{
					if (ba::ends_with(itr->path().filename(), ".xml"))
						test(fs::system_complete(itr->path()));
				}
			}
			else
				test(p);
		}
	}
	catch (exception& e)
	{
		cerr << e.what() << endl;
		return 1;
	}
	
	return 0;
}
