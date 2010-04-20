//  Copyright Maarten L. Hekkelman, Radboud University 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_DOCUMENT_EXPAT_H
#define SOAP_XML_DOCUMENT_EXPAT_H

#include "zeep/config.hpp"
#include "document-imp.hpp"
#if SOAP_XML_HAS_EXPAT_SUPPORT
#include <expat.h>

namespace zeep { namespace xml {

struct expat_doc_imp : public document_imp
{
					expat_doc_imp(document* doc);

	static void		XML_StartElementHandler(
						void*				userData,
						const XML_Char*		name,
						const XML_Char**	atts);

	static void		XML_EndElementHandler(
						void*				userData,
						const XML_Char*		name);

	static void		XML_CharacterDataHandler(
						void*				userData,
						const XML_Char*		s,
						int					len);

	static void		XML_ProcessingInstructionHandler(
						void*				userData,
						const XML_Char*		target,
						const XML_Char*		data);

	static void		XML_CommentHandler(
						void*				userData,
						const XML_Char*		data);

	static void		XML_StartCdataSectionHandler(
						void *userData);

	static void		XML_EndCdataSectionHandler(
						void *userData);

	static void		XML_StartNamespaceDeclHandler(
                        void*				userData,
                        const XML_Char*		prefix,
                        const XML_Char*		uri);

	static void		XML_EndNamespaceDeclHandler(
						void*				userData,
						const XML_Char*		prefix);

	static void		XML_NotationDeclHandler(
                        void*				userData,
                        const XML_Char*		notationName,
                        const XML_Char*		base,
                        const XML_Char*		systemId,
                        const XML_Char*		publicId);

	static int		XML_ExternalEntityRefHandler(
                        XML_Parser			parser,
                        const XML_Char*		context,
                        const XML_Char*		base,
                        const XML_Char*		systemId,
                        const XML_Char*		publicId);

	void			StartElementHandler(
						const XML_Char*		name,
						const XML_Char**	atts);

	void			EndElementHandler(
						const XML_Char*		name);

	void			CharacterDataHandler(
						const XML_Char*		s,
						int					len);

	void			ProcessingInstructionHandler(
						const XML_Char*		target,
						const XML_Char*		data);

	void			CommentHandler(
						const XML_Char*		data);

	void			StartCdataSectionHandler();

	void			EndCdataSectionHandler();

	void			StartNamespaceDeclHandler(
                        const XML_Char*		prefix,
                        const XML_Char*		uri);

	void			EndNamespaceDeclHandler(
						const XML_Char*		prefix);

	void			NotationDeclHandler(
                        const XML_Char*		notationName,
                        const XML_Char*		base,
                        const XML_Char*		systemId,
                        const XML_Char*		publicId);

	void			parse(
						std::istream&		data);

	void			parse_name(
						const char*			name,
						std::string&		element,
						std::string&		ns,
						std::string&		prefix);

};

}
}

#endif
#endif
