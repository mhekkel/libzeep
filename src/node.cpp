#include <boost/algorithm/string.hpp>

#include "xml/node.hpp"

using namespace std;
namespace ba = boost::algorithm;

namespace xml
{

void node::add_attribute(
	attribute_ptr		attr)
{
	if (not attributes_)
		attributes_ = attr;
	else
	{
		attribute_ptr after = attributes_;
		while (after->next_)
			after = after->next_;
		after->next_ = attr;
	}
}

void node::add_child(
	node_ptr			node)
{
	if (not children_)
		children_ = node;
	else
	{
		node_ptr after = children_;
		while (after->next_)
			after = after->next_;
		after->next_ = node;
	}
}

void node::add_content(
	const char*			text,
	unsigned long		length)
{
	content_.append(text, length);
}

void node::write(
	ostream&			stream,
	int					level) const
{
	for (int i = 0; i < level; ++i)
		stream << ' ';
	
	stream << '<';
	
	if (prefix_.length())
		stream << prefix_ << ':';
	
	stream << name_;

	if (attributes_)
	{
		for (const_attribute_iterator a = attribute_begin(); a != attribute_end(); ++a)
		{
//			stream << endl;
//
//			for (int i = 0; i < level; ++i)
//				stream << ' ';

			stream << ' ' << a->name() << "=\"" << a->value() << '"';
		}
	}

	string cont = content_;
	ba::trim(cont);
	
	ba::replace_all(cont, "&", "&amp;");
	ba::replace_all(cont, "<", "&lt;");
	ba::replace_all(cont, ">", "&gt;");

	if (cont.length() or children_)
		stream << '>';
	
	if (cont.length())
		stream << cont;

	if (children_)
	{
		stream << endl;

		for (const_iterator c = begin(); c != end(); ++c)
			c->write(stream, level + 1);

		for (int i = 0; i < level; ++i)
			stream << ' ';
	}

	if (cont.length() or children_)
	{
		stream << "</";
		
		if (prefix_.length())
			stream << prefix_ << ':';
		
		stream << name_ << ">" << endl;
	}	
	else
		stream << "/>" << endl;
}

ostream& operator<<(ostream& lhs, const node& rhs)
{
	rhs.write(lhs, 0);
	return lhs;
}

}
