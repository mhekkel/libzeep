#include <iostream>

#include <string>
#include <list>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "zeep/xml/document.hpp"
#include "zeep/xml/ex_doc.hpp"

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
	
	try
	{
		fs::ifstream file1(p);
		xml::document doc1(file1);
		
		fs::ifstream file2(p);
		xml::ex_doc doc2(file2);
		
		if (not (*doc1.root() == *doc2.root()))
		{
			cout << doc1 << endl << endl << doc2 << endl;
			
			throw exception();
		}
	}
	catch (...)
	{
		cerr << "Error while processing " << p << endl;
//		throw;
	}
}

void test_2()
{
	xml::node a("a"), b("a");
	
	a.add_attribute("een", "1");
	a.add_attribute("twee", "2");

	b.add_attribute("een", "1");
	b.add_attribute("twee", "2");

	bool result = (a == b);
	if (not result)
		cerr << "oeps" << endl;
}

int main(int argc, char* argv[])
{
	test_2();
	
	
	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help", "produce help message")
	    ("verbose", "verbose output")
	    ("input-file", "input file")
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
		if (vm.count("input-file"))
		{
			fs::path p(vm["input-file"].as<string>());
			
			if (fs::is_directory(p))
			{
				fs::directory_iterator end_itr; // default construction yields past-the-end
				for (fs::directory_iterator itr(p); itr != end_itr; ++itr)
				{
					if (ba::ends_with(itr->path().filename(), ".xml"))
						test(itr->path());
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
