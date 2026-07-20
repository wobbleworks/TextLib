///----------------------------------------
///      @file DelimitedText.h
///   @ingroup TextLib
///     @brief Header-only reader and writer for delimited text files (TSV, CSV).
///   @details A @ref Text::DelimitedDialect names the separator and conventions (quoted fields, space
///            trimming, control-character filtering); @ref Text::tsvDialect and @ref Text::csvDialect
///            fix the two file formats byte-for-byte, including the backslash escapes
///            (@c \\n, @c \\t, @c \\\\) both formats share and the @c # comment convention.
///
///            Reading is a single range-for over the file — comment rows appear in the iteration and
///            answer @ref DelimitedRow::isComment. Each row
///            carries a @ref RowMarker so large files can be indexed once and individual rows re-read
///            lazily via @ref DelimitedReader::rowAt. Writing goes through @ref DelimitedWriter, whose
///            @c writeRow returns the row's marker.
///    @author Created by John Stephen on 7/2/26.
/// @copyright Copyright © 2026 John Stephen (wobbleworks.com)
///            Licensed under the Apache License, Version 2.0.
///            SPDX-License-Identifier: Apache-2.0
///----------------------------------------

#pragma once

#include "TextLib/SelfTestCheck.h"

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

///----------------------------------------
namespace Text {
///----------------------------------------

///----------------------------------------
/// MARK: Dialects
///----------------------------------------

///----------------------------------------
///  @struct DelimitedDialect
///   @brief Names a delimited-text convention: the separator and how fields are quoted and cleaned.
/// @details Use @ref tsvDialect or @ref csvDialect for the two shipped formats; the members exist so a
///          variant convention can be described without a new reader.
///----------------------------------------

struct DelimitedDialect {
	/// @brief Field separator character.
	char separator = '\t';
	
	/// @brief Whether a field may be wrapped in double quotes (CSV convention).
	bool quotesFields = false;
	
	/// @brief Whether spaces around unquoted fields are trimmed (CSV convention).
	bool trimsSpaces = false;
	
	/// @brief Whether lines containing control characters (other than the separator) are skipped.
	bool skipsControlLines = false;
};

/// @brief Tab-separated values: backslash escapes, no quoting, control-character lines skipped.
inline constexpr DelimitedDialect tsvDialect{'\t', false, false, true};

/// @brief Comma-separated values: double-quoted fields, spaces trimmed, backslash escapes.
inline constexpr DelimitedDialect csvDialect{',', true, true, false};

///----------------------------------------
/// MARK: Escaping
///----------------------------------------

///----------------------------------------
///   @brief Escapes newline, tab, and backslash for storage in a delimited field.
/// @details The escaping shared by the TSV and CSV dialects — @c \\n, @c \\t, @c \\\\ — a stable
///          on-disk contract.
///----------------------------------------

[[nodiscard]] inline std::string escapeDelimitedField(std::string_view text) {
	auto escaped = std::string();
	escaped.reserve(2 * text.length());
	
	auto startPos = std::string_view::size_type{0};
	auto endPos = std::string_view::size_type{};
	while ((endPos = text.find_first_of("\n\t\\", startPos)) != std::string_view::npos) {
		if (endPos != startPos) {
			escaped.append(text.substr(startPos, endPos - startPos));
		}
		switch (text[endPos++]) {
			case '\n':
				escaped.append("\\n");
				break;
			case '\t':
				escaped.append("\\t");
				break;
			case '\\':
				escaped.append("\\\\");
				break;
			default:
				assert(false);
				break;
		}
		startPos = endPos;
	}
	
	escaped.append(text.substr(startPos));
	return escaped;
}

///----------------------------------------
///   @brief Reverses @ref escapeDelimitedField.
/// @details Unrecognized escapes are dropped and a trailing lone backslash is kept — fixed decoding
///          behavior that files already on disk rely on.
///----------------------------------------

[[nodiscard]] inline std::string unescapeDelimitedField(std::string_view escaped) {
	auto text = std::string();
	text.reserve(escaped.length());
	
	auto startPos = std::string_view::size_type{0};
	auto endPos = std::string_view::size_type{};
	while ((endPos = escaped.find('\\', startPos)) != std::string_view::npos) {
		// A trailing lone backslash passes through with the remainder
		if (endPos + 1 == escaped.length()) {
			break;
		}
		
		if (endPos != startPos) {
			text.append(escaped.substr(startPos, endPos - startPos));
		}
		
		switch (escaped[++endPos]) {
			case 'n':
				text.append("\n");
				break;
			case 't':
				text.append("\t");
				break;
			case '\\':
				text.append("\\");
				break;
			default:
				break;
		}
		
		startPos = endPos + 1;
	}
	
	text.append(escaped.substr(startPos));
	return text;
}

///----------------------------------------
/// MARK: RowMarker
///----------------------------------------

///----------------------------------------
/// @struct RowMarker
///  @brief A row's position within its file, for re-reading it later without a full scan.
///----------------------------------------

struct RowMarker {
	uint64_t offset = 0;
	uint32_t length = 0;
	uint32_t lineNumber = 0;
	
