// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; -*-

#include <libxml/globals.h>
#include <libxml/tree.h>

#include "XmlUtils.h"

USING_NAMESPACE_FIVEL


//=========================================================================
//  Support Code
//=========================================================================

const xml_node::char_type *xml_node::to_utf8(const char *inStr)
{
	ASSERT(xmlCheckUTF8(reinterpret_cast<const unsigned char*>(inStr)));
	return reinterpret_cast<const xmlChar*>(inStr);
}

const char *xml_node::to_ascii(const char_type *inStr)
{
	// This function should check carefully for UTF-8 characters.  Failing
	// to do so may result in overlong-encoding attacks, which might or might
	// not matter to the security of Tamale when run with untrusted data.
	// We check for UTF-8 data by checking for the high bit.
	unsigned const char *result =
		reinterpret_cast<unsigned const char*>(inStr);
	for (unsigned const char *c = result; *c != '\0'; c++)
		CHECK(*c < 128, "Unexpected UTF-8 data in XML file");
	return reinterpret_cast<const char*>(result);
	
}


//=========================================================================
//  xml_node::iterator Methods
//=========================================================================

xml_node::iterator::iterator(node_ptr inNode, bool inIsInMixed)
	: mNode(inNode), mIsInMixed(inIsInMixed)
{
	if (!mIsInMixed)
		skip_whitespace();
}


void xml_node::iterator::skip_whitespace()
{
	while (mNode != NULL && xmlNodeIsText(mNode))
	{
		// XXX - Disabled to work around bug in libxml2 2.5.0, which thinks
		// that the "\n  " after the open tag of the root element is not
		// a blank node.  Feel free to enable this check if the bug is
		// fixed.
		//CHECK(xmlIsBlankNode(mNode), "Unexpected text among XML elements");
		mNode = mNode->next;
	}
}

xml_node::iterator &xml_node::iterator::operator++()
{
    CHECK(mNode != NULL, "Unexpected end of XML elements");
	mNode = mNode->next;
	if (!mIsInMixed)
		skip_whitespace();
	return *this;
}


//=========================================================================
//  xml_node Methods
//=========================================================================

size_t xml_node::size()
{
	size_t count = 0;
	for (iterator node = begin(); node != end(); ++node)
		++count;
	return count;
}

xml_node::iterator xml_node::begin()
{
	return iterator(mNode->children);
}

size_t xml_node::size_mixed()
{
	size_t count = 0;
	for (iterator node = begin_mixed(); node != end_mixed(); ++node)
		++count;
	return count;
}

xml_node::iterator xml_node::begin_mixed()
{
	return iterator(mNode->children, true);
}

xml_node xml_node::only_child()
{
	iterator node = begin();
	CHECK(node != end(), "Expected XML node to have one child");
	iterator tester = node;
	CHECK(++tester == end(), "Expected XML node to have only one child");
	return *node;
}

xml_node::string xml_node::name()
{
	return string(to_ascii(mNode->name));
}

xml_node::string xml_node::attribute(const char *inName)
{
	xmlChar *attr = xmlGetProp(mNode, to_utf8(inName));
	CHECK(attr, "Missing expected XML attribute");
	string result(to_ascii(attr));
	xmlFree(attr);
	return result;
}

xml_node::string xml_node::text()
{
	// Extract the text, expanding entities.
	// TODO - Check for element content (which is ignored by
	// xmlNodeListGetString at the moment).
	xmlChar *text = xmlNodeListGetString(mNode->doc, mNode->children, 1);
	if (text == NULL)
		return "";
	else
	{
		string result(to_ascii(text));
		xmlFree(text);
		return result;
	}
}

void xml_node::append_text(const std::string &inText)
{
	CHECK(xmlAddChild(mNode, xmlNewText(to_utf8(inText.c_str()))),
		  "Could not add text to XML tree");
}

xml_node xml_node::new_child(const char *inName)
{
	return xmlNewChild(mNode, NULL, to_utf8(inName), NULL);
}

xml_node xml_node::new_child(const char *inName, const std::string &inData)
{
	return xmlNewTextChild(mNode, NULL, to_utf8(inName),
						   to_utf8(inData.c_str()));
}

void xml_node::set_attribute(const char *inName, const std::string &inValue)
{
	xmlSetProp(mNode, to_utf8(inName), to_utf8(inValue.c_str()));
}