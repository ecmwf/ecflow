/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Str_HPP
#define ecflow_core_Str_HPP

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "ecflow/core/Converter.hpp"

namespace ecf {

namespace string_constants {

inline const std::string child_cmd  = "chd:";
inline const std::string user_cmd   = "--";
inline const std::string server_cmd = "svr:"; // Only for automatic check_pt

inline const std::string empty = "";

inline const std::string root_path      = "/";
inline const std::string path_separator = "/";
inline const std::string colon          = ":";

inline const std::string task    = "TASK";
inline const std::string family  = "FAMILY";
inline const std::string family1 = "FAMILY1";
inline const std::string suite   = "SUITE";
inline const std::string alias   = "ALIAS";

inline const std::string default_port_number = "3141";
inline const std::string localhost           = "localhost";

inline const std::string white_list_file = "ecf.lists";

inline const std::string alphanumeric_underscore_chars =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

inline const std::string numeric_chars = "0123456789";

} // namespace string_constants

namespace algorithm {

///
/// @brief Join a sequence of strings into a single string with a separator between them. The (optional) separator
/// defaults to ", " if not provided.
///
/// @tparam Sequence1 A container type that supports iteration and whose elements can be converted to std::string_view.
/// @tparam Sequence2 A string-like type.
///
/// @param strings The sequence of string-like objects to join.
/// @param separator The string-like object used as the separator between joined strings.
/// @return The joined string.
///
template <typename Sequence1, typename Sequence2 = std::string>
inline std::string join(const Sequence1& strings, const Sequence2& separator = ", ") {
    std::string buffer;
    auto first = true;
    for (const auto& s : strings) {
        if (!first) {
            buffer.append(std::data(separator), std::size(separator));
        }
        buffer.append(s);
        first = false;
    }
    return buffer;
}

///
/// @brief Replace the first occurrence of a search string in the input sequence with a replacement string.
///
/// No replacement is performed if the search string is not found.
///
/// @tparam Sequence A string-like type.
/// @tparam SearchSequence A string-like type.
/// @tparam ReplaceSequence A string-like type.
///
/// @param input The input sequence to search and replace within.
/// @param search The string to search for.
/// @param replace The string to replace the search string with.
///
template <typename Sequence, typename SearchSequence, typename ReplaceSequence>
inline void replace_first(Sequence& input, const SearchSequence& search, const ReplaceSequence& replace) {
    auto found = input.find(search);
    if (found != std::string::npos) {
        input.replace(found, std::string_view(search).size(), replace);
    }
}

///
/// @brief Splits an input string into a sequence of substrings based on a set of separators.
///
/// The resulting buffer is cleared of any contents, before being populated with the substrings.
/// Consecutive separators are treated as a single separator, meaning no empty substrings are produced.
///
/// Splitting the input `"a,,b,c;d; ;e;;f;"` with the separator `",;"` would result
/// in a buffer containing `["a", "b", "c", "d", " ", "e", "f"]`.
/// It is important to notice that:
///  1) the separators define a set of single characters, and thus duplications are treated as a single separator.
///  2) consecutive separators are collapsed into one, so no empty substrings are produced.
///  3) leading and trailing separators are ignored.
///
/// If the input string is empty, or contains only separator characters, the result be ane empty sequence (i.e. `[ ]`).
///
/// @tparam ResultSequence A container type to hold the resulting substrings.
/// @tparam Sequence1 A string-like type representing the input string.
/// @tparam Sequence2 A string-like type representing single character separators.
///
/// @param buffer A reference to the container where the substrings will be stored.
/// @param input The input string to be split.
/// @param separators The string containing characters to use as separators (n.b. duplicates they will be treated as
/// a single separator)
/// @return A reference to the container holding the resulting substrings.
///
template <typename ResultSequence, typename Sequence1, typename Sequence2 = std::string_view>
inline ResultSequence&
split_at(ResultSequence& buffer, const Sequence1& input, const Sequence2& separators = std::string_view(" \t")) {
    buffer.clear();
    std::string_view in(input);
    std::string_view seps(separators);
    size_t start = in.find_first_not_of(seps);
    while (start != std::string_view::npos) {
        auto pos = in.find_first_of(seps, start);
        if (pos == std::string_view::npos) {
            buffer.emplace_back(in.substr(start));
            break;
        }
        buffer.emplace_back(in.substr(start, pos - start));
        start = in.find_first_not_of(seps, pos + 1);
    }
    return buffer;
}

///
/// @brief Splits an input string into a sequence of substrings based on a given pattern.
///
/// The resulting buffer is cleared of any contents, before being populated with the substrings.
/// Consecutive separator patterns are treated as a single separator, meaning no empty substrings are produced.
///
/// Splitting the input `"a==b==c!=d==e == f"` with the pattern `"=="` would result
/// in a buffer containing `["a", "b", "c!=d", "e ", " f"]`.
///
/// iIf the input string is empty, or contains only repetirion of the separator pattern, the result be ane empty
/// sequence (i.e. `[ ]`).
///
/// @tparam ResultSequence A container type to hold the resulting substrings.
/// @tparam Sequence1 A string-like type representing the input string.
/// @tparam Sequence2 A string-like type representing the pattern to split by (n.b. if the pattern contains duplicated
/// characters, they will be treated as a single separator).
///
/// @param buffer A reference to the container where the resulting substrings will be stored.
/// @param input The input string to be split.
/// @param pattern The pattern to split the input string by.
/// @return A reference to the container holding the resulting substrings.
///
template <typename ResultSequence, typename Sequence1, typename Sequence2 = std::string>
inline ResultSequence& split_by(ResultSequence& buffer, const Sequence1& input, const Sequence2& pattern) {
    buffer.clear();
    std::string::size_type start = 0;
    std::string::size_type pos   = 0;
    while ((pos = input.find(pattern, start)) != std::string::npos) {
        auto token = input.substr(start, pos - start);
        if (!token.empty()) {
            buffer.push_back(token);
        }
        if constexpr (std::is_same_v<Sequence2, std::string>) {
            start = pos + std::size(pattern);
        }
        else if constexpr (std::is_same_v<Sequence2, std::string_view>) {
            start = pos + std::size(pattern);
        }
        else {
            start = pos + std::size(pattern) - 1;
        }
    }
    auto token = input.substr(start);
    if (!token.empty()) {
        buffer.push_back(token);
    }
    return buffer;
}

///
/// @brief Tokenize the input string selecting the content within the specified quotation marks.
///
/// The quotation marks are included in the resulting tokens!
///
/// Some example:
///  - split_within_quotes("'a'  x  'b' y 'c'  zz  ", "'") will return tokens ['a', x, 'b', y, 'c', zz].
///    Notice that any non-whitespace characters _outside_ the quotation marks are also collected (without quotes).
///
///  - split_within_quotes("'a \"b\" c' \"d 'e' f\" , "\"'") will return ['a \"b\" c', 'd \'e\' f']
///    Every quotation mark provided is considered, but only one level/quoted region is considered.
///    This means that regions are detected by matching the first quote found.
///
/// @param input The input string to tokenize.
/// @param quotes The set of characters to use as quotation marks.
/// @return A vector of string views representing the tokens extracted from the input string.
///
std::vector<std::string_view> split_within_quotes(const std::string& input, std::string_view quotes);

///
/// @brief Check if the input string starts with the given pattern.
///
/// Notice that an empty pattern will match any string (empty or otherwise).
///
/// @tparam Sequence1 A string-like type representing the input string.
/// @tparam Sequence2 A string-like type representing the pattern.
///
/// @param input The input string to check.
/// @param pattern The pattern to check for at the start of the input.
/// @return true if the input starts with the pattern, false otherwise.
///
template <typename Sequence1, typename Sequence2>
inline static bool starts_with(const Sequence1& input, const Sequence2& pattern) {
    std::string_view in(input);
    std::string_view p(pattern);
    return in.size() >= p.size() && in.substr(0, p.size()) == p;
}

///
/// @brief Check if the input string ends with the given pattern.
///
/// @tparam Sequence1 A string-like type representing the input string.
/// @tparam Sequence2 A string-like type representing the pattern.
///
/// @param input The input string to check.
/// @param pattern The pattern to check for at the end of the input.
/// @return true if the input ends with the pattern, false otherwise
/// .
template <typename Sequence1, typename Sequence2>
inline static bool ends_with(const Sequence1& input, const Sequence2& pattern) {
    std::string_view in(input);
    std::string_view p(pattern);
    return in.size() >= p.size() && in.substr(in.size() - p.size()) == p;
}

///
/// @brief Trim leading and trailing whitespace from the input string in-place.
///
/// Whitespace is determined by `std::isspace` (i.e. considers ' ', '\f', '\r', '\n', '\t', '\v').
///
/// @tparam Sequence A string-like type representing the string to trim.
///
/// @param input The string to trim.
///
template <typename Sequence>
inline static void trim(Sequence& input) {
    // remove leading whitespace
    auto leading = std::find_if(std::begin(input), std::end(input), [](unsigned char ch) { return !std::isspace(ch); });
    input.erase(std::begin(input), leading);

    // remove trailing whitespace
    auto trailing =
        std::find_if(std::rbegin(input), std::rend(input), [](unsigned char ch) { return !std::isspace(ch); });
    input.erase(trailing.base(), std::end(input));
}

///
/// @brief Trim leading whitespace from the input string in-place.
///
/// Whitespace is determined by `std::isspace` (i.e. considers ' ', '\f', '\r', '\n', '\t', '\v').
///
/// @tparam Sequence A string-like type representing the string to trim.
///
/// @param input The string to trim.
///
template <typename Sequence>
inline static void trim_leading(Sequence& input) {
    // remove leading whitespace
    auto leading = std::find_if(input.begin(), input.end(), [](unsigned char ch) { return !std::isspace(ch); });
    input.erase(input.begin(), leading);
}

///
/// @brief Return a copy of the input string with leading whitespace removed.
///
/// Whitespace is determined by `std::isspace` (i.e. considers ' ', '\f', '\r', '\n', '\t', '\v').
///
/// @tparam Sequence A string-like type representing the string to trim.
///
/// @param input The string to trim.
/// @return A copy of the input with leading whitespace removed.
///
template <typename Sequence>
inline static Sequence trim_leading_copy(const Sequence& input) {
    Sequence buffer = input;
    // remove leading whitespace
    auto leading = std::find_if(buffer.begin(), buffer.end(), [](unsigned char ch) { return !std::isspace(ch); });
    buffer.erase(buffer.begin(), leading);
    return buffer;
}

///
/// @brief Trim trailing whitespace from the input string in-place.
///
/// Whitespace is determined by `std::isspace` (i.e. considers ' ', '\f', '\r', '\n', '\t', '\v').
///
/// @tparam Sequence A string-like type representing the string to trim.
///
/// @param input The string to trim.
///
template <typename Sequence>
inline static void trim_trailing(Sequence& input) {
    // remove trailing whitespace
    auto trailing = std::find_if(input.rbegin(), input.rend(), [](unsigned char ch) { return !std::isspace(ch); });
    input.erase(trailing.base(), input.end());
}

///
/// @brief Return a copy of the input string with trailing whitespace removed.
///
/// Whitespace is determined by `std::isspace` (i.e. considers ' ', '\f', '\r', '\n', '\t', '\v').
///
/// @tparam Sequence A string-like type representing the string to trim.
///
/// @param input The string to trim.
/// @return A copy of the input with trailing whitespace removed.
///
template <typename Sequence>
inline static Sequence trim_trailing_copy(const Sequence& input) {
    Sequence buffer = input;
    // remove trailing whitespace
    auto trailing = std::find_if(buffer.rbegin(), buffer.rend(), [](unsigned char ch) { return !std::isspace(ch); });
    buffer.erase(trailing.base(), buffer.end());
    return buffer;
}

///
/// @brief Check if the input string contains the given pattern.
///
/// Notice that an empty pattern is considered to be contained in any string.
///
/// @tparam Sequence A string-like type representing the input string.
/// @tparam Pattern A string-like type representing the pattern to search for.
///
/// @param input The input string to search within.
/// @param pattern The pattern to search for.
/// @return true if the input contains the pattern, false otherwise.
///
template <typename Sequence, typename Pattern>
inline static bool contains(const Sequence& input, const Pattern& pattern) {
    return std::string_view(input).find(std::string_view(pattern)) != std::string_view::npos;
}

///
/// @brief Remove all occurrences of the given pattern from the input string in-place.
///
/// @tparam Sequence A string-like type representing the input string.
/// @tparam Pattern A string-like type representing the pattern to remove.
///
/// @param input The input string to modify.
/// @param pattern The pattern to remove from the input.
///
template <typename Sequence, typename Pattern>
inline static void remove_all(Sequence& input, const Pattern& pattern) {
    if (std::empty(std::string_view(pattern))) {
        return;
    }

    std::string_view pat(pattern);
    size_t pos = 0;
    while ((pos = input.find(pat.data(), pos, pat.size())) != std::string::npos) {
        input.erase(pos, pat.size());
    }
}

///
/// @brief Transform a vector of elements into a new vector by applying a function to each element.
///
/// @tparam Sequence A container type containing elements to be transformed.
/// @tparam Functor A callable type that takes a value of type T and returns the transformed value.
///
/// @param input The sequence of elements to transform.
/// @param transformation The function to apply to each element.
/// @return A new vector containing the transformed elements.
///
template <typename Sequence, typename Functor>
inline auto transform_to_vector(const Sequence& input, Functor transformation) {
    std::vector<decltype(transformation(std::declval<typename Sequence::value_type>()))> buffer;
    buffer.reserve(input.size());
    std::transform(std::begin(input), std::end(input), std::back_inserter(buffer), [&transformation](const auto& v) {
        return transformation(v);
    });
    return buffer;
}

///
/// @brief Convert a string to lowercase.
///
/// Returns a copy of the input string with all characters converted to their lowercase equivalents
/// using `std::tolower`.
///
/// @param s The input string
/// @return A new string with all characters converted to lowercase.
///
inline std::string tolower(std::string s) {
    std::transform(std::begin(s), std::end(s), std::begin(s), [](unsigned char c) { return std::tolower(c); });
    return s;
}

///
/// @brief Check if the \param name is valid according to the rules for node/attributes names in ecFlow.
///
/// This is used to verify the validity of nodes and attributes (Variable, Label, Event, ...) names in ecFlow.
///
/// If the name is valid, the error buffer will not be updated (e.g. cleared), otherwise the error buffer will contain a
/// description of the validation failure.
///
/// @param name The name to be validated.
/// @param error A buffer to hold the error description, if validation fails (i.e. when function returns false).
/// @return true if the name is valid, false otherwise.
///
bool is_valid_name(const std::string& name, std::string& error);

///
/// @brief Check if the \param name is valid according to the rules for node names in ecFlow.
///
/// This is used to verify the validity of nodes and attributes (Variable, Label, Event, ...) names in ecFlow.
///
/// @param name The name to be validated.
/// @return true if the name is valid, false otherwise.
///
bool is_valid_name(const std::string& name);

///
/// @brief Remove surrounding double quotes from a string in-place.
///
/// Only if double quotes exist *at the beginning and at the end* of the input, will any change be performed.
///
/// @param input The input string to modify.
///
inline void remove_double_quotes(std::string& input) {
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        input.erase(0, 1);
        input.erase(input.size() - 1, 1);
    }
}

///
/// @brief Remove surrounding double quotes from a string.
///
/// Only if double quotes exist *at the beginning and at the end* of the input, will any change be performed.
///
/// @param input The input string.
/// @return A new string with surrounding double quotes removed.
///
inline std::string remove_double_quotes_copy(std::string input) {
    remove_double_quotes(input);
    return input;
}

///
/// @brief Remove surrounding single quotes from a string in-place.
///
/// Only if single quotes exist *at the beginning and at the end* of the input, will any change be performed.
///
/// @param input The input string to modify.
///
inline void remove_single_quotes(std::string& input) {
    if (input.size() >= 2 && input.front() == '\'' && input.back() == '\'') {
        input.erase(0, 1);
        input.erase(input.size() - 1, 1);
    }
}

///
/// @brief Remove surrounding single quotes from a string.
///
/// Only if single quotes exist *at the beginning and at the end* of the input, will any change be performed.
///
/// @param input The input string.
/// @return A new string with surrounding single quotes removed.
///
inline std::string remove_single_quotes_copy(std::string input) {
    remove_single_quotes(input);
    return input;
}

///
/// @brief Compare two strings case-insensitively.
///
/// @param s1 The first string to compare.
/// @param s2 The second string to compare.
/// @return True if the strings are equal when case is ignored, false otherwise.
///
inline bool case_insensitive_compare(const std::string& s1, const std::string& s2) {
    auto compare = [](char a, char b) { return (toupper(a) == toupper(b)); };
    return ((s1.size() == s2.size()) && equal(s1.begin(), s1.end(), s2.begin(), compare));
}

///
/// @brief Compare two strings case-insensitively using a less-than ordering.
///
/// When characters differ only in case, uppercase is considered greater (i.e. 'a' < 'A').
/// Otherwise, characters are compared by their uppercase equivalents.
///
/// @param a The first string to compare.
/// @param b The second string to compare.
/// @return True if \p a is less than \p b under case-insensitive ordering, false otherwise.
///
inline bool case_insensitive_less(const std::string& a, const std::string& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](char x, char y) -> bool {
        if (toupper(x) == toupper(y)) {
            return x > y;
        }
        return toupper(static_cast<unsigned char>(x)) < toupper(static_cast<unsigned char>(y));
    });
}