	/// @brief True when the marker points at a recorded row.
	[[nodiscard]] bool valid() const noexcept {
		return length != 0;
	}
};

///----------------------------------------
/// MARK: FieldIndex
///----------------------------------------

///----------------------------------------
///  @class FieldIndex
///   @brief Maps header field names to column indices.
/// @details Build one from the header row's fields, then resolve columns by name for the data rows.
///----------------------------------------

class FieldIndex final {
///----------------------------------------
	std::map<std::string, int, std::less<>> _columns;
	
public:
	FieldIndex() = default;
	
	///----------------------------------------
	/// @brief Builds the index from field names in column order.
	///----------------------------------------
	
	explicit FieldIndex(std::span<const std::string> names) {
		auto column = 0;
		for (const auto& name : names) {
			_columns[name] = column++;
		}
	}
	
	///----------------------------------------
	/// @brief Column index of @a name, or -1 when the header has no such field.
	///----------------------------------------
	
	[[nodiscard]] int indexOf(std::string_view name) const {
		auto found = _columns.find(name);
		return found == _columns.end() ? -1 : found->second;
	}
	
	///----------------------------------------
	/// @brief Column index of @a name, or @a defaultIndex when the header has no such field.
	///----------------------------------------
	
	[[nodiscard]] int indexOf(std::string_view name, int defaultIndex) const {
		auto found = _columns.find(name);
		return found == _columns.end() ? defaultIndex : found->second;
	}
	
	///----------------------------------------
	///   @brief Column index of @a name, for a field the header is required to have.
	/// @details Asserts when the field is missing and falls back to column 0, so a schema mismatch is
	///          loud in debug builds without derailing release parsing of known-good data.
	///----------------------------------------
	
	[[nodiscard]] int requiredIndexOf(std::string_view name) const {
		auto found = _columns.find(name);
		if (found == _columns.end()) {
			assert(false);
			return 0;
		}
		return found->second;
	}
	
	///----------------------------------------
	/// @brief True when the header has a field called @a name.
	///----------------------------------------
	
	[[nodiscard]] bool contains(std::string_view name) const {
		return _columns.find(name) != _columns.end();
	}
	
	///----------------------------------------
	/// @brief True when no header has been indexed yet.
	///----------------------------------------
	
	[[nodiscard]] bool empty() const noexcept {
		return _columns.empty();
	}
	
	///----------------------------------------
	/// @brief Number of indexed fields.
	///----------------------------------------
	
	[[nodiscard]] size_t size() const noexcept {
		return _columns.size();
	}
	
	///----------------------------------------
	/// @brief Iteration over the (name, column) pairs, in name order.
	///----------------------------------------
	
	[[nodiscard]] auto begin() const noexcept {
		return _columns.begin();
	}
	
	[[nodiscard]] auto end() const noexcept {
		return _columns.end();
	}
};

///----------------------------------------
/// MARK: DelimitedRow
///----------------------------------------

///----------------------------------------
///  @class DelimitedRow
///   @brief One row of a delimited file: its unescaped fields, raw text, and file position.
/// @details Comment rows (lines starting with @c #) appear in the iteration too; test @ref isComment
///          before reading fields.
///----------------------------------------

class DelimitedRow final {
///----------------------------------------
	friend class DelimitedReader;
	
	std::string _text;
	std::vector<std::string> _fields;
	RowMarker _marker;
	bool _isComment = false;
	
public:
	///----------------------------------------
	/// @brief True for a comment line; @ref commentText carries the text after the @c #.
	///----------------------------------------
	
	[[nodiscard]] bool isComment() const noexcept {
		return _isComment;
	}
	
