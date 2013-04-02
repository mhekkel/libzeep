// Copyright Maarten L. Hekkelman, Radboud University 2008-2012.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_DOCUMENT_H
#define SOAP_XML_DOCUMENT_H

#include <boost/filesystem/path.hpp>
#include <boost/function.hpp>

#include <zeep/config.hpp>
#include <zeep/xml/node.hpp>
#include <zeep/xml/unicode_support.hpp>
#include <zeep/xml/serialize.hpp>

namespace zeep { namespace xml {

#if SOAP_XML_HAS_EXPAT_SUPPORT
/// By default libzeep will use its own XML SAX parser, but you can use
/// expat instead.
enum parser_type
{
	parser_zeep,
	parser_expat
};
#endif

struct document_imp;

/// zeep::xml::document is the class that contains a parsed XML file.
/// You can create an empty document and add nodes to it, or you can
/// create it by specifying a string containing XML or an std::istream
/// to parse.
///
/// If you use an std::fstream to read a file, be sure to open the file
/// ios::binary. Otherwise, the detection of text encoding might go wrong
/// or the content can become corrupted.
///
/// Default is to parse CDATA sections into zeep::xml::text nodes. If you
/// want to preserve CDATA sections in the DOM tree, you have to call
/// set_preserve_cdata before reading the file.
/// 
/// By default a document is not validated. But you can turn on validation
/// by using the appropriate constructor or read method, or by setting
/// set_validating explicitly. The DTD's will be loaded from the base dir
/// specified, but you can change this by assigning a external_entity_ref_handler.
///
/// A document has one zeep::xml::root_node element. This root element
/// can have only one zeep::xml::element child node.

class document
{
  public:
	/// \brief Constructor for an empty document.
						document();

	/// \brief Constructor that will parse the XML passed in argument \a s
						document(const std::string& s);

	/// \brief Constructor that will parse the XML passed in argument \a is
						document(std::istream& is);

	/// \brief Constructor that will parse the XML passed in argument \a is. This
	/// constructor will also validate the input using DTD's found in \a base_dir
						document(std::istream& is, const boost::filesystem::path& base_dir);

#ifndef LIBZEEP_DOXYGEN_INVOKED
	virtual				~document();
#endif

	// I/O
	void				read(const std::string& s);		///< Replace the content of the document with the parsed XML in \a s
	void				read(std::istream& is);			///< Replace the content of the document with the parsed XML in \a is
	void				read(std::istream& is, const boost::filesystem::path& base_dir);
														///< Replace the content of the document with the parsed XML in \a is and use validation based on DTD's found in \a base_dir

	virtual void		write(writer& w) const;			///< Write the contents of the document as XML using zeep::xml::writer object \a w

	/// Serialization support
	template<typename T>
	void				serialize(const char* name, const T& data);		///< Serialize \a data into a document containing \a name as root node
	
	template<typename T>
	void				deserialize(const char* name, T& data);			///< Deserialize root node with name \a name into \a data.

	/// A valid xml document contains exactly one zeep::xml::root_node element
	root_node*			root() const;
	
	/// The root has one child zeep::xml::element
	element*			child() const;
	void				child(element* e);
	
	// helper functions
	element_set			find(const std::string& path) const			///< Return all zeep::xml::elements that match the XPath query \a path
																	{ return find(path.c_str()); }
	element*			find_first(const std::string& path) const	///< Return the first zeep::xml::element that matches the XPath query \a path
																	{ return find_first(path.c_str()); }

	element_set			find(const char* path) const;				///< Return all zeep::xml::elements that match the XPath query \a path
	element*			find_first(const char* path) const;			///< Return the first zeep::xml::element that matches the XPath query \a path

	void				find(const char* path, node_set& nodes) const;	///< Return all zeep::xml::nodes (attributes or elements) that match the XPath query \a path
	void				find(const char* path, element_set& elements) const;	///< Return all zeep::xml::elements that match the XPath query \a path
	node*				find_first_node(const char* path) const;		///< Return the first zeep::xml::node (attribute or element) that matches the XPath query \a path

