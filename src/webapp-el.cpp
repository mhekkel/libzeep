// --------------------------------------------------------------------
//
// webapp::el is a set of classes used to implement an 'expression language'
// A script language used in the XHTML templates used by the zeep webapp.
//

#include <zeep/config.hpp>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/algorithm/string.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/http/webapp-el.hpp>
#include <zeep/xml/unicode_support.hpp>

using namespace std;
namespace ba = boost::algorithm;

namespace zeep { namespace http { namespace el {

object::object()
	: m_type(ot_undef)
{
}

object::object(const object& o)
	: m_type(o.m_type)
{
	switch (m_type)
	{
		case ot_string:		m_string = o.m_string; break;
		case ot_boolean:
		case ot_number:		m_number = o.m_number; break;
		case ot_struct:		m_fields = o.m_fields; break;
		case ot_array:		m_array = o.m_array;
		default:			break;
	}
}

object::object(const char* s)
	: m_type(ot_string)
	, m_string(s ? s : "")
{
}

object::object(const std::string& s)
	: m_type(ot_string)
	, m_string(s)
{
}

object::object(double n)
	: m_type(ot_number)
	, m_number(n)
{
}

object::object(int8 n)
	: m_type(ot_number)
	, m_number(n)
{
}

object::object(uint8 n)
	: m_type(ot_number)
	, m_number(n)
{
}

object::object(int16 n)
	: m_type(ot_number)
	, m_number(n)
{
}

object::object(uint16 n)
	: m_type(ot_number)
	, m_number(n)
{
}

object::object(int32 n)
	: m_type(ot_number)
	, m_number(n)
{
}

object::object(uint32 n)
	: m_type(ot_number)
	, m_number(n)
{
}

object::object(int64 n)
	: m_type(ot_number)
	, m_number(n)
{
}

object::object(uint64 n)
	: m_type(ot_number)
	, m_number(n)
{
}

object::object(bool b)
	: m_type(ot_boolean)
	, m_number(b)
{
}

object::object(const std::vector<object>& a)
	: m_type(ot_array)
	, m_array(a)
{
}

//object::object(object_type type)
//	: m_type(type)
//{
//}

bool object::empty() const
{
	bool result = false;
	switch (m_type)
	{
		case ot_string:		result = m_string.empty(); break;
		case ot_array:		result = m_array.empty(); break;
		case ot_undef:		result = true; break;
		default:			break;
	}
	return result;
}

bool object::undefined() const
{
	return m_type == ot_undef;
}

bool object::is_array() const
{
	return m_type == ot_array;
}

uint32 object::count() const
{
	return m_type == ot_array ? m_array.size() : 0;
}

bool object::is_number() const
{
	return m_type == ot_number;
}

object& object::operator=(const object& rhs)
{
	m_type = rhs.m_type;
	switch (m_type)
	{
		case ot_string:		m_string = rhs.m_string; break;
		case ot_boolean:
		case ot_number:		m_number = rhs.m_number; break;
		case ot_struct:		m_fields = rhs.m_fields; break;
		case ot_array:		m_array = rhs.m_array;
		default:			break;
	}
	return *this;
}

object& object::operator=(const std::string& rhs)
{
	try
	{
		m_number = boost::lexical_cast<double>(rhs);
		m_type = ot_number;
	}
	catch (...)
	{
		m_string = rhs;
		m_type = ot_string;
	}