	///----------------------------------------
	/// @brief The comment text following the leading @c #; empty for data rows.
	///----------------------------------------
	
	[[nodiscard]] std::string_view commentText() const noexcept {
		return _isComment ? std::string_view(_text).substr(1) : std::string_view();
	}
	
	///----------------------------------------
	/// @brief Number of fields in the row.
	///----------------------------------------
	
	[[nodiscard]] size_t size() const noexcept {
		return _fields.size();
	}
	
	///----------------------------------------
	/// @brief The unescaped fields, in column order.
	///----------------------------------------
	
	[[nodiscard]] const std::vector<std::string>& fields() const noexcept {
		return _fields;
	}
	
	///----------------------------------------
	/// @brief Field @a index; asserts when out of range.
	///----------------------------------------
	
	[[nodiscard]] const std::string& operator[](size_t index) const {
		assert(index < _fields.size());
		return _fields[index];
	}
	
	///----------------------------------------
	/// @brief Field @a index, or empty when the index is out of range (including -1 from @ref FieldIndex).
	///----------------------------------------
	
	[[nodiscard]] std::string_view field(int index) const noexcept {
		if (index < 0 || size_t(index) >= _fields.size()) {
			return {};
		}
		return _fields[size_t(index)];
	}
	
	///----------------------------------------
	/// @brief Field @a index parsed as a double; NaN when missing or empty.
	///----------------------------------------
	
	[[nodiscard]] double doubleField(int index) const {
		auto value = field(index);
		if (value.empty()) {
			return std::numeric_limits<double>::quiet_NaN();
		}
		return std::strtod(std::string(value).c_str(), nullptr);
	}
	
	///----------------------------------------
	/// @brief The raw line text (escaped, without the line terminator).
	///----------------------------------------
	
	[[nodiscard]] const std::string& text() const noexcept {
		return _text;
	}
	
	///----------------------------------------
	/// @brief The row's position in the file, for @ref DelimitedReader::rowAt.
	///----------------------------------------
	
	[[nodiscard]] const RowMarker& marker() const noexcept {
		return _marker;
	}
};

///----------------------------------------
/// MARK: DelimitedReader
///----------------------------------------

///----------------------------------------
///  @class DelimitedReader
///   @brief Iterates the rows of a delimited text file.
/// @details Open with @ref open, then range-for over the reader; each element is a @ref DelimitedRow
///          (data or comment). Iteration is single-pass — call @ref begin once. Blank lines are
///          skipped, trailing carriage returns are stripped, and (per the dialect) lines containing
///          stray control characters are ignored. A malformed row (an unclosed CSV quote) stops the
///          iteration and sets @ref failed. @ref rowAt re-reads one row by its marker without scanning.
///----------------------------------------

class DelimitedReader final {
///----------------------------------------
	std::filesystem::path _path;
	DelimitedDialect _dialect;
	std::ifstream _stream;
	DelimitedRow _row;
	uint32_t _nextLineNumber = 0;
	bool _done = false;
	bool _failed = false;
	
public:
	///----------------------------------------
	///   @brief Opens @a path for iteration.
	/// @returns The reader, or a `std::error_code` when the file can't be opened.
	///----------------------------------------
	
	[[nodiscard]] static std::expected<DelimitedReader, std::error_code> open(const std::filesystem::path& path, const DelimitedDialect& dialect) {
		auto reader = DelimitedReader();
		reader._path = path;
		reader._dialect = dialect;
		reader._stream.open(path, std::ios::in | std::ios::binary);
		if (!reader._stream) {
			return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
		}
		return reader;
	}
	
	DelimitedReader(DelimitedReader&&) = default;
	DelimitedReader& operator=(DelimitedReader&&) = default;
	
	///----------------------------------------
	///   @brief Splits and unescapes one line of text into fields.
	/// @returns The fields, or `std::nullopt` for a malformed line (an unclosed quote).
	///----------------------------------------
	
	[[nodiscard]] static std::optional<std::vector<std::string>> parseFields(std::string_view text, const DelimitedDialect& dialect) {
		auto fields = std::vector<std::string>();
		
		// Quoting dialects need field-by-field scanning
		if (dialect.quotesFields) {
			if (!parseQuotedFields(text, dialect, fields)) {
				return std::nullopt;
			}
			return fields;
		}
		
		// Plain dialects split on the separator and unescape each field
		auto startPos = std::string_view::size_type{0};
		while (startPos <= text.length()) {
			auto endPos = text.find(dialect.separator, startPos);
			auto escaped = text.substr(startPos, endPos == std::string_view::npos ? endPos : endPos - startPos);
			fields.push_back(unescapeDelimitedField(escaped));
			if (endPos == std::string_view::npos) {
				break;
			}
			startPos = endPos + 1;
		}
		return fields;
	}
	
