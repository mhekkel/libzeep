// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2020
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file
/// definition of the zeep::xml::document class

#include <zeep/config.hpp>

#include <zeep/xml/character-classification.hpp>
#include <zeep/xml/parser.hpp>
#include <zeep/xml/serialize.hpp>

namespace zeep::xml
{

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

struct doc_type
{
	std::string m_root;
	std::string m_pubid;	/// pubid is empty for SYSTEM DOCTYPE
	std::string m_dtd;
};

class document : public element
{
  public:

	/// \brief Constructor for an empty document.
	document();

	/// \brief Copy constructor
	document(const document& doc);

	/// \brief Move constructor
	document(document&& other);

	/// \brief Copy operator=
	document& operator=(const document& doc);

	/// \brief Move operator=
	document& operator=(document&& other);

	/// \brief Constructor that will parse the XML passed in argument using default settings \a s
	document(const std::string& s);

	/// \brief Constructor that will parse the XML passed in argument using default settings \a is
	document(std::istream& is);

	/// \brief Constructor that will parse the XML passed in argument \a is. This
	/// constructor will also validate the input using DTD's found in \a base_dir
	document(std::istream& is, const std::string& base_dir);

	virtual ~document();

	/// options for parsing
	/// validating uses a DTD if it is defined
	bool is_validating() const								{ return m_validating; }
	void set_validating(bool validate)						{ m_validating = validate; }

	/// preserve cdata, preserves CDATA sections instead of converting them
	/// into text nodes.
	bool preserves_cdata() const							{ return m_preserve_cdata; }

	/// \brief if \a p is true, the CDATA sections will be preserved when parsing XML, if \a p is false, the content of the CDATA will be treated as text
	void set_preserve_cdata(bool p)							{ m_preserve_cdata = p; }

	/// \brief collapse means replacing e.g. `<foo></foo>` with `<foo/>`
	bool collapses_empty_tags() const						{ return m_fmt.collapse_tags; }

	/// \brief if \a c is true, empty tags will be replaced, i.e. write `<foo/>` instead of `<foo></foo>`
	void set_collapse_empty_tags(bool c)					{ m_fmt.collapse_tags = c; }

	/// \brief whether to write out comments
	bool suppresses_comments() const						{ return m_fmt.suppress_comments; }

	/// \brief if \a s is true, comments will not be written
	void set_suppress_comments(bool s)						{ m_fmt.suppress_comments = s; }

	/// \brief whether to escape white space
	bool escapes_white_space() const						{ return m_fmt.escape_white_space; }

	/// \brief if \a e is true, white space will be written as XML entities
	void set_escape_white_space(bool e)						{ m_fmt.escape_white_space = e; }

	/// \brief whether to escape double quotes
	bool escapes_double_quote() const						{ return m_fmt.escape_double_quote; }

	/// \brief if \a e is true, double quotes will be written as &quot;
	void set_escape_double_quote(bool e)					{ m_fmt.escape_double_quote = e; }

	/// \brief whether to place a newline after a prolog
	bool wraps_prolog() const								{ return m_wrap_prolog; }

	/// \brief if \a w is true, a newline will be written after the XML prolog
	void set_wrap_prolog(bool w)							{ m_wrap_prolog = w; }

	/// \brief Get the doctype as parsed
	doc_type get_doctype() const							{ return m_doctype; }

	/// \brief Set the doctype to write out
	void set_doctype(const std::string& root, const std::string& pubid, const std::string& dtd)
	{
		set_doctype({root, pubid, dtd});
	}

	/// Set the doctype to write out
	void set_doctype(const doc_type& doctype)				{ m_doctype = doctype; m_write_doctype = true; }

	/// \brief whether to write a XML prolog
	bool writes_xml_decl() const							{ return m_write_xml_decl; }

	/// \brief if \a w is true, an XML prolog will be written
	void set_write_xml_decl(bool w)							{ m_write_xml_decl = w; }

	/// \brief whether to write a DOCTYPE
	bool writes_doctype() const								{ return m_write_doctype; }

	/// \brief if \a f is true a DOCTYPE will be written
	void set_write_doctype(bool f)							{ m_write_doctype = f; }

	/// \brief Check the doctype to see if this is supposed to be HTML5
	bool is_html5() const;

	/// \brief Write out the document
	friend std::ostream& operator<<(std::ostream& os, const document& doc);

	/// \brief Read in a document
	friend std::istream& operator>>(std::istream& is, document& doc);

