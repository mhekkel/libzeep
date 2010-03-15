//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_PARSER_H
#define SOAP_XML_PARSER_H

#include <boost/function.hpp>
#include <boost/filesystem/path.hpp>

#include "zeep/xml/node.hpp"

namespace zeep { namespace xml {

class basic_parser_base : public boost::noncopyable
{
  public:
	virtual				~basic_parser_base() {}
	
	std::string			wstring_to_string(const std::wstring& s);

  protected:
						basic_parser_base(struct parser_imp* impl, std::istream* is)
							: m_impl(impl), m_istream(is) {}

	typedef std::list<std::pair<std::wstring,std::wstring> >		att_list;

	virtual void		start_element(const std::wstring& name, const att_list& atts) = 0;

	virtual void		end_element(const std::wstring& name) = 0;

	virtual void		character_data(const std::wstring& data) = 0;
	
	virtual void		processing_instruction(const std::wstring& target, const std::wstring& data) = 0;

	virtual void		comment(const std::wstring& data) = 0;

	virtual void		start_cdata_section() = 0;

	virtual void		end_cdata_section() = 0;

	virtual void		start_namespace_decl(const std::wstring& prefix, const std::wstring& uri) = 0;

	virtual void		end_namespace_decl(const std::wstring& prefix) = 0;

	virtual std::istream*
						find_external_dtd(const std::wstring& pubid, const std::wstring& uri) = 0;

	struct parser_imp*	m_impl;
	std::istream*		m_istream;
};

template<typename CharT>
struct parser_text_encoding_traits
{
};

template<>
struct parser_text_encoding_traits<char>
{
						parser_text_encoding_traits(basic_parser_base& parser)
							: m_parser(parser) {}

	std::string			convert(const std::wstring& s)
						{
							return m_parser.wstring_to_string(s);
						}

	basic_parser_base&	m_parser;
};

template<>
struct parser_text_encoding_traits<wchar_t>
{
						parser_text_encoding_traits(basic_parser_base& parser) {}

	const std::wstring&	convert(const std::wstring& s)
						{
							return s;
						}
};

template<typename CharT>
class basic_parser : public basic_parser_base
{
	friend struct parser_imp;
	
  public:

	typedef std::basic_string<CharT>						string_type;
	typedef parser_text_encoding_traits<CharT>				text_traits;
	typedef std::list<std::pair<string_type,string_type> >	att_list_type;

							basic_parser(
								std::istream&		data);
							
							basic_parser(
								const string_type&	data);
		
							~basic_parser();

	boost::function<void(const string_type& name,
				const att_list_type& atts)>					start_element_handler;

	boost::function<void(const string_type& name)>			end_element_handler;

	boost::function<void(const string_type& data)>			character_data_handler;
	
	boost::function<void(const string_type& target,
				const string_type& data)>					processing_instruction_handler;

	boost::function<void(const string_type& data)>			comment_handler;

	boost::function<void()>									start_cdata_section_handler;

	boost::function<void()>									end_cdata_section_handler;

	boost::function<void(const string_type& prefix,
				const string_type& uri)>					start_namespace_decl_handler;

	boost::function<void(const string_type& prefix)>		end_namespace_decl_handler;

	boost::function<std::istream*(const string_type& pubid, const string_type& uri)>
															find_external_dtd_handler;

	void					parse();

  private:

	virtual void			start_element(const std::wstring& name, const att_list& atts)
							{
								if (start_element_handler)
								{
									att_list_type attributes;
									
									for (att_list::const_iterator att = atts.begin(); att != atts.end(); ++att)
									{
										std::pair<std::string,std::string> attribute;
										attribute.first = m_traits.convert(att->first);
										attribute.second = m_traits.convert(att->second);
										
										attributes.push_back(attribute);
									}
									
									start_element_handler(m_traits.convert(name), attributes);
								}
							}

	virtual void			end_element(const std::wstring& name)
							{
								if (end_element_handler)
									end_element_handler(m_traits.convert(name));
							}


	virtual void			character_data(const std::wstring& data)
							{
								if (character_data_handler)
									character_data_handler(m_traits.convert(data));
							}
	
	virtual void			processing_instruction(const std::wstring& target, const std::wstring& data)
							{
								if (processing_instruction_handler)
									processing_instruction_handler(m_traits.convert(target), m_traits.convert(data));
							}
	
	virtual void			comment(const std::wstring& data)
							{
								if (comment_handler)
									comment_handler(m_traits.convert(data));
							}


	virtual void			start_cdata_section()
							{
								if (start_cdata_section_handler)
									start_cdata_section_handler();
							}

	virtual void			end_cdata_section()
							{
								if (end_cdata_section_handler)
									end_cdata_section_handler();
							}

	virtual void			start_namespace_decl(const std::wstring& prefix, const std::wstring& uri)
							{
								if (start_namespace_decl_handler)
									start_namespace_decl_handler(m_traits.convert(prefix), m_traits.convert(uri));
							}

	virtual void			end_namespace_decl(const std::wstring& prefix)
							{
								if (end_namespace_decl_handler)
									end_namespace_decl_handler(m_traits.convert(prefix));
							}
	

	virtual std::istream*	find_external_dtd(const std::wstring& pubid, const std::wstring& uri)
							{
								std::istream* result = NULL;
								if (find_external_dtd_handler)
									result = find_external_dtd_handler(m_traits.convert(pubid), m_traits.convert(uri));
								return result;
							}
	
	text_traits				m_traits;
};

typedef basic_parser<char>		parser;
typedef basic_parser<wchar_t>	wparser;

}
}

#endif