	///----------------------------------------
	///   @brief Re-reads the row recorded by @a marker.
	/// @returns The parsed row, or `std::nullopt` when the marker or file is invalid.
	///----------------------------------------
	
	[[nodiscard]] std::optional<DelimitedRow> rowAt(const RowMarker& marker) const {
		if (!marker.valid()) {
			assert(false);
			return std::nullopt;
		}
		
		// Read the recorded byte range
		auto stream = std::ifstream(_path, std::ios::in | std::ios::binary);
		if (!stream) {
			return std::nullopt;
		}
		auto row = DelimitedRow();
		row._text.resize(marker.length);
		stream.seekg(std::streamoff(marker.offset));
		stream.read(row._text.data(), marker.length);
		if (!stream) {
			return std::nullopt;
		}
		
		// Parse it
		auto fields = DelimitedReader::parseFields(row._text, _dialect);
		if (!fields) {
			return std::nullopt;
		}
		row._fields = std::move(*fields);
		row._marker = marker;
		return row;
	}
	
	///----------------------------------------
	/// @brief True when iteration stopped on a malformed row rather than the end of the file.
	///----------------------------------------
	
	[[nodiscard]] bool failed() const noexcept {
		return _failed;
	}
	
	///----------------------------------------
	///  @class iterator
	///  @brief Single-pass input iterator over the reader's rows.
	///----------------------------------------
	
	class iterator {
	///----------------------------------------
		DelimitedReader* _reader = nullptr;
		
	public:
		using value_type = DelimitedRow;
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::input_iterator_tag;
		
		iterator() = default;
		explicit iterator(DelimitedReader* reader) : _reader(reader) {}
		
		[[nodiscard]] const DelimitedRow& operator*() const {
			return _reader->_row;
		}
		
		[[nodiscard]] const DelimitedRow* operator->() const {
			return &_reader->_row;
		}
		
		iterator& operator++() {
			_reader->advance();
			return *this;
		}
		
		void operator++(int) {
			++(*this);
		}
		
		[[nodiscard]] bool operator==(std::default_sentinel_t) const {
			return _reader == nullptr || _reader->_done;
		}
	};
	
	///----------------------------------------
	/// @brief Starts the single-pass iteration; call once per reader.
	///----------------------------------------
	
	[[nodiscard]] iterator begin() {
		advance();
		return iterator(this);
	}
	
	///----------------------------------------
	/// @brief The end-of-iteration sentinel.
	///----------------------------------------
	
	[[nodiscard]] std::default_sentinel_t end() const noexcept {
		return {};
	}
	
private:
	DelimitedReader() = default;
	
	///----------------------------------------
	/// @brief Reads forward to the next reportable row (data or comment), skipping blank and junk lines.
	///----------------------------------------
	
	void advance() {
		while (true) {
			// Record where this line starts
			auto lineStart = uint64_t(_stream.tellg());
			if (!std::getline(_stream, _row._text)) {
				_done = true;
				return;
			}
			auto lineNumber = _nextLineNumber++;
			
			// Strip a trailing carriage return
			if (!_row._text.empty() && _row._text.back() == '\r') {
				_row._text.pop_back();
			}
			
			// Skip blank lines
			if (_row._text.empty()) {
				continue;
			}
			
			// Skip lines with stray control characters, when the dialect filters them
			if (_dialect.skipsControlLines && hasStrayControlCharacter(_row._text, _dialect.separator)) {
				continue;
			}
			
			// Fill in the marker
			_row._marker.offset = lineStart;
			_row._marker.length = uint32_t(_row._text.length());
			_row._marker.lineNumber = lineNumber;
			
			// Report a comment row
			if (_row._text.front() == '#') {
				_row._isComment = true;
				_row._fields.clear();
				return;
			}
			
			// Parse a data row; a malformed one ends the iteration as a failure
			_row._isComment = false;
			auto fields = DelimitedReader::parseFields(_row._text, _dialect);
			if (!fields) {
				_failed = true;
				_done = true;
				return;
			}
			_row._fields = std::move(*fields);
			return;
		}
	}
	
