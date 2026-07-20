///----------------------------------------
///      @file HTML.cpp
///   @ingroup TextLib
///     @brief HTML document parser and DOM traversal implementations.
///    @author Created by John Stephen on 5/9/14.
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#include "TextLib/HTML.h"
#include "CoreLib/File/MemoryMappedFile.h"
#include <algorithm>
#include "CoreLib/Log/Log.h"

///----------------------------------------
namespace HTML {
///----------------------------------------

///----------------------------------------
/// MARK: Node
///----------------------------------------

void Node::print(std::string_view prefix) const {
#ifdef DEBUG
	// Print this node's tag and content
	auto nameStr = name ? reinterpret_cast<const char*>(name) : "(null)";
	auto contentStr = content ? reinterpret_cast<const char*>(content) : "(null)";
	LOG_DEBUG("{}TAG: {} content:{}", prefix, nameStr, contentStr);
#endif
	
	// Print attributes
	auto childPrefix = std::string{prefix} + "     " + (name ? reinterpret_cast<const char*>(name) : "(null name)") + ": ";
	for (auto* attr = properties; attr; attr = attr->next) {
		if (attr->children) {
			for (auto* attrNode = attr->children; attrNode; attrNode = attrNode->next) {
				LOG_DEBUG("{}ATTR: {} {} {}", childPrefix,
					reinterpret_cast<const char*>(attr->name),
					reinterpret_cast<const char*>(attrNode->name),
					reinterpret_cast<const char*>(attrNode->content));
			}
		} else {
			LOG_DEBUG("{}ATTR: {} (no children)", childPrefix,
				reinterpret_cast<const char*>(attr->name));
		}
	}
	
	// Print children recursively
	for (auto* childNode = static_cast<Node*>(children); childNode; childNode = static_cast<Node*>(childNode->next)) {
		childNode->print(childPrefix);
	}
}

Node* Node::findTagContainingText(std::string_view tag, std::string_view text) const {
	for (auto* node = const_cast<Node*>(this); node; node = static_cast<Node*>(node->next)) {
		if (tag == reinterpret_cast<const char*>(node->name)) {
			// Check direct content
			if (node->content) {
				auto contentStr = std::string_view{reinterpret_cast<const char*>(node->content)};
				if (contentStr.find(text) != std::string_view::npos) {
					return node;
				}
			}
			
			// Check text child nodes
			for (auto* childNode = node->children; childNode; childNode = childNode->next) {
				if (std::string_view{reinterpret_cast<const char*>(childNode->name)} == "text") {
					auto contentStr = std::string_view{reinterpret_cast<const char*>(childNode->content)};
					if (contentStr.find(text) != std::string_view::npos) {
						return node;
					}
				}
			}
		}
		
		// Recurse into children
		if (node->children) {
			auto* foundNode = static_cast<Node*>(node->children)->findTagContainingText(tag, text);
			if (foundNode) {
				return foundNode;
			}
		}
	}
	return nullptr;
}

Node* Node::findNodeWithTextAttr(std::string_view attrName, std::string_view attrValue) const {
	if (hasTextAttr(attrName, attrValue)) {
		return const_cast<Node*>(this);
	}
	for (auto* childNode = static_cast<Node*>(children); childNode; childNode = static_cast<Node*>(childNode->next)) {
		auto* foundNode = childNode->findNodeWithTextAttr(attrName, attrValue);
		if (foundNode) {
			return foundNode;
		}
	}
	return nullptr;
}

Node* Node::getNextNodeLinear(Node* parentLimit) const {
	if (children) {
		return static_cast<Node*>(children);
	}
	if (next) {
		return static_cast<Node*>(next);
	}
	
	// Walk up parents looking for a sibling
	auto* parentNode = static_cast<Node*>(parent);
	while (parentNode && parentNode != parentLimit) {
		auto* parentNext = static_cast<Node*>(parentNode->next);
		if (parentNext) {
			return parentNext;
		}
		parentNode = static_cast<Node*>(parentNode->parent);
	}
	return nullptr;
}

bool Node::hasTextAttr(std::string_view attrName, std::string_view attrValue) const {
	for (auto* attr = properties; attr; attr = attr->next) {
		if (attr->children) {
			for (auto* attrNode = attr->children; attrNode; attrNode = attrNode->next) {
				if (attrName == reinterpret_cast<const char*>(attr->name)
				&& std::string_view{reinterpret_cast<const char*>(attrNode->name)} == "text"
				&& attrValue == reinterpret_cast<const char*>(attrNode->content)) {
					return true;
				}
			}
		}
	}
	return false;
}

std::string Node::attributesHTML() const {
	auto html = std::string{};
	for (auto* attr = properties; attr; attr = attr->next) {
		if (attr->children) {
			for (auto* attrNode = attr->children; attrNode; attrNode = attrNode->next) {
				if (std::string_view{reinterpret_cast<const char*>(attrNode->name)} == "text") {
					html += " ";
					html += reinterpret_cast<const char*>(attr->name);
					html += "=\"";
					html += reinterpret_cast<const char*>(attrNode->content);
					html += "\"";
				}
			}
		}
	}
	return html;
}

std::string Node::openingTag() const {
	if (type == XML_CDATA_SECTION_NODE) {
		return "";
	}
	if (std::string_view{reinterpret_cast<const char*>(name)} == "text") {
		return "";
	}
	
	auto tag = std::string{"<"};
	tag += reinterpret_cast<const char*>(name);
	tag += attributesHTML();
	tag += children ? ">" : "/>";
	return tag;
}

std::string Node::closingTag() const {
	if (!children) {
		return "";
	}
	return std::string{"</"} + reinterpret_cast<const char*>(name) + ">";
}

std::string Node::outerHTML() const {
	return openingTag() + innerHTML() + closingTag();
}

std::string Node::innerHTML() const {
	auto html = std::string{};
	if (content) {
		html = reinterpret_cast<const char*>(content);
	}
	if (children) {
		for (auto* childNode = static_cast<Node*>(children); childNode; childNode = static_cast<Node*>(childNode->next)) {
			html += childNode->outerHTML();
		}
	}
	return html;
}

Node* Node::findNextNodeWithTextAttr(std::string_view attrName, std::string_view attrValue, Node* parentLimit) const {
	auto* nextNode = getNextNodeLinear(parentLimit);
	while (nextNode) {
		if (nextNode->hasTextAttr(attrName, attrValue)) {
			return nextNode;
		}
		nextNode = nextNode->getNextNodeLinear(parentLimit);
	}
	return nullptr;
}

Node* Node::findParentNodeWithTextAttr(std::string_view attrName, std::string_view attrValue) const {
	if (hasTextAttr(attrName, attrValue)) {
		return const_cast<Node*>(this);
	}
	if (parent) {
		return static_cast<Node*>(parent)->findParentNodeWithTextAttr(attrName, attrValue);
	}
	return nullptr;
}

Node* Node::findNodeWithTag(std::string_view tag) const {
	if (name && tag == reinterpret_cast<const char*>(name)) {
		return const_cast<Node*>(this);
	}
	if (!children) {
		return nullptr;
	}
	for (auto* child = static_cast<Node*>(children); child; child = static_cast<Node*>(child->next)) {
		auto* found = child->findNodeWithTag(tag);
		if (found) {
			return found;
		}
	}
	return nullptr;
}

std::vector<Node*> Node::nodesWithTag(std::string_view tag) const {
	auto nodes = std::vector<Node*>{};
	
	if (name && tag == reinterpret_cast<const char*>(name)) {
		nodes.push_back(const_cast<Node*>(this));
	}
	
	if (children) {
		for (auto* child = static_cast<Node*>(children); child; child = static_cast<Node*>(child->next)) {
			auto childNodes = child->nodesWithTag(tag);
			if (!childNodes.empty()) {
				nodes.insert(nodes.end(), childNodes.begin(), childNodes.end());
			}
		}
	}
	
	return nodes;
}

Node* Node::findCommonParent(Node* node) const {
	if (!node) {
		return nullptr;
	}
	
	// Start from both nodes and equalize depths
	auto* parent1 = const_cast<Node*>(this);
	auto* parent2 = node;
	auto depth1 = parent1->getNodeDepth();
	auto depth2 = parent2->getNodeDepth();
	
	while (depth1 > depth2) {
		parent1 = static_cast<Node*>(parent1->parent);
		--depth1;
	}
	
	while (depth2 > depth1) {
		parent2 = static_cast<Node*>(parent2->parent);
		--depth2;
	}
	
	// Walk up together until common ancestor found
	while (parent1 && parent1 != parent2) {
		parent1 = static_cast<Node*>(parent1->parent);
		parent2 = static_cast<Node*>(parent2->parent);
	}
	return parent1;
}

int Node::getNodeDepth() const noexcept {
	auto depth = 0;
	auto* node = parent;
	while (node) {
		++depth;
		node = node->parent;
	}
	return depth;
}

Node* Node::findParentWithTag(std::string_view tag) const {
	if (!parent) {
		return nullptr;
	}
	if (tag == reinterpret_cast<const char*>(parent->name)) {
		return static_cast<Node*>(parent);
	}
	return static_cast<Node*>(parent)->findParentWithTag(tag);
}

std::vector<std::string> Node::anchors() const {
	std::vector<std::string> result;
	for (auto* childNode = static_cast<Node*>(children); childNode; childNode = static_cast<Node*>(childNode->next)) {
		if (std::string_view{reinterpret_cast<const char*>(childNode->name)} == "a") {
			for (auto* attr = childNode->properties; attr; attr = attr->next) {
				if (std::string_view{reinterpret_cast<const char*>(attr->name)} == "href") {
					for (auto* attrNode = attr->children; attrNode; attrNode = attrNode->next) {
						if (std::string_view{reinterpret_cast<const char*>(attrNode->name)} == "text") {
							result.emplace_back(reinterpret_cast<const char*>(attrNode->content));
						}
					}
				}
			}
		}
		if (childNode->children) {
			auto childAnchors = childNode->anchors();
			result.insert(result.end(), childAnchors.begin(), childAnchors.end());
		}
	}
	return result;
}

std::vector<AnchorLink> Node::anchorLinks() const {
	std::vector<AnchorLink> result;
	for (auto* childNode = static_cast<Node*>(children); childNode; childNode = static_cast<Node*>(childNode->next)) {
		if (std::string_view{reinterpret_cast<const char*>(childNode->name)} == "a") {
			for (auto* attr = childNode->properties; attr; attr = attr->next) {
				if (std::string_view{reinterpret_cast<const char*>(attr->name)} == "href") {
					for (auto* attrNode = attr->children; attrNode; attrNode = attrNode->next) {
						if (std::string_view{reinterpret_cast<const char*>(attrNode->name)} == "text") {
							result.push_back({reinterpret_cast<const char*>(attrNode->content), childNode->getText()});
						}
					}
				}
			}
		}
		if (childNode->children) {
			auto childLinks = childNode->anchorLinks();
			result.insert(result.end(), childLinks.begin(), childLinks.end());
		}
	}
	return result;
}

std::string Node::getProperty(std::string_view propertyName) const {
	if (!properties) {
		return "";
	}
	for (auto* attr = properties; attr; attr = attr->next) {
		if (propertyName == reinterpret_cast<const char*>(attr->name)) {
			for (auto* attrNode = attr->children; attrNode; attrNode = attrNode->next) {
				if (std::string_view{reinterpret_cast<const char*>(attrNode->name)} == "text") {
					return reinterpret_cast<const char*>(attrNode->content);
				}
			}
		}
	}
	return "";
}

std::string Node::getText() const {
	auto text = std::string{};
	if (content) {
		text = reinterpret_cast<const char*>(content);
	}
	if (children) {
		for (auto* child = static_cast<Node*>(children); child; child = static_cast<Node*>(child->next)) {
			if (!text.empty()) {
				text += child->getText();
			} else {
				text = child->getText();
			}
		}
	}
	return trimString(text);
}

std::string Node::trimString(std::string_view string) {
	auto start = string.find_first_not_of(' ');
	if (start == std::string_view::npos) {
		return "";
	}
	auto end = string.find_last_not_of(' ');
	return std::string{string.substr(start, end - start + 1)};
}

///----------------------------------------
/// MARK: Document
///----------------------------------------

Document::~Document() {
	if (_parser) {
		htmlFreeParserCtxt(_parser);
	}
}

bool Document::loadFile(const std::filesystem::path& path) {
	// Free previous parser
	if (_parser) {
		htmlFreeParserCtxt(_parser);
		_parser = nullptr;
	}
	
	// Open the file as a memory map
	auto file = Core::MemoryMappedFile::open(path);
	if (!file) {
		return false;
	}
	auto bytes = file->bytes();
	
	// Parse
	_parser = htmlCreatePushParserCtxt(nullptr, nullptr, nullptr, 0, nullptr, XML_CHAR_ENCODING_UTF8);
	htmlCtxtUseOptions(_parser, HTML_PARSE_RECOVER | HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
	htmlParseChunk(_parser, reinterpret_cast<const char*>(bytes.data()), int(bytes.size()), 1);
	return true;
}

bool Document::loadString(std::string_view string) {
	// Free previous parser
	if (_parser) {
		htmlFreeParserCtxt(_parser);
		_parser = nullptr;
	}
	
	// Parse
	_parser = htmlCreatePushParserCtxt(nullptr, nullptr, nullptr, 0, nullptr, XML_CHAR_ENCODING_UTF8);
	htmlCtxtUseOptions(_parser, HTML_PARSE_RECOVER | HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
	htmlParseChunk(_parser, string.data(), string.size(), 1);
	return true;
}

void Document::sortAndRemoveDuplicates(std::vector<std::string>& strings) {
	std::ranges::sort(strings);
	auto duplicates = std::ranges::unique(strings);
	strings.erase(duplicates.begin(), duplicates.end());
}

std::vector<Node*> Document::nodesWithTag(std::string_view tag) const {
	return getRootNode()->nodesWithTag(tag);
}
	
} // namespace HTML
