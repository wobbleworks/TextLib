///----------------------------------------
///      @file HTML.h
///   @ingroup TextLib
///     @brief Lightweight HTML document parser and DOM traversal built on libxml2.
///    @author Created by John Stephen on 5/9/14.
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <libxml/HTMLparser.h>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

///----------------------------------------
namespace HTML {
///----------------------------------------

///----------------------------------------
/// @brief An anchor's href paired with its visible label text.
///----------------------------------------

struct AnchorLink {
	std::string href;
	std::string label;
};

///----------------------------------------
/// MARK: Node
///----------------------------------------

///----------------------------------------
/// @brief Method wrapper around libxml2's xmlNode for HTML traversal and queries.
/// @details Inherits directly from xmlNode with no additional data members,
///          allowing safe casting between xmlNode* and Node*.
///----------------------------------------

struct Node : xmlNode {
	/// @brief Prints this node and its subtree for debugging.
	void print(std::string_view prefix) const;
	
	/// @brief Finds the first node matching a tag whose text contains the given string.
	[[nodiscard]] Node* findTagContainingText(std::string_view tag, std::string_view text) const;
	
	/// @brief Finds the first descendant (or self) matching the given tag name.
	[[nodiscard]] Node* findNodeWithTag(std::string_view tag) const;
	
	/// @brief Walks up the tree and returns the first ancestor matching the given tag.
	[[nodiscard]] Node* findParentWithTag(std::string_view tag) const;
	
	/// @brief Collects all descendants (and self) matching the given tag name.
	[[nodiscard]] std::vector<Node*> nodesWithTag(std::string_view tag) const;
	
	/// @brief Returns true if this node has an attribute with the given name and value.
	[[nodiscard]] bool hasTextAttr(std::string_view attrName, std::string_view attrValue) const;
	
	/// @brief Finds the first descendant (or self) with a matching attribute name/value.
	[[nodiscard]] Node* findNodeWithTextAttr(std::string_view attrName, std::string_view attrValue) const;
	
	/// @brief Finds the next node in linear traversal order with a matching attribute.
	[[nodiscard]] Node* findNextNodeWithTextAttr(std::string_view attrName, std::string_view attrValue, Node* parentLimit = nullptr) const;
	
	/// @brief Walks up from this node to find an ancestor with a matching attribute.
	[[nodiscard]] Node* findParentNodeWithTextAttr(std::string_view attrName, std::string_view attrValue) const;
	
	/// @brief Finds the lowest common ancestor of this node and the given node.
	[[nodiscard]] Node* findCommonParent(Node* node) const;
	
	/// @brief Returns the depth of this node in the tree (root = 0).
	[[nodiscard]] int getNodeDepth() const noexcept;
	
	/// @brief Returns the next node in a depth-first linear traversal.
	[[nodiscard]] Node* getNextNodeLinear(Node* parentLimit = nullptr) const;
	
	/// @brief Returns the full HTML of this element including its tag.
	[[nodiscard]] std::string outerHTML() const;
	
	/// @brief Returns the HTML content inside this element's tags.
	[[nodiscard]] std::string innerHTML() const;
	
	/// @brief Returns the attribute string for this element's opening tag.
	[[nodiscard]] std::string attributesHTML() const;
	
	/// @brief Returns the opening tag string (e.g. "&lt;div class='x'&gt;").
	[[nodiscard]] std::string openingTag() const;
	
	/// @brief Returns the closing tag string (e.g. "&lt;/div&gt;").
	[[nodiscard]] std::string closingTag() const;
	
	/// @brief Returns all anchor href values from this subtree.
	[[nodiscard]] std::vector<std::string> anchors() const;
	
	/// @brief Returns all anchors (href + visible label text) from this subtree.
	[[nodiscard]] std::vector<AnchorLink> anchorLinks() const;
	
	/// @brief Returns the value of the named attribute, or empty string if not found.
	[[nodiscard]] std::string getProperty(std::string_view propertyName) const;
	
	/// @brief Returns the concatenated text content of this node and its children.
	[[nodiscard]] std::string getText() const;
	
	/// @brief Trims leading and trailing spaces from a string.
	[[nodiscard]] static std::string trimString(std::string_view string);
};

static_assert(sizeof(Node) == sizeof(xmlNode));

///----------------------------------------
/// MARK: Document
///----------------------------------------

///----------------------------------------
/// @brief Owns an HTML document parsed via libxml2's push parser.
///----------------------------------------

class Document final {
	htmlParserCtxtPtr _parser = nullptr;