	///----------------------------------------
	/// @brief True when @a text contains a control character other than @a separator.
	///----------------------------------------
	
	[[nodiscard]] static bool hasStrayControlCharacter(std::string_view text, char separator) {
		for (auto c : text) {
			if (std::iscntrl(static_cast<unsigned char>(c)) && c != separator) {
				return true;
			}
		}
		return false;
	}
	
	///----------------------------------------
	/// @brief Parses a line of a quoting dialect (CSV): quoted or plain fields, spaces trimmed.
	///----------------------------------------
	
	[[nodiscard]] static bool parseQuotedFields(std::string_view text, const DelimitedDialect& dialect, std::vector<std::string>& fields) {
		constexpr auto quote = '"';
		auto length = text.length();
		for (auto startPos = std::string_view::size_type{0}; startPos < length; ) {
			// Skip leading spaces
			if (dialect.trimsSpaces) {
				startPos = text.find_first_not_of(' ', startPos);
				if (startPos == std::string_view::npos) {
					break;
				}
			}
			
			auto endPos = std::string_view::size_type{};
			if (text[startPos] == quote) {
				// A quoted field runs to the closing quote; missing one is malformed
				++startPos;
				endPos = text.find(quote, startPos);
				if (endPos == std::string_view::npos) {
					assert(false);
					return false;
				}
				fields.push_back(unescapeDelimitedField(text.substr(startPos, endPos - startPos)));
				
				// Continue after the separator following the closing quote
				endPos = text.find(dialect.separator, endPos);
			} else {
				// A plain field runs to the separator, with surrounding spaces trimmed
				endPos = text.find(dialect.separator, startPos);
				auto field = text.substr(startPos, endPos == std::string_view::npos ? endPos : endPos - startPos);
				if (dialect.trimsSpaces) {
					auto tail = field.find_last_not_of(' ');
					field = tail == std::string_view::npos ? std::string_view() : field.substr(0, tail + 1);
				}
				fields.push_back(unescapeDelimitedField(field));
			}
			
			startPos = endPos == std::string_view::npos ? endPos : endPos + 1;
		}
		return true;
	}
};

///----------------------------------------
/// MARK: DelimitedWriter
///----------------------------------------

///----------------------------------------
///  @class DelimitedWriter
///   @brief Writes rows to a delimited text file, escaping fields per the dialect.
/// @details Create with @ref create, append rows, then @ref close (which reports any deferred stream
///          error). @c writeRow returns the row's @ref RowMarker so an index can be built while
///          writing. Fields containing raw control characters other than newline or tab are rejected.
///----------------------------------------

class DelimitedWriter final {
///----------------------------------------
	std::ofstream _stream;
	DelimitedDialect _dialect;
	uint32_t _nextLineNumber = 0;
	
public:
	///----------------------------------------
	///   @brief Creates (or truncates) @a path for writing.
	/// @returns The writer, or a `std::error_code` when the file can't be created.
	///----------------------------------------
	
	[[nodiscard]] static std::expected<DelimitedWriter, std::error_code> create(const std::filesystem::path& path, const DelimitedDialect& dialect) {
		auto writer = DelimitedWriter();
		writer._dialect = dialect;
		writer._stream.open(path, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!writer._stream) {
			return std::unexpected(std::make_error_code(std::errc::permission_denied));
		}
		return writer;
	}
	
	DelimitedWriter(DelimitedWriter&&) = default;
	DelimitedWriter& operator=(DelimitedWriter&&) = default;
	
	///----------------------------------------
	///   @brief Writes one row of fields, escaped per the dialect.
	/// @returns The row's marker, or `std::nullopt` when a field holds a raw control character (other
	///          than newline or tab, which the escaping encodes) or the stream has failed.
	///----------------------------------------
	
	[[nodiscard]] std::optional<RowMarker> writeRow(std::span<const std::string> fields) {
		return writeFields(fields.begin(), fields.end());
	}
	
	///----------------------------------------
	/// @brief Convenience overload for a braced list of fields.
	///----------------------------------------
	
	[[nodiscard]] std::optional<RowMarker> writeRow(std::initializer_list<std::string_view> fields) {
		return writeFields(fields.begin(), fields.end());
	}
	
	///----------------------------------------
	/// @brief Writes a comment line, prefixed with @c #.
	///----------------------------------------
	
	void writeComment(std::string_view comment) {
		_stream << '#' << comment << '\n';
		++_nextLineNumber;
	}
	
