/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Str.hpp"

#include "ecflow/core/Converter.hpp"

using namespace std;

namespace ecf {

const char* VALID_NODE_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.";

const char* Str::CHILD_CMD() {
    static const char* CHILD_CMD = "chd:";
    return CHILD_CMD;
}
const char* Str::USER_CMD() {
    static const char* USER_CMD = "--";
    return USER_CMD;
}
const char* Str::SVR_CMD() {
    static const char* SVR_CMD = "svr:";
    return SVR_CMD;
} // Only for automatic check_pt

const std::string& Str::EMPTY() {
    static std::string empty = std::string();
    return empty;
}
const std::string& Str::ROOT_PATH() {
    static std::string root_path = "/";
    return root_path;
}
const std::string& Str::PATH_SEPARATOR() {
    static std::string path_sep = "/";
    return path_sep;
}
const std::string& Str::COLON() {
    static std::string colon = ":";
    return colon;
}

const std::string& Str::STATE_CHANGE() {
    static std::string state_change = "   state change ";
    return state_change;
}

const std::string& Str::TASK() {
    static std::string task = "TASK";
    return task;
}
const std::string& Str::FAMILY() {
    static std::string family = "FAMILY";
    return family;
}
const std::string& Str::SUITE() {
    static std::string suite = "SUITE";
    return suite;
}
const std::string& Str::ALIAS() {
    static std::string alias = "ALIAS";
    return alias;
}

const std::string& Str::DEFAULT_PORT_NUMBER() {
    static std::string port_number = "3141";
    return port_number;
}
const std::string& Str::LOCALHOST() {
    static std::string localhost = "localhost";
    return localhost;
}

const std::string& Str::WHITE_LIST_FILE() {
    static std::string WHITE_LIST_FILE = "ecf.lists";
    return WHITE_LIST_FILE;
}

const std::string& Str::ALPHANUMERIC_UNDERSCORE() {
    static string ALPHANUMERIC_UNDERSCORE = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    return ALPHANUMERIC_UNDERSCORE;
}

const std::string& Str::NUMERIC() {
    static string NUMERIC = "0123456789";
    return NUMERIC;
}

void Str::removeQuotes(std::string& s) {
    if (!s.empty()) {
        if (s[0] == '"' && s[s.size() - 1] == '"') {
            s.erase(s.begin());
            s.erase(s.begin() + s.size() - 1);
        }
    }
}
// 047 is octal for '
void Str::removeSingleQuotes(std::string& s) {
    if (!s.empty()) {
        if (s[0] == 047 && s[s.size() - 1] == 047) {
            s.erase(s.begin());
            s.erase(s.begin() + s.size() - 1);
        }
    }
}

bool Str::replace(std::string& jobLine, const std::string& stringToFind, const std::string& stringToReplace) {
    size_t pos = jobLine.find(stringToFind);
    if (pos != string::npos) {
        jobLine.replace(pos, stringToFind.length(), stringToReplace);
        return true;
    }
    return false;
}

bool Str::replace_all(std::string& subject, const std::string& stringToFind, const std::string& stringToReplace) {
    bool replaced = false;
    size_t pos    = 0;
    while ((pos = subject.find(stringToFind, pos)) != std::string::npos) {
        subject.replace(pos, stringToFind.length(), stringToReplace);
        pos += stringToReplace.length();
        replaced = true;
    }
    return replaced;
}

bool Str::extract_data_member_value(const std::string& str,
                                    const std::string& data_member_name,
                                    std::string& data_member_value) {
    //        012345678901234567,
    //    str=cmd 1 user:mao
    //    data_member_name=user:
    //    data_member_value=ma0
    std::string::size_type start = str.find(data_member_name);
    if (start != std::string::npos) {
        start += data_member_name.size();
        data_member_value.clear();
        for (size_t i = start; i < str.size(); i++) {
            if (str[i] == ' ')
                break;
            data_member_value += str[i];
        }
        return true;
    }
    return false;
}

void Str::replaceall(std::string& subject, const std::string& search, const std::string& replace) {
    boost::replace_all(subject, search, replace);
}

#define USE_STRINGSPLITTER 1
void Str::split(const std::string& line, std::vector<std::string>& tokens, const std::string& delimiters) {
#ifdef USE_STRINGSPLITTER
    Str::split_using_string_view(line, tokens, delimiters);
#else
    Str::split_orig(line, tokens, delimiters);
#endif
    //   This test will split a line 1000000 times: 'This is a long string that is going to be used to test the
    //   performance of splitting with different Implementations the fastest times wins ' Time for istreamstream 1000000
    //   times = 2.131s wall, (2.140s user + 0.000s system = 2.140s) CPU (100.4%) Time for std::getline 1000000 times
    //   = 1.490s wall, (1.490s user + 0.000s system = 1.490s) CPU (100.0%) Time for boost::split 1000000 times = 0.930s
    //   wall, (0.920s user + 0.000s system = 0.920s) CPU (99.0%) Time for Str::split_orig 1000000               times
    //   = 1.065s wall, (1.070s user + 0.000s system = 1.070s) CPU (100.5%) Time for Str::split_orig1 1000000 times =
    //   0.561s wall, (0.560s user + 0.000s system = 0.560s) CPU (99.9%) Time for Str::split_using_string_view2 1000000
    //   times = 0.686s wall, (0.690s user + 0.000s system = 0.690s) CPU (100.6%) Time for Str::split_using_string_view
    //   1000000  times = 0.482s wall, (0.480s user + 0.000s system = 0.480s) CPU (99.6%) Time for
    //   make_split_iterator::split 1000000    times = 3.611s wall, (3.610s user + 0.000s system = 3.610s) CPU (100.0%)
    //   Time for std::string_view 1000000            times = 0.769s wall, (0.770s user + 0.000s system = 0.770s) CPU
    //   (100.1%) Time for std::string_view(2) 1000000         times = 0.688s wall, (0.690s user + 0.000s system =
    //   0.690s) CPU (100.3%)
    //  core :: test_str_split_perf_with_file
    //   This test will split each line in file ${ECF_TEST_DEFS_DIR}vsms2.31415.def
    //   Time for istreamstream 2001774                 times = 1.567s wall, (1.570s user + 0.000s system = 1.570s) CPU
    //   (100.2%) Time for std::getline 2001774                  times = 2.456s wall, (2.460s user + 0.000s system
    //   = 2.460s) CPU (100.2%) Time for boost::split 2001774                  times = 1.698s wall, (1.690s user +
    //   0.000s system = 1.690s) CPU (99.5%) Time for Str::split_orig 2001774               times = 0.822s wall, (0.830s
    //   user + 0.000s system = 0.830s) CPU (101.0%) Time for Str::split_orig1 2001774              times = 0.502s wall,
    //   (0.500s user + 0.000s system = 0.500s) CPU (99.7%) Time for Str::split_using_string_view 2001774  times =
    //   0.489s wall, (0.490s user + 0.000s system = 0.490s) CPU (100.3%) Time for Str::split_using_string_view2 2001774
    //   times = 0.639s wall, (0.630s user + 0.000s system = 0.630s) CPU (98.6%) Time for boost::make_split_iterator
    //   2001774    times = 3.338s wall, (3.340s user + 0.000s system = 3.340s) CPU (100.1%) Time for std::string_view
    //   2001774            times = 0.599s wall, (0.600s user + 0.000s system = 0.600s) CPU (100.1%) Time for
    //   std::string_view(2) 2001774         times = 0.696s wall, (0.700s user + 0.000s system = 0.700s) CPU (100.6%)
}

void Str::split_orig(const std::string& line, std::vector<std::string>& tokens, const std::string& delimiters) {
    // Skip delimiters at beginning.
    string::size_type lastPos = line.find_first_not_of(delimiters, 0);

    // Find first "non-delimiter".
    string::size_type pos = line.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos) {
        tokens.push_back(line.substr(lastPos, pos - lastPos)); // Found a token, add it to the vector.
        lastPos = line.find_first_not_of(delimiters, pos);     // Skip delimiters.  Note the "not_of"
        pos     = line.find_first_of(delimiters, lastPos);     // Find next "non-delimiter"
    }
}