	return *this;
}

object& object::operator=(double rhs)
{
	m_type = ot_number;
	m_number = rhs;
	return *this;
}

object& object::operator=(bool rhs)
{
	m_type = ot_boolean;
	m_number = rhs;
	return *this;
}

object::operator std::string() const
{
	std::string result;
	switch (m_type)
	{
		case ot_number:		result = boost::lexical_cast<std::string>(m_number); break;
		case ot_string:		result = m_string; break;
		case ot_boolean:	result = m_number ? "true" : "false"; break;
		default:			break;
	}
	return result;
}

object::operator double() const
{
	double result;
	switch (m_type)
	{
		case ot_boolean:
		case ot_number:		result = m_number; break;
		case ot_string:		result = boost::lexical_cast<double>(m_string); break;
		default:			throw zeep::exception("object is not a number");
	}
	return result;
}

object::operator bool() const
{
	bool result;
	switch (m_type)
	{
		case ot_boolean:
		case ot_number:		result = m_number != 0; break;
		case ot_string:		result = not m_string.empty(); break;
		case ot_array:		result = not m_array.empty(); break;
		case ot_struct:		result = not m_fields.empty(); break;
		default:			result = false; break;
	}
	return result;
}

const object object::operator[](
	const std::string& name) const
{
	std::map<std::string,object>::const_iterator i = m_fields.find(name);
	if (i == m_fields.end())
		throw zeep::exception((boost::format("field %1% not found") % name).str());
	return i->second;
}

const object object::operator[](
	const char* name) const
{
	return operator[](std::string(name));
}

const object object::operator[](uint32 ix) const
{
	return m_array[ix];
}

object& object::operator[](
	const std::string& name)
{
	m_type = ot_struct;
	return m_fields[name];
}

object& object::operator[](
	const char* name)
{
	return operator[](std::string(name));
}

object& object::operator[](uint32 ix)
{
	return m_array[ix];
}

struct compare_object
{
	compare_object(const std::string& field, bool descending)
		: m_field(field)
		, m_descending(descending)
	{
	}
	
	bool		operator()(const object& a, const object& b) const;

	std::string		m_field;
	bool		m_descending;
};

void object::sort(const std::string& sort_field, bool descending)
{
	if (m_type == ot_array)
		std::sort(m_array.begin(), m_array.end(), compare_object(sort_field, descending));
}

ostream& operator<<(ostream& os, const object& o)
{
	switch (o.m_type)
	{
		case object::ot_undef:		os << "undef"; break;
		case object::ot_number:		os << "number(" << o.m_number << ")"; break;
		case object::ot_string:		os << "string(" << o.m_string << ")"; break;
		case object::ot_struct:
			os << "struct(";
			for (std::map<std::string,object>::const_iterator fi = o.m_fields.begin(); fi != o.m_fields.end(); ++fi)
				os << fi->first << ':' << fi->second << ", ";
			os << ")";
			break;
		case object::ot_array:
			os << "array[";
			BOOST_FOREACH (const object& f, o.m_array)
				os << f << ", ";
			os << "]";
			break;
		case object::ot_boolean:	os << "bool(" << (o.m_number ? "true)" : "false)"); break;
	}
	
	return os;
}

object operator<(const object& a, const object& b)
{
//cerr << endl << __func__ << " a = " << a << " b = " << b << endl;
	bool result = false;
	
	if (a.m_type != object::ot_undef and b.m_type != object::ot_undef)
	{
		try
		{
			result = (double)a < (double)b;
		}
		catch (boost::bad_lexical_cast&)
		{
			result = (string)a < (string)b;
		}
	}
	
	return el::object(result);
}

object operator<=(const object& a, const object& b)
{
//cerr << endl << __func__ << " a = " << a << " b = " << b << endl;
	bool result = false;
	
	if (a.m_type != object::ot_undef and b.m_type != object::ot_undef)
	{
		try
		{
			result = (double)a <= (double)b;
		}
		catch (boost::bad_lexical_cast&)
		{
			result = (string)a <= (string)b;
		}
	}
	
	return el::object(result);
}

object operator>=(const object& a, const object& b)
{
//cerr << endl << __func__ << " a = " << a << " b = " << b << endl;
	bool result = false;
	
	if (a.m_type != object::ot_undef and b.m_type != object::ot_undef)
	{
		try
		{
			result = (double)a >= (double)b;
		}
		catch (boost::bad_lexical_cast&)
		{
			result = (string)a >= (string)b;
		}
	}
	
	return el::object(result);
}

object operator>(const object& a, const object& b)
{
//cerr << endl << __func__ << " a = " << a << " b = " << b << endl;
	bool result = false;
	
	if (a.m_type != object::ot_undef and b.m_type != object::ot_undef)
	{
		try
		{
			result = (double)a > (double)b;
		}
		catch (boost::bad_lexical_cast&)
		{
			result = (string)a > (string)b;
		}
	}
	
	return el::object(result);
}

object operator!=(const object& a, const object& b)
{
//cerr << endl << __func__ << " a = " << a << " b = " << b << endl;
	bool result = false;
	
	if (a.m_type != object::ot_undef and b.m_type != object::ot_undef)
	{
		try
		{
			result = (double)a != (double)b;
		}
		catch (boost::bad_lexical_cast&)
		{
			result = (string)a != (string)b;
		}
	}
	
