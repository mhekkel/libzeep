#include <iostream>

#include <string>
#include <list>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "zeep/xml/parser.hpp"

using namespace std;
using namespace zeep;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

void start_element(const string& name, const xml::attribute_list& attrs)
{
	cout << '<' << name;
	
	foreach (const xml::attribute& attr, attrs)
	{
		cout << ' ' << attr.name() << "=\"" << attr.value() << '\"';
	}
	
	cout << '>';
}

void end_element(const string& name)
{
	cout << '<' << '/' << name;
	
	cout << '>' << endl;
}

void character_data(const string& data)
{
	cout << data;
}

void test(const fs::path& p)
{
	cout << endl << endl << "+++ PROCESSING " << p << " +++" << endl << endl;
	
	fs::ifstream file(p);

	xml::parser parser(file);
//		parser.start_element_handler = start_element;
//		parser.end_element_handler = end_element;
//		parser.character_data_handler = character_data;

	try
	{
		parser.parse();
	}
	catch (...)
	{
		cerr << "Error while processing " << p << endl;
		throw;
	}
}

int main(int argc, char* argv[])
{
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