void Str::split_orig1(const std::string& line, std::vector<std::string>& tokens, const std::string& delims) {
    auto first = std::cbegin(line);
    auto end   = std::cend(line);

    while (first != end) {
        const auto second = std::find_first_of(first, end, std::cbegin(delims), std::cend(delims));

        if (first != second)
            tokens.emplace_back(first, second);

        if (second == end)
            break;

        first = std::next(second);
    }
}

void Str::split_using_string_view(std::string_view strv, std::vector<std::string>& output, std::string_view delims) {
    // Uses pointers
    for (auto first = strv.data(), second = strv.data(), last = first + strv.size(); second != last && first != last;
         first = second + 1) {
        second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

        if (first != second)
            output.emplace_back(first, second - first);
    }
}

void Str::split_using_string_view2(std::string_view strv, std::vector<std::string>& output, std::string_view delims) {
    size_t first = 0;

    size_t strv_size = strv.size();
    while (first < strv_size) {
        const auto second = strv.find_first_of(delims, first);

        if (first != second) {
            std::string_view ref = strv.substr(first, second - first);
            output.emplace_back(ref.begin(), ref.end());
        }

        if (second == std::string_view::npos)
            break;

        first = second + 1;
    }
}

std::vector<std::string_view> Str::tokenize_quotation(const std::string& s, std::string_view quotes) {

    std::vector<std::string_view> tokens;

    std::string levels;

    const char* current = &s[0];
    const char* start   = current;
    while (*current != 0) {
        if (*current == ' ' && levels.empty()) {
            if (start != current) {
                tokens.emplace_back(start, static_cast<size_t>(current - start));
            }
            start = current + 1;
        }
        else {
            if (std::any_of(
                    std::begin(quotes), std::end(quotes), [&current](char quote) { return *current == quote; })) {
                if (!levels.empty() && (levels.back() == *current)) {
                    levels.pop_back();
                }
                else {
                    if (levels.empty()) {
                        start = current;
                    }
                    levels.push_back(*current);
                }
            }
        }
        ++current;
    }

    if (start != current) {
        tokens.emplace_back(start, static_cast<size_t>(current - start));
    }

    return tokens;
}