///
/// @brief Compare two strings case-insensitively using a greater-than ordering.
///
/// When characters differ only in case, lowercase is considered greater (i.e. 'A' < 'a').
/// Otherwise, characters are compared by their uppercase equivalents in reverse order.
///
/// @param a The first string to compare.
/// @param b The second string to compare.
/// @return True if \p a is greater than \p b under case-insensitive ordering, false otherwise.
///
inline bool case_insensitive_greater(const std::string& a, const std::string& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](char x, char y) -> bool {
        if (toupper(x) == toupper(y)) {
            return x < y;
        }
        return toupper(static_cast<unsigned char>(x)) > toupper(static_cast<unsigned char>(y));
    });
}

///
/// @brief Check if the string can be converted to an integer.
///
/// @param input the string to check.
/// @return true if the string can be converted to an integer, false otherwise.
///
inline bool is_int(const std::string& input) {
    if (input.empty()) {
        return false;
    }
    size_t start = 0;
    if (input[0] == '+' || input[0] == '-') {
        start = 1;
    }
    if (start >= input.size()) {
        return false;
    }
    return std::all_of(input.begin() + start, input.end(), [](unsigned char ch) { return std::isdigit(ch); });
}

///
/// @brief Convert a given string to an integer.
///
/// This function checks if the string has any digits before attempting the conversion -- this approach is deemed
/// faster than using ecf::convert_to<int> directly (and thus always attempt to perform the conversion).
///
/// Use this function when it is not possible to ensure the string is convertible to an integer (e.g. user input).
///
/// @param input The string to convert.
/// @param error_return The value to return if the conversion fails (e.g. if the string contains non-numerical chars).
/// @return upon successful conversion, the converted value; otherwise the given `error_return` value.
///
inline int to_int(const std::string& input, int error_return = std::numeric_limits<int>::max()) {
    if (input.find_first_of(ecf::string_constants::numeric_chars, 0) != std::string::npos) {
        try {
            return ecf::convert_to<int>(input);
        }
        catch (const ecf::bad_conversion&) {
            // falls through to returning error...
        }
    }
    return error_return;
}