	/// @brief Frees the parser context and the document it produced, leaving both null.
	/// @details Freeing the context alone leaks the parsed tree; see the definition for why.
	void releaseParser();

public:
	Document() = default;
	~Document();
	Document(const Document&) = delete;
	Document& operator=(const Document&) = delete;
	
	/// @brief Loads and parses HTML from a file path.
	[[nodiscard]] bool loadFile(const std::filesystem::path& path);
	
	/// @brief Parses an HTML string.
	[[nodiscard]] bool loadString(std::string_view string);
	
	/// @brief Returns the root element of the parsed document.
	[[nodiscard]] Node* getRootNode() const {
		return static_cast<Node*>(xmlDocGetRootElement(_parser->myDoc));
	}
	
	/// @brief Finds the first node matching a tag whose text contains the given string.
	[[nodiscard]] Node* findTagContainingText(std::string_view tag, std::string_view text) const {
		return getRootNode()->findTagContainingText(tag, text);
	}
	
	/// @brief Finds the first descendant matching the given tag name.
	[[nodiscard]] Node* findNodeWithTag(std::string_view tag) const {
		return getRootNode()->findNodeWithTag(tag);
	}
	
	/// @brief Finds the first descendant with a matching attribute name/value.
	[[nodiscard]] Node* findNodeWithTextAttr(std::string_view attrName, std::string_view attrValue) const {
		return getRootNode()->findNodeWithTextAttr(attrName, attrValue);
	}
	
	/// @brief Returns all anchor href values from the document.
	[[nodiscard]] std::vector<std::string> anchors() const {
		return getRootNode()->anchors();
	}
	
	/// @brief Prints the document tree for debugging.
	void print(std::string_view prefix) const {
		getRootNode()->print(prefix);
	}
	
	/// @brief Collects all nodes matching the given tag name.
	[[nodiscard]] std::vector<Node*> nodesWithTag(std::string_view tag) const;
	
	/// @brief Sorts a string vector and removes duplicates in place.
	static void sortAndRemoveDuplicates(std::vector<std::string>& strings);
};

///----------------------------------------
/// MARK: Self-Test
///----------------------------------------

///----------------------------------------
/// @brief Exercises HTML parsing and DOM traversal against a tiny, fixed document.
/// @details Registered with the shared runner in @ref SelfTest.h. Parses a paragraph and an anchor, then
///          asserts the text extraction, re-serialization, tag and attribute searches, and anchor
///          collection the public API documents.
///----------------------------------------

inline void htmlSelfTest() {
	using Text::selftest::check;
	// Static trimming needs no parser and drops the surrounding spaces.
	check(Node::trimString("  hi  ") == "hi", "trimString drops surrounding spaces");
	// Parse a tiny document carrying a paragraph and an anchor.
	auto document = Document{};
	check(document.loadString("<html><body><p>Hello</p><a href=\"page.html\">Link</a></body></html>"), "loadString parses a document");
	// The parsed tree has an <html> root element.
	check(document.findNodeWithTag("html") != nullptr, "root element is <html>");
	// The paragraph's text is its trimmed content, and it re-serializes to its original HTML.
	auto* paragraph = document.findNodeWithTag("p");
	check(paragraph != nullptr, "finds the paragraph node");
	check(paragraph->getText() == "Hello", "paragraph text is its content");
	check(paragraph->outerHTML() == "<p>Hello</p>", "paragraph re-serializes to its HTML");
	// A tag/text search locates the paragraph by a substring of its text.
	check(document.findTagContainingText("p", "ell") == paragraph, "locates a tag by contained text");
	// A tag census counts the single paragraph.
	check(document.nodesWithTag("p").size() == 1, "one paragraph in the document");
	// The anchor exposes its href through both a property and an attribute lookup.
	auto* anchor = document.findNodeWithTag("a");
	check(anchor != nullptr, "finds the anchor node");
	check(anchor->getProperty("href") == "page.html", "anchor href property");
	check(anchor->getText() == "Link", "anchor label text");
	check(document.findNodeWithTextAttr("href", "page.html") == anchor, "finds a node by attribute value");
	// The document's anchor collection carries the one href.
	auto hrefs = document.anchors();
	check(hrefs.size() == 1 && hrefs.front() == "page.html", "document anchors list the href");
	// Anchor links pair each href with its visible label text.
	auto links = document.getRootNode()->anchorLinks();
	check(links.size() == 1 && links.front().href == "page.html" && links.front().label == "Link", "anchor links pair href and label");
}
	
} // namespace HTML