	return el::object(result);


//
//	if (a.m_type == object::ot_undef or b.m_type == object::ot_undef)
//		return object(false);
//	else if (a.m_type == object::ot_number and b.m_type == object::ot_number)
//		return object(a.m_number != b.m_number);
//	else if (a.m_type == object::ot_string and b.m_type == object::ot_string)
//		return object(a.m_string != b.m_string);
//	else if (a.m_type == object::ot_boolean and b.m_type == object::ot_boolean)
//		return object((a.m_number == 0) != (b.m_number == 0));
//	else
//		throw zeep::exception("incompatible types in compare");
}

object operator==(const object& a, const object& b)
{
//cerr << endl << __func__ << " a = " << a << " b = " << b << endl;
	bool result = false;
	
	if (a.m_type != object::ot_undef and b.m_type != object::ot_undef)
	{
		try
		{
			result = (double)a == (double)b;
		}
		catch (boost::bad_lexical_cast&)
		{
			result = (string)a == (string)b;
		}
	}
	
	return el::object(result);
//
//	if (a.m_type == object::ot_undef or b.m_type == object::ot_undef)
//		return object(false);
//	else if (a.m_type == object::ot_number and b.m_type == object::ot_number)
//		return object(a.m_number == b.m_number);
//	else if (a.m_type == object::ot_string and b.m_type == object::ot_string)
//		return object(a.m_string == b.m_string);
//	else if (a.m_type == object::ot_boolean and b.m_type == object::ot_boolean)
//		return object((a.m_number == 0) == (b.m_number == 0));
//	else
//		throw zeep::exception("incompatible types in compare");
}

object operator+(const object& a, const object& b)
{
// cerr << endl << __func__ << " a = " << a << " b = " << b << endl;
	if (a.m_type == object::ot_string or b.m_type == object::ot_string)
		return object((string)a + (string)b);

	if (a.m_type != object::ot_number or b.m_type != object::ot_number)
		throw zeep::exception("incompatible types in compare");
	return object(a.m_number + b.m_number);
}

object operator-(const object& a, const object& b)
{
// cerr << endl << __func__ << " a = " << a << " b = " << b << endl;

	if (a.m_type != object::ot_number or b.m_type != object::ot_number)
		throw zeep::exception("incompatible types in compare");
	return object(a.m_number - b.m_number);
}

object operator*(const object& a, const object& b)
{
// cerr << endl << __func__ << " a = " << a << " b = " << b << endl;

	if (a.m_type != object::ot_number or b.m_type != object::ot_number)
		throw zeep::exception("incompatible types in compare");
	return object(a.m_number * b.m_number);
}

object operator%(const object& a, const object& b)
{
// cerr << endl << __func__ << " a = " << a << " b = " << b << endl;

	if (a.m_type != object::ot_number or b.m_type != object::ot_number)
		throw zeep::exception("incompatible types in compare");
	return object(fmod(a.m_number, b.m_number));
}

object operator/(const object& a, const object& b)
{
// cerr << endl << __func__ << " a = " << a << " b = " << b << endl;

	if (a.m_type != object::ot_number or b.m_type != object::ot_number)
		throw zeep::exception("incompatible types in compare");
	return object(a.m_number / b.m_number);
}

object operator&&(const object& a, const object& b)
{
// cerr << endl << __func__ << " a = " << a << " b = " << b << endl;
	bool ba = (bool)a;
	bool bb = (bool)b;

	return object(ba && bb);
}

object operator||(const object& a, const object& b)
{
// cerr << endl << __func__ << " a = " << a << " b = " << b << endl;

	bool ba = (bool)a;
	bool bb = (bool)b;

