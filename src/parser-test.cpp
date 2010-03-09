#include <iostream>

#include <string>
#include <list>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>

#include "zeep/xml/parser.hpp"

using namespace std;
using namespace zeep;
namespace po = boost::program_options;
namespace bf = boost::filesystem;

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
	
	if (vm.count("input-file"))
	{
		bf::ifstream file(vm["input-file"].as<string>());

		xml::parser parser(file);
//		parser.start_element_handler = start_element;
//		parser.end_element_handler = end_element;
//		parser.character_data_handler = character_data;

		try
		{
			parser.parse();
		}
		catch (exception& e)
		{
			cerr << "Exception: " << e.what() << endl;
			return 1;
		}
	}
	
	return 0;
}