bool Str::get_token(std::string_view str, size_t pos, std::string& token, std::string_view delims) {
    //   Time for StringSplitter::get_token 250000 times = 1.457s wall, (1.460s user + 0.000s system = 1.460s) CPU
    //   (100.2%) Time for Str::get_token            250000 times = 0.566s wall, (0.560s user + 0.000s system = 0.560s)
    //   CPU (99.0%) Time for Str::get_token2           250000 times = 0.668s wall, (0.670s user + 0.000s system =
    //   0.670s) CPU (100.3%) Time for Str::get_token3           250000 times = 0.620s wall, (0.620s user + 0.000s
    //   system = 0.620s) CPU (100.0%)

    size_t current_pos = 0;
    auto first         = std::cbegin(str);
    auto end           = std::cend(str);

    while (first != end) {
        const auto second = std::find_first_of(first, end, std::cbegin(delims), std::cend(delims));

        if (first != second) {
            if (current_pos == pos) {
                token = std::string(first, second);
                return true;
            }
            current_pos++;
        }

        if (second == end)
            break;

        first = std::next(second);
    }
    return false;
}

bool Str::get_token2(std::string_view strv, size_t pos, std::string& token, std::string_view delims) {
    size_t current_pos = 0;
    size_t first       = 0;
    while (first < strv.size()) {
        const auto second = strv.find_first_of(delims, first);

        if (first != second) {
            if (current_pos == pos) {
                std::string_view ref = strv.substr(first, second - first);
                token                = std::string(ref.begin(), ref.end());
                return true;
            }
            current_pos++;
        }

        if (second == std::string_view::npos)
            break;

        first = second + 1;
    }
    return false;
}

bool Str::get_token3(std::string_view str, size_t pos, std::string& token, std::string_view delims) {
    size_t current_pos = 0;
    for (auto first = str.data(), second = str.data(), last = first + str.size(); second != last && first != last;
         first = second + 1) {

        second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));
        if (first != second) {
            if (current_pos == pos) {
                token = std::string(first, second - first);
                return true;
            }
            current_pos++;
        }
    }
    return false;
}

boost::split_iterator<std::string::const_iterator> Str::make_split_iterator(const std::string& line,
                                                                            const std::string& delimiters) {
    return boost::make_split_iterator(
        line, boost::algorithm::token_finder(boost::is_any_of(delimiters), boost::algorithm::token_compress_on));
}

static bool caseInsCharCompare(char a, char b) {
    return (toupper(a) == toupper(b));
}