	return object(ba || bb);
}

object operator-(const object& a)
{
	if (a.m_type != object::ot_number)
		throw zeep::exception("incompatible type in negate");
	return object(-a.m_number);
}

bool compare_object::operator()(const object& a, const object& b) const
{
	if (m_descending)
		return b[m_field] < a[m_field];
	else
		return a[m_field] < b[m_field];
}

enum token_type
{
	elt_undef,
	elt_eof,
	elt_number,
	elt_string,
	elt_object,
	elt_and, elt_or, elt_not, elt_empty,
	elt_eq, elt_ne, elt_lt, elt_le, elt_ge, elt_gt,
	elt_plus, elt_minus, elt_div, elt_mod, elt_mult,
	elt_lparen, elt_rparen, elt_lbracket, elt_rbracket,
	elt_if, elt_else,
	elt_dot
};

object interpreter::evaluate(
	const string&	s)
{
	object result;

	try
	{
		m_ptr = s.begin();
		m_end = s.end();
		get_next_token();
		
		if (m_lookahead != elt_eof)
			result = parse_expr();
		match(elt_eof);
	}
	catch (exception& e)
	{
//		if (VERBOSE)
//			cerr << e.what() << endl;
	}
	
	return result;
}

void interpreter::process(
	string&			s)
{
	try
	{
		m_ptr = s.begin();
		m_end = s.end();
		get_next_token();
		
		object result;
		
		if (m_lookahead != elt_eof)
			result = parse_expr();
		match(elt_eof);
		
		s = (string)result;
	}
	catch (exception& e)
	{
//		if (VERBOSE)
//			cerr << e.what() << endl;

		s = "error in el expression: ";
		s += e.what();
	}
}

void interpreter::match(
	uint32		t)
{
	if (t != m_lookahead)
		throw zeep::exception("syntax error");
	get_next_token();
}

unsigned char interpreter::next_byte()
{
	char result = 0;
	
	if (m_ptr < m_end)
	{
		result = *m_ptr;
		++m_ptr;
	}

	m_token_string += result;

	return static_cast<unsigned char>(result);
}

// We assume all paths are in valid UTF-8 encoding
unicode interpreter::get_next_char()
{
	unicode result = 0;
	unsigned char ch[5];
	
	ch[0] = next_byte();
	
	if ((ch[0] & 0x080) == 0)
		result = ch[0];
	else if ((ch[0] & 0x0E0) == 0x0C0)
	{
		ch[1] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080)
			throw zeep::exception("Invalid utf-8");
		result = ((ch[0] & 0x01F) << 6) | (ch[1] & 0x03F);
	}
	else if ((ch[0] & 0x0F0) == 0x0E0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080)
			throw zeep::exception("Invalid utf-8");
		result = ((ch[0] & 0x00F) << 12) | ((ch[1] & 0x03F) << 6) | (ch[2] & 0x03F);
	}
	else if ((ch[0] & 0x0F8) == 0x0F0)
	{
		ch[1] = next_byte();
		ch[2] = next_byte();
		ch[3] = next_byte();
		if ((ch[1] & 0x0c0) != 0x080 or (ch[2] & 0x0c0) != 0x080 or (ch[3] & 0x0c0) != 0x080)
			throw zeep::exception("Invalid utf-8");
		result = ((ch[0] & 0x007) << 18) | ((ch[1] & 0x03F) << 12) | ((ch[2] & 0x03F) << 6) | (ch[3] & 0x03F);
	}

	if (result > 0x10ffff)
		throw zeep::exception("invalid utf-8 character (out of range)");
	