///
/// @brief Trim the string \param fileContents, so that \param max_lines at the tail of the file are kept.
///
/// Lines are counted by the number of '\n' characters found when scanning from the end of the string towards the
/// beginning. Once \param max_lines newlines have been encountered, all content before (and including) the newline
/// at the split point is erased.
///
/// @param fileContents  The string to truncate in-place.
/// @param max_lines     The maximum number of lines to retain (counted from the end).
/// @return true if the string was truncated, false if it was empty or already within the limit.
///
bool tail(std::string& fileContents, size_t max_lines);

///
/// @brief Trim the string \param fileContents, so that \param max_lines at the head of the file are kept.
///
/// Lines are counted by the number of '\n' characters found when scanning from the begining of the string towards
/// the end. Once \param max_lines newlines have been encountered, all content after (and including) the
/// newline at the split point is erased.
///
/// @param fileContents  The string to truncate in-place.
/// @param max_lines     The maximum number of lines to retain (counted from the beginninf).
/// @return true if the string was truncated, false if it was empty or already within the limit.
///
bool head(std::string& fileContents, size_t max_lines);

///
/// @brief Convert a boolean value to a string representation.
///
/// @param value The boolean value to convert to a string.
/// @return 'true' if the value is true, 'false' otherwise.
///
inline std::string as_string(bool value) {
    return value ? "true" : "false";
}