bool Str::caseInsCompare(const std::string& s1, const std::string& s2) {
    return ((s1.size() == s2.size()) && equal(s1.begin(), s1.end(), s2.begin(), caseInsCharCompare));
}

bool Str::caseInsLess(const std::string& a, const std::string& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](char x, char y) -> bool {
        if (toupper(x) == toupper(y)) {
            return x > y;
        }
        return toupper(static_cast<unsigned char>(x)) < toupper(static_cast<unsigned char>(y));
    });
}

bool Str::caseInsGreater(const std::string& a, const std::string& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](char x, char y) -> bool {
        if (toupper(x) == toupper(y)) {
            return x < y;
        }
        return toupper(static_cast<unsigned char>(x)) > toupper(static_cast<unsigned char>(y));
    });
}

bool Str::valid_name(const std::string& name, std::string& msg) {
    // valid names are alphabetic (alphanumeric | underscore | .)
    // however we can't have a leading '.' as that can interfere with trigger expressions

    // verify that the string is not empty
    if (name.empty()) {
        msg = "Invalid name. Empty string.";
        return false;
    }

    // verify that the first character is alphanumeric or is an underscore
    bool result = Str::ALPHANUMERIC_UNDERSCORE().find(name[0], 0) != string::npos;
    if (!result) {
        msg = "Valid names can only consist of alphanumeric characters, "
              "underscores and dots (The first character cannot be a dot). "
              "The first character is not valid (only alphanumeric or an underscore is allowed): ";
        msg += name;
        return false;
    }

    // verify that any other characters are alphanumeric or underscore
    if (name.size() > 1) {
        result = name.find_first_not_of(VALID_NODE_CHARS, 1) == string::npos;
        if (!result) {
            msg = "Valid names can only consist of alphanumeric characters, "
                  "underscores and dots (The first character cannot be a dot). ";
            if (name.find('\r') != string::npos)
                msg += "Windows line ending ? ";
            msg += "'";
            msg += name;
            msg += "'"; // use '<name>' to show if PC format, i.e. carriage return
        }
    }

    return result;
}

bool Str::valid_name(const std::string& name) {
    // valid names are alphabetic (alphanumeric | underscore | .)
    // however we can't have a leading '.' as that can interfere with trigger expressions

    // verify that the string is not empty
    if (name.empty()) {
        return false;
    }

    // verify that the first character is alphabetic or has underscore
    bool result = Str::ALPHANUMERIC_UNDERSCORE().find(name[0], 0) != string::npos;
    if (!result) {
        return false;
    }

    // verify that any other characters are alphanumeric or underscore
    if (name.size() > 1) {
        result = name.find_first_not_of(VALID_NODE_CHARS, 1) == string::npos;
    }

    return result;
}

int Str::to_int(const std::string& the_str, int error_return) {
    if (the_str.find_first_of(Str::NUMERIC(), 0) != std::string::npos) {
        try {
            return ecf::convert_to<int>(the_str);
        }
        catch (const ecf::bad_conversion&) {
        }
    }
    return error_return;
}

bool Str::truncate_at_start(std::string& fileContents, size_t max_lines) {
    if (fileContents.empty())
        return false;

    /// Truncate from the front
    size_t no_of_new_lines = 0;
    for (size_t i = fileContents.size() - 1; i > 0; --i) {
        if (fileContents[i] == '\n')
            no_of_new_lines++;
        if (no_of_new_lines >= max_lines) {
            fileContents.erase(fileContents.begin(), fileContents.begin() + i + 1); // skip new line at start of file
            return true;
        }
    }
    return false;
}

bool Str::truncate_at_end(std::string& fileContents, size_t max_lines) {
    if (fileContents.empty())
        return false;

    /// Truncate from the back
    size_t no_of_new_lines = 0;
    size_t the_size        = fileContents.size();
    for (size_t i = 0; i < the_size; ++i) {
        if (fileContents[i] == '\n')
            no_of_new_lines++;
        if (no_of_new_lines >= max_lines) {
            fileContents.erase(fileContents.begin() + i + 1, fileContents.end()); // skip new line at end of file
            return true;
        }
    }
    return false;
}

std::string Str::dump_string_vec(const std::vector<std::string>& vec) {
    std::string str;
    for (const auto& s : vec) {
        str += s;
        str += "\n";
    }
    return str;
}

} // namespace ecf