	/// \brief Serialization support
	template <typename T>
	void serialize(const char* name, const T& data); ///< Serialize \a data into a document containing \a name as root node

	/// \brief Serialization support
	template <typename T>
	void deserialize(const char* name, T& data); ///< Deserialize root node with name \a name into \a data.

	/// Compare two xml documents
	bool operator==(const document& doc) const;
	bool operator!=(const document& doc) const { return not operator==(doc); }

	/// If you want to validate the document using DTD files stored on disk, you can specifiy this directory prior to reading
	/// the document.
	void set_base_dir(const std::string& path);

	/// If you want to be able to load external documents other than trying to read them from disk
	/// you can set a callback here.
	template<typename Callback>
	void set_entity_loader(Callback&& cb)
	{
		m_external_entity_ref_loader = cb;
	}

	encoding_type get_encoding() const;   ///< The text encoding as detected in the input.
	void set_encoding(encoding_type enc); ///< The text encoding to use for output

	float get_version() const;			///< XML version, should be either 1.0 or 1.1
	void set_version(float v);			///< XML version, should be either 1.0 or 1.1

	virtual element* root()					{ return this; }
	virtual const element* root() const		{ return this; }

	virtual node* child()					{ return empty() ? nullptr : &front(); }
	virtual const node* child() const		{ return empty() ? nullptr : &front(); }

  protected:

	virtual node_iterator insert_impl(const_iterator pos, node* n);

	void XmlDeclHandler(encoding_type encoding, bool standalone, float version);
	void StartElementHandler(const std::string& name, const std::string& uri, const parser::attr_list_type& atts);
	void EndElementHandler(const std::string& name, const std::string& uri);
	void CharacterDataHandler(const std::string& data);
	void ProcessingInstructionHandler(const std::string& target, const std::string& data);
	void CommentHandler(const std::string& comment);
	void StartCdataSectionHandler();
	void EndCdataSectionHandler();
	void StartNamespaceDeclHandler(const std::string& prefix, const std::string& uri);
	void EndNamespaceDeclHandler(const std::string& prefix);
	void DoctypeDeclHandler(const std::string& root, const std::string& publicId, const std::string& uri);
	void NotationDeclHandler(const std::string& name, const std::string& sysid, const std::string& pubid);

	std::istream* external_entity_ref(const std::string& base, const std::string& pubid, const std::string& sysid);
	void parse(std::istream& data);

	// /// \brief To read a document and process elements on the go, use this streaming input function.
	// /// If the \a proc callback retuns false, processing is terminated. The \a doc_root parameter of
	// /// the callback is the leading xml up to the first element.
	// void process_document_elements(std::istream& data, const std::string& element_xpath,
	// 							std::function<bool(node* doc_root, element* e)> cb);

	/// The default for libzeep is to locate the external reference based
	/// on sysid and base_dir. Only local files are loaded this way.
	/// You can specify a entity loader here if you want to be able to load
	/// DTD files from another source.
	std::function<std::istream *(const std::string& base, const std::string& pubid, const std::string& sysid)>
		m_external_entity_ref_loader;

	virtual void write(std::ostream& os, format_info fmt) const;

	std::string m_dtd_dir;

	// some content information
	doc_type m_doctype;
	bool m_validating;
	bool m_preserve_cdata;
	bool m_has_xml_decl;
	encoding_type m_encoding;
	float m_version;
	bool m_standalone;
	bool m_wrap_prolog = true;
	bool m_write_doctype = false;
	bool m_write_xml_decl = false;

	format_info m_fmt;

	struct notation
	{
		std::string m_name;
		std::string m_sysid;
		std::string m_pubid;
	};

	element* m_cur = nullptr; // construction
	cdata* m_cdata = nullptr; // only defined in a CDATA section
	std::vector<std::pair<std::string, std::string>> m_namespaces;
	std::list<notation> m_notations;
	size_t m_root_size_at_first_notation = 0;	// for processing instructions that occur before a notation
};

namespace literals
{

document operator""_xml(const char* text, size_t length);

}

template <typename T>
void document::serialize(const char* name, const T& data)
{
	serializer sr(*this);
	sr.serialize_element(name, data);
}

template <typename T>
void document::deserialize(const char* name, T& data)
{
	if (child() == nullptr)
		throw zeep::exception("empty document");

	if (child()->name() != name)
		throw zeep::exception("root mismatch");

	deserializer sr(*this);
	sr.deserialize_element(name, data);
}

} // namespace zeep::xml