///
/// @brief Convert a string vector to a string representation.
///
/// @param values The string vector to convert to a string.
/// @return A string representation of the vector, with elements separated by ", " and enclosed in square brackets.
///
inline std::string as_string(const std::vector<std::string>& values) {
    return "[ " + ecf::algorithm::join(values) + " ]";
}

///
/// @brief Convert a string_view vector to a string representation.
///
/// @param values The string_view vector to convert to a string.
/// @return A string representation of the vector, with elements separated by ", " and enclosed in square brackets.
///
inline std::string as_string(const std::vector<std::string_view>& values) {
    return "[ " + ecf::algorithm::join(values) + " ]";
}

///
/// @brief Convert a vector of arithmetic values to a string representation.
///
/// @tparam T The type of elements in the vector -- must be an arithmetic type (e.g. int, double, etc.).
/// @param values The vector of arithmetic values to convert to a string.
/// @return A string representation of the vector, with elements separated by ", " and enclosed in square brackets.
///
template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
inline std::string as_string(const std::vector<T>& values) {
    auto strings = ecf::algorithm::transform_to_vector(values, [](const auto& v) { return std::to_string(v); });
    return "[ " + ecf::algorithm::join(strings) + " ]";
}

///
/// @brief Replace the first occurrence of 'find' with 'replace' in 'input'.
///
/// When 'find' pattern is empty, no replacement is made and false is returned.
///
/// @param input The input string in which to perform the replacement.
/// @param find The string to search for within the input.
/// @param replace The string to replace occurrences of 'find' with.
/// @return true if a replacement was made, false otherwise.
///
inline bool replace(std::string& input, const std::string& find, const std::string& replace) {
    if (find.empty()) {
        return false;
    }

    if (size_t pos = input.find(find); pos != std::string::npos) {
        input.replace(pos, find.length(), replace);
        return true;
    }
    return false;
}

///
/// @brief Replace all occurrences of 'find' with 'replace' in 'input'.
///
/// When 'find' pattern is empty, no replacement is made and false is returned.
///
/// @param input The input string in which to perform the replacement.
/// @param find The string to search for within the input.
/// @param replace The string to replace occurrences of 'find' with.
/// @return true if at least one replacement was made, false otherwise.
///
inline bool replace_all(std::string& input, const std::string& find, const std::string& replace) {
    if (find.empty()) {
        return false;
    }

    bool replaced = false;
    size_t pos    = 0;
    while ((pos = input.find(find, pos)) != std::string::npos) {
        input.replace(pos, find.length(), replace);
        pos += replace.length();
        replaced = true;
    }
    return replaced;
}

///
/// @brief Extract the token at the specified index from the input string, using the given delimiters.
///
/// @param input The input string to tokenise.
/// @param index The (0-based) index of the token to extract.
/// @param token The extracted token.
/// @param delimiters The set of characters to use as token separators.
/// @return true if a token was found at the specified index, false otherwise.
///
bool get_token(std::string_view input, size_t index, std::string& token, std::string_view delimiters = " \t");

} // namespace algorithm

} // namespace ecf

#endif /* ecflow_core_Str_HPP */