	/// Compare two xml documents
	bool				operator==(const document& doc) const;
	bool				operator!=(const document& doc) const		{ return not operator==(doc); }
	
	/// If you want to validate the document using DTD files stored on disk, you can specifiy this directory prior to reading
	/// the document.
	void				base_dir(const boost::filesystem::path& path);

	/// The default for libzeep is to locate the external reference based
	/// on sysid and base_dir. Only local files are loaded this way.
	/// You can specify a entity loader here if you want to be able to load
	/// DTD files from another source.
	boost::function<std::istream*(const std::string& base, const std::string& pubid, const std::string& sysid)>
						external_entity_ref_handler;

	encoding_type		encoding() const;				///< The text encoding as detected in the input.	
	void				encoding(encoding_type enc);	///< The text encoding to use for output

	// to format the output, use the following:
	int					indent() const;					///< get number of spaces to indent elements:
	void				indent(int indent);				///< set number of spaces to indent elements:
	
	// whether to have each element on a separate line
	bool				wrap() const;					///< get wrap flag, whether elements will appear on their own line or not
	void				wrap(bool wrap);				///< set wrap flag, whether elements will appear on their own line or not
	
	// reduce the whitespace in #PCDATA sections
	bool				trim() const;					///< get trim flag, strips white space in #PCDATA sections
	void				trim(bool trim);				///< set trim flag, strips white space in #PCDATA sections

	// suppress writing out comments
	bool				no_comment() const;				///< get no_comment flag, suppresses the output of XML comments
	void				no_comment(bool no_comment);	///< get no_comment flag, suppresses the output of XML comments

	/// options for parsing
	/// validating uses a DTD if it is defined
	void				set_validating(bool validate);
	/// preserve cdata, preserves CDATA sections instead of converting them
	/// into text nodes.
	void				set_preserve_cdata(bool preserve_cdata);
	
#if SOAP_XML_HAS_EXPAT_SUPPORT
	/// By default, libzeep uses its own parser implementation. But if you prefer
	/// expat you can specify that here.
	static void			set_parser_type(parser_type type);
#endif

#ifndef LIBZEEP_DOXYGEN_INVOKED
  protected:
						document(struct document_imp* impl);

	friend void process_document_elements(std::istream& data, const std::string& element_xpath,
										boost::function<bool(node* doc_root, element* e)> cb);

#if SOAP_XML_HAS_EXPAT_SUPPORT
	static parser_type	s_parser_type;
#endif
	document_imp*		create_imp(document* doc);

	document_imp*		m_impl;

  private:
						document(const document&);
	document&			operator=(const document&);
#endif
};

/// Using operator>> is an alternative for calling rhs.read(lhs);
std::istream& operator>>(std::istream& lhs, document& rhs);

/// Using operator<< is an alternative for calling writer w(lhs); rhs.write(w);
std::ostream& operator<<(std::ostream& lhs, const document& rhs);

/// \brief To read a document and process elements on the go, use this streaming input function.
/// If the \a proc callback retuns false, processing is terminated. The \a doc_root parameter of
/// the callback is the leading xml up to the first element.
void process_document_elements(std::istream& data, const std::string& element_xpath,
	boost::function<bool(node* doc_root, element* e)> cb);

#ifndef LIBZEEP_DOXYGEN_INVOKED

template<typename T>
void document::serialize(const char* name, const T& data)
{
	serializer sr(root());
	sr.serialize_element(name, data);
}

template<typename T>
void document::deserialize(const char* name, T& data)
{
	if (child() == nullptr)
		throw zeep::exception("empty document");

	if (child()->name() != name)
		throw zeep::exception("root mismatch");

	deserializer sr(root());
	sr.deserialize_element(name, data);
}

#endif

}
}

#endif