	return result;
}

void interpreter::retract()
{
	string::iterator c = m_token_string.end();

	// skip one valid character back in the input buffer
	// since we've arrived here, we can safely assume input
	// is valid UTF-8
	do --c; while ((*c & 0x0c0) == 0x080);
	
	if (m_ptr != m_end or *c != 0)
		m_ptr -= m_token_string.end() - c;
	m_token_string.erase(c, m_token_string.end());
}

void interpreter::get_next_token()
{
	enum State {
		els_Start,
		els_Equals,
		els_ExclamationMark,
		els_LessThan,
		els_GreaterThan,
		els_Number,
		els_NumberFraction,
		els_Name,
		els_Literal
	} start, state;
	start = state = els_Start;

	token_type token = elt_undef;
	double fraction = 1.0;
	unicode quoteChar;

	m_token_string.clear();
	
	while (token == elt_undef)
	{
		unicode ch = get_next_char();
		
		switch (state)
		{
			case els_Start:
				switch (ch)
				{
					case 0:		token = elt_eof; break;
					case '(':	token = elt_lparen; break;
					case ')':	token = elt_rparen; break;
					case '[':	token = elt_lbracket; break;
					case ']':	token = elt_rbracket; break;
					case ':':	token = elt_else; break;
					case '?':	token = elt_if; break;
					case '*':	token = elt_mult; break;
					case '/':	token = elt_div; break;
					case '+':	token = elt_plus; break;
					case '-':	token = elt_minus; break;
					case '.':	token = elt_dot; break;
					case '=':	state = els_Equals; break;
					case '!':	state = els_ExclamationMark; break;
					case '<':	state = els_LessThan; break;
					case '>':	state = els_GreaterThan; break;
					case ' ':
					case '\n':
					case '\r':
					case '\t':
						m_token_string.clear();
						break;
					case '\'':	quoteChar = ch; state = els_Literal; break;
					case '"':	quoteChar = ch; state = els_Literal; break;

					default:
						if (ch >= '0' and ch <= '9')
						{
							m_token_number = ch - '0';
							state = els_Number;
						}
						else if (zeep::xml::is_name_start_char(ch))
							state = els_Name;
						else
							throw zeep::exception((boost::format("invalid character (%1%) in expression") % ch).str());
				}
				break;
			
			case els_Equals:
				if (ch != '=')
					retract();
				token = elt_eq;
				break;
			
			case els_ExclamationMark:
				if (ch != '=')
				{
					retract();
					throw zeep::exception("unexpected character ('!') in expression");
				}
				token = elt_ne;
				break;
			
			case els_LessThan:
				if (ch == '=')
					token = elt_le;
				else
				{
					retract();
					token = elt_lt;
				}
				break;

			case els_GreaterThan:
				if (ch == '=')
					token = elt_ge;
				else
				{
					retract();
					token = elt_gt;
				}
				break;
			
			case els_Number:
				if (ch >= '0' and ch <= '9')
					m_token_number = 10 * m_token_number + (ch - '0');
				else if (ch == '.')
				{
					fraction = 0.1;
					state = els_NumberFraction;
				}
				else
				{
					retract();
					token = elt_number;
				}
				break;
			
			case els_NumberFraction:
				if (ch >= '0' and ch <= '9')
				{
					m_token_number += fraction * (ch - '0');
					fraction /= 10;
				}
				else
				{
					retract();
					token = elt_number;
				}
				break;
			
			case els_Name:
				if (ch == '.' or not zeep::xml::is_name_char(ch))
				{
					retract();
					if (m_token_string == "div")
						token = elt_div;
					else if (m_token_string == "mod")
						token = elt_mod;
					else if (m_token_string == "and")
						token = elt_and;
					else if (m_token_string == "or")
						token = elt_or;
					else if (m_token_string == "not")
						token = elt_not;
					else if (m_token_string == "empty")
						token = elt_empty;
					else if (m_token_string == "lt")
						token = elt_lt;
					else if (m_token_string == "le")
						token = elt_le;
					else if (m_token_string == "ge")
						token = elt_ge;
					else if (m_token_string == "gt")
						token = elt_gt;
					else if (m_token_string == "ne")
						token = elt_ne;
					else if (m_token_string == "eq")
						token = elt_eq;
					else
						token = elt_object;
				}
				break;
			
			case els_Literal:
				if (ch == 0)
					throw zeep::exception("run-away string, missing quote character?");
				else if (ch == quoteChar)
				{
					token = elt_string;
					m_token_string = m_token_string.substr(1, m_token_string.length() - 2);
				}
				break;
		}
	}
	
	m_lookahead = token;
}

object interpreter::parse_expr()
{
	object result = parse_or_expr();
	if (m_lookahead == elt_if)
	{
		match(m_lookahead);
		object a = parse_expr();
		match(elt_else);
		object b = parse_expr();
		if (result)
			result = a;
		else
			result = b;
	}
	return result;
}

object interpreter::parse_or_expr()
{
	object result = parse_and_expr();
	while (m_lookahead == elt_or)
	{
		match(m_lookahead);
		result = result or parse_and_expr();
	}
	return result;
}

object interpreter::parse_and_expr()
{
	object result = parse_equality_expr();
	while (m_lookahead == elt_and)
	{
		match(m_lookahead);
		result = result and parse_equality_expr();
	}
	return result;
}

object interpreter::parse_equality_expr()
{
	object result = parse_relational_expr();
	if (m_lookahead == elt_eq)
	{
		match(m_lookahead);
		result = (result == parse_relational_expr());
	}
	else if (m_lookahead == elt_ne)
	{
		match(m_lookahead);
		result = (result != parse_relational_expr());
	}
	return result;
}

object interpreter::parse_relational_expr()
{
	object result = parse_additive_expr();
	switch (m_lookahead)
	{
		case elt_lt:	match(m_lookahead); result = (result < parse_additive_expr()); break;
		case elt_le:	match(m_lookahead); result = (result <= parse_additive_expr()); break;
		case elt_ge:	match(m_lookahead); result = (result >= parse_additive_expr()); break;
		case elt_gt:	match(m_lookahead); result = (result > parse_additive_expr()); break;
		default:		break;
	}
	return result;
}

object interpreter::parse_additive_expr()
{
	object result = parse_multiplicative_expr();
	while (m_lookahead == elt_plus or m_lookahead == elt_minus)
	{
		if (m_lookahead == elt_plus)
		{
			match(m_lookahead);
			result = (result + parse_multiplicative_expr());
		}
		else
		{
			match(m_lookahead);
			result = (result - parse_multiplicative_expr());
		}
	}
	return result;
}

object interpreter::parse_multiplicative_expr()
{
	object result = parse_unary_expr();
	while (m_lookahead == elt_div or m_lookahead == elt_mod or m_lookahead == elt_mult)
	{
		if (m_lookahead == elt_mult)
		{
			match(m_lookahead);
			result = (result * parse_unary_expr());
		}
		else if (m_lookahead == elt_div)
		{
			match(m_lookahead);
			result = (result / parse_unary_expr());
		}
		else
		{
			match(m_lookahead);
			result = (result % parse_unary_expr());
		}
	}
	return result;
}

object interpreter::parse_unary_expr()
{
	object result;
	if (m_lookahead == elt_minus)
	{
		match(m_lookahead);
		result = -(parse_primary_expr());
	}
	else if (m_lookahead == elt_not)
	{
		match(m_lookahead);
		result = not (bool)parse_primary_expr();
	}
	else
		result = parse_primary_expr();
	return result;
}

object interpreter::parse_primary_expr()
{
	object result;
	switch (m_lookahead)
	{
		case elt_number:	result = m_token_number; match(m_lookahead); break;
		case elt_string:	result = m_token_string; match(m_lookahead); break;
		case elt_lparen:
			match(m_lookahead);
			result = parse_expr();
			match(elt_rparen);
			break;
		case elt_object:
			result = m_scope.lookup(m_token_string);
			match(elt_object);
			for (;;)
			{
				if (m_lookahead == elt_dot)
				{
					match(m_lookahead);
					if (result.is_array() and m_token_string == "count")
						result = el::object(result.count());
					else
						result = result[m_token_string];
					match(elt_object);
					continue;
				}
				
				if (m_lookahead == elt_lbracket)
				{
					match(m_lookahead);
					
					uint32 index = m_token_number;
					match(elt_number);
					match(elt_rbracket);
					
					result = result[index];
					continue;
				}

				break;
			}
			break;
		case elt_empty:
			match(m_lookahead);
			if (m_lookahead != elt_object)
				throw zeep::exception("syntax error, expected an object after operator 'empty'");
			result = parse_primary_expr().empty();
			break;
		default:
			throw zeep::exception("syntax error, expected number, string or object");
	}
	return result;
}

ostream& operator<<(ostream& lhs, const scope& rhs)
{
	const scope* s = &rhs;
	while (s != nil)
	{
		foreach (auto e, s->m_data)
			lhs << e.first << " = " << e.second << endl;
		s = s->m_next;
	}
	return lhs;
}

scope::scope(const scope& next)
	: m_next(const_cast<scope*>(&next))
	, m_req(nil)
{
}

scope::scope(const request& req)
	: m_next(nil)
	, m_req(&req)
{
}

object& scope::operator[](
	const std::string& name)
{
	return lookup(name);
}

const object& scope::lookup(
	const std::string&	name) const
{
	std::map<std::string,object>::const_iterator i = m_data.find(name);
	if (i != m_data.end())
		return i->second;
	else if (m_next != nil)
		return m_next->lookup(name);
	
	static object s_null;
	return s_null;
}

const object& scope::operator[](
	const std::string& name) const
{
	return lookup(name);
}

object& scope::lookup(
	const std::string&	name)
{
	object* result = nil;

	std::map<std::string,object>::iterator i = m_data.find(name);
	if (i != m_data.end())
		result = &i->second;
	else if (m_next != nil)
		result = &m_next->lookup(name);
	else
	{
		m_data[name] = object();
		result = &m_data[name];
	}
	 
	 return *result;
}

const request& scope::get_request() const
{
	if (m_next)
		return m_next->get_request();
	if (m_req == nil)
		throw zeep::exception("Invalid scope, no request");
	return *m_req;
}

}
}
}
