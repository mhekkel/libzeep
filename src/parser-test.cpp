#include <iostream>

#include <string>
#include <list>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "zeep/xml/parser.hpp"

using namespace std;
using namespace zeep;

void start_element(const string& name, const xml::attribute_list& attrs)
{
	cout << '<' << name;
	
	foreach (const xml::attribute& attr, attrs)
	{
		cout << ' ' << attr.name() << '=' << attr.value();
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
	string xml_doc("<hello name=\"my_name\">hello, world!</hello>");
	
	xml::parser parser(xml_doc);
	parser.start_element_handler = start_element;
	parser.end_element_handler = end_element;
	parser.character_data_handler = character_data;
	
	parser.parse();
	
	return 0;
}