	///----------------------------------------
	/// @brief Writes a raw line verbatim, with no escaping.
	///----------------------------------------
	
	void writeRaw(std::string_view line) {
		_stream << line << '\n';
		++_nextLineNumber;
	}
	
	///----------------------------------------
	///   @brief Flushes and closes the file.
	/// @returns Nothing on success, or a `std::error_code` when any write failed.
	///----------------------------------------
	
	[[nodiscard]] std::expected<void, std::error_code> close() {
		_stream.close();
		if (_stream.fail()) {
			return std::unexpected(std::make_error_code(std::errc::io_error));
		}
		return {};
	}
	
private:
	DelimitedWriter() = default;
	
	template <class Iterator>
	[[nodiscard]] std::optional<RowMarker> writeFields(Iterator first, Iterator last) {
		if (!_stream) {
			assert(false);
			return std::nullopt;
		}
		
		// Record where the row starts
		auto marker = RowMarker();
		marker.offset = uint64_t(_stream.tellp());
		marker.lineNumber = _nextLineNumber;
		
		// Write each field, escaped, rejecting raw control characters the escaping can't encode
		auto first_field = true;
		for (auto field = first; field != last; ++field) {
			auto text = std::string_view(*field);
			for (auto c : text) {
				if (std::iscntrl(static_cast<unsigned char>(c)) && c != '\n' && c != '\t') {
					assert(false);
					return std::nullopt;
				}
			}
			
			if (!first_field) {
				_stream << _dialect.separator;
			}
			first_field = false;
			_stream << escapeDelimitedField(text);
		}
		
		// Finish the marker before the terminator
		marker.length = uint32_t(uint64_t(_stream.tellp()) - marker.offset);
		_stream << '\n';
		++_nextLineNumber;
		
		return _stream ? std::optional(marker) : std::nullopt;
	}
};

///----------------------------------------
/// @brief Exercises the field escaping, line parsing, and header-index helpers.
///----------------------------------------

inline void delimitedTextSelfTest() {
	using Text::selftest::check;
	// Each special byte escapes to its two-character backslash form.
	check(escapeDelimitedField("\n") == "\\n", "escapes newline");
	check(escapeDelimitedField("\t") == "\\t", "escapes tab");
	check(escapeDelimitedField("\\") == "\\\\", "escapes backslash");
	// Escaping and unescaping round-trip text carrying newline, tab, and backslash.
	auto raw = std::string("a\nb\tc\\d");
	check(unescapeDelimitedField(escapeDelimitedField(raw)) == raw, "escape round-trips embedded control characters");
	// A trailing lone backslash is preserved rather than consumed.
	check(unescapeDelimitedField("abc\\") == "abc\\", "trailing lone backslash passes through");
	// TSV splits on tabs and unescapes each field.
	auto tsv = DelimitedReader::parseFields("a\tb\tc", tsvDialect);
	check(tsv.has_value() && tsv->size() == 3 && (*tsv)[0] == "a" && (*tsv)[2] == "c", "TSV splits on tabs");
	auto tsvEscaped = DelimitedReader::parseFields("x\\ty", tsvDialect);
	check(tsvEscaped.has_value() && tsvEscaped->size() == 1 && (*tsvEscaped)[0] == "x\ty", "TSV unescapes a field");
	// CSV trims spaces around plain fields and keeps a separator inside quotes.
	auto csv = DelimitedReader::parseFields("  hello , \"wo,rld\" ", csvDialect);
	check(csv.has_value() && csv->size() == 2, "CSV splits into two fields");
	check(csv.has_value() && (*csv)[0] == "hello", "CSV trims spaces around a field");
	check(csv.has_value() && (*csv)[1] == "wo,rld", "CSV keeps a separator inside quotes");
	// A field index resolves header names to their column order.
	auto names = std::vector<std::string>{"id", "name", "value"};
	auto index = FieldIndex(std::span<const std::string>(names));
	check(index.size() == 3 && !index.empty(), "field index counts columns");
	check(index.indexOf("name") == 1, "field index resolves a column by name");
	check(index.indexOf("missing") == -1, "field index reports an absent column");
	check(index.contains("value") && !index.contains("nope"), "field index membership test");
	check(index.requiredIndexOf("id") == 0, "field index required lookup");
	check(FieldIndex().empty(), "default field index is empty");
}
	
} // namespace Text
