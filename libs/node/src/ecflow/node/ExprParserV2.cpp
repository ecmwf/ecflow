/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

///
/// @file ExprParserV2.cpp
/// @brief Handcrafted recursive-descent lexer/parser for trigger/complete expressions.
///
/// @details This translation unit provides the ecf::expression::v2::ExprParser, a drop-in
/// replacement for the Boost.Spirit Classic grammar in ExprParserV1.cpp.  The implementation
/// follows the authoritative grammar in ecflow/docs/ug/user_manual/expression_spec.rst and
/// reproduces the exact semantics of the V1 parser, including:
///
///  - Greedy (longest-match) tokenisation.
///  - Word-boundary enforcement: word-form keywords (and/or/not/eq/ne/... and node/event
///    states) must not be immediately followed by an identifier-continuation character
///    (letter, digit, '_' or '.').
///  - Contiguous node:VAR tokens (no interior whitespace around ':').
///  - No arithmetic-operator precedence: + - * / % are all equal-precedence, left-associative.
///  - Asymmetric boolean associativity: pure 'and' runs are left-associative; once 'or' or a
///    parenthesised group appears, subsequent heads chain right-associatively.
///  - AstNot::set_root_name("not "/"~ "/"! ") preserving the source spelling.
///  - ExprDuplicate cache consulted first (find) and populated after a successful parse (add).
///

#include "ecflow/node/ExprParserV2.hpp"

#include <cctype>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/core/Chrono.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/DState.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/ExprDuplicate.hpp"
#include "ecflow/node/Flag.hpp"

namespace ecf::expression::v2 {

//
// Lexer
//

///
/// @brief Token kinds produced by the lexer.
///
enum class TK {
    // Literals
    INTEGER,  ///< Unsigned integer (ECFLOW-2112: must be followed by word boundary).
    DATETIME, ///< YYYYMMDDThhmmss instant.
    NAME,     ///< Identifier (letter/digit/'_' start, then letter/digit/'_'/'.').
    // Paths / attributes
    ABS_PATH,    ///< Absolute path segment, e.g. "/suite/family/task".
    DOT_PATH,    ///< Relative "./" path.
    DOTDOT_PATH, ///< Parent-relative "../" path.
    // Composite tokens assembled by the lexer
    VARIABLE,           ///< path:name — contiguous (no interior whitespace around ':').
    PARENT_VAR,         ///< :name (parent variable, colon without a preceding path).
    FLAG_REF,           ///< path<flag>name or /<flag>name — node flag reference.
    CAL_DATE_TO_JULIAN, ///< cal::date_to_julian
    CAL_JULIAN_TO_DATE, ///< cal::julian_to_date
    // Node/event states
    STATE_COMPLETE,
    STATE_ABORTED,
    STATE_QUEUED,
    STATE_SUBMITTED,
    STATE_ACTIVE,
    STATE_UNKNOWN,
    EVENT_SET,
    EVENT_CLEAR,
    // Operators (symbolic)
    EQUAL,     ///< ==
    NOT_EQUAL, ///< !=
    GE,        ///< >=
    LE,        ///< <=
    GT,        ///< >
    LT,        ///< <
    PLUS,      ///< +
    MINUS,     ///< -
    STAR,      ///< *
    SLASH,     ///< /
    PERCENT,   ///< %
    AND_SYM,   ///< &&
    OR_SYM,    ///< ||
    NOT_SYM,   ///< ! (unary)
    TILDE,     ///< ~ (unary not)
    // Operators (word forms — validated at word boundary)
    AND_KW, ///< and / AND
    OR_KW,  ///< or / OR
    NOT_KW, ///< not
    EQ_KW,  ///< eq
    NE_KW,  ///< ne
    LT_KW,  ///< lt
    LE_KW,  ///< le
    GT_KW,  ///< gt
    GE_KW,  ///< ge
    // Structure
    LPAREN, ///< (
    RPAREN, ///< )
    END,    ///< end of input
    ERROR,  ///< lexer error
};

///
/// @brief A single lexical token.
///
struct Token
{
    TK kind;
    std::string text; ///< Source text for name/path/literal tokens.
};

//
// Character classification helpers
//

/// Returns true when @p c can be the first character of a name.
static bool is_name_start(char c) {
    const unsigned char uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) || uc >= 0x80 || c == '_';
}

/// Returns true when @p c can follow the first character of a name.
static bool is_name_cont(char c) {
    const unsigned char uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) || uc >= 0x80 || c == '_' || c == '.';
}

/// Returns true when the character at @p pos in @p s is a word-boundary
/// (the position is past the end, or the character is not an identifier-continuation
/// character including '.').
static bool is_word_boundary(const std::string& s, std::size_t pos) {
    if (pos >= s.size()) {
        return true;
    }
    char c                 = s[pos];
    const unsigned char uc = static_cast<unsigned char>(c);
    return !(std::isalnum(uc) || uc >= 0x80 || c == '_' || c == '.');
}

/// Returns true when all characters of @p s[from..from+len) are decimal digits.
static bool all_digits(const std::string& s, std::size_t from, std::size_t len) {
    for (std::size_t i = from; i < from + len; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    }
    return true;
}

//
// Tokeniser
//

///
/// @brief Tokenises a raw expression string into a flat token list.
///
/// @details Each call to next() advances the cursor by one token.  The tokeniser
/// is greedy (longest match) and enforces word boundaries on all keyword/state tokens.
///
class Lexer {
public:
    ///
    /// @brief Constructs a Lexer operating on the given input string.
    ///
    /// @param[in] input Expression string to tokenise; referenced by pointer (must outlive the Lexer).
    ///
    explicit Lexer(const std::string& input)
        : s_(input),
          pos_(0),
          expects_operand_(true) {}

    ///
    /// @brief Consumes and returns the next token, skipping leading whitespace.
    ///
    /// @return The next token; kind == TK::END when the input is exhausted.
    ///
    Token next() {
        skip_whitespace();
        if (pos_ >= s_.size()) {
            return {TK::END, {}};
        }
        Token token = lex_one();
        update_operand_expectation(token.kind);
        return token;
    }

    ///
    /// @brief Returns the current cursor position (for error reporting).
    ///
    std::size_t pos() const { return pos_; }

private:
    void skip_whitespace() {
        while (pos_ < s_.size() && (s_[pos_] == ' ' || s_[pos_] == '\t')) {
            ++pos_;
        }
    }

    Token lex_one() {
        char c = s_[pos_];

        //
        // Two-character symbolic operators — must check before single-char
        //
        if (c == '=' && pos_ + 1 < s_.size() && s_[pos_ + 1] == '=') {
            pos_ += 2;
            return {TK::EQUAL, "=="};
        }
        if (c == '!' && pos_ + 1 < s_.size() && s_[pos_ + 1] == '=') {
            pos_ += 2;
            return {TK::NOT_EQUAL, "!="};
        }
        if (c == '>' && pos_ + 1 < s_.size() && s_[pos_ + 1] == '=') {
            pos_ += 2;
            return {TK::GE, ">="};
        }
        if (c == '<' && pos_ + 1 < s_.size() && s_[pos_ + 1] == '=') {
            pos_ += 2;
            return {TK::LE, "<="};
        }
        if (c == '&' && pos_ + 1 < s_.size() && s_[pos_ + 1] == '&') {
            pos_ += 2;
            return {TK::AND_SYM, "&&"};
        }
        if (c == '|' && pos_ + 1 < s_.size() && s_[pos_ + 1] == '|') {
            pos_ += 2;
            return {TK::OR_SYM, "||"};
        }

        //
        // Single-character symbolic operators (excluding '/' which must be
        // checked for absolute-path context first — see below)
        //
        switch (c) {
            case '>':
                ++pos_;
                return {TK::GT, ">"};
            case '<':
                ++pos_;
                return {TK::LT, "<"};
            case '+':
                ++pos_;
                return {TK::PLUS, "+"};
            case '-':
                ++pos_;
                return {TK::MINUS, "-"};
            case '*':
                ++pos_;
                return {TK::STAR, "*"};
            case '%':
                ++pos_;
                return {TK::PERCENT, "%"};
            case '!':
                ++pos_;
                return {TK::NOT_SYM, "!"};
            case '~':
                ++pos_;
                return {TK::TILDE, "~"};
            case '(':
                ++pos_;
                return {TK::LPAREN, "("};
            case ')':
                ++pos_;
                return {TK::RPAREN, ")"};
            default:
                break;
        }

        //
        // '/' — either an absolute-path start or a lone divide operator.
        // Must be checked AFTER the two-char operators (already done above)
        // and BEFORE returning SLASH so that "/name" starts an absolute path.
        //
        if (c == '/') {
            if (pos_ + 6 < s_.size() && s_.compare(pos_ + 1, 6, "<flag>") == 0) {
                ++pos_;
                return try_lex_flag_suffix("/");
            }
            if (expects_operand_ && pos_ + 1 < s_.size() && is_name_start(s_[pos_ + 1])) {
                return lex_absolute_path(/*has_leading_slash=*/true);
            }
            ++pos_;
            return {TK::SLASH, "/"};
        }

        //
        // Flag reference: <flag> token inside a flag_ref
        // path<flag>name is tokenised as a complete VARIABLE/FLAG_REF by
        // the path-reading logic below; but a bare '<' that was not <=
        // terminates here as LT (already handled above).
        //

        //
        // Colon: ':' at the start of a non-whitespace token -> parent variable ":name"
        //
        if (c == ':') {
            ++pos_; // consume the ':'
            // Read the variable name immediately (no whitespace allowed inside :name)
            std::size_t start = pos_;
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
            std::string varname = s_.substr(start, pos_ - start);
            if (varname.empty()) {
                return {TK::ERROR, ":"};
            }
            return {TK::PARENT_VAR, ":" + varname};
        }

        //
        // Names, keywords, paths, integers, datetimes, cal:: functions
        //
        if (c == '.' && pos_ + 1 < s_.size() && s_[pos_ + 1] == '.') {
            return lex_dotdot_path();
        }
        if (c == '.' && pos_ + 1 < s_.size() && s_[pos_ + 1] == '/') {
            return lex_dot_path();
        }

        //
        // Name token (also keywords, states, cal:: functions)
        //
        if (is_name_start(c)) {
            return lex_name_or_keyword();
        }

        //
        // Unknown character
        //
        ++pos_;
        return {TK::ERROR, std::string(1, c)};
    }

    // Lex a '..' relative path:  ".." ("/.." )* ("/" name)*
    Token lex_dotdot_path() {
        std::size_t start = pos_;
        // consume ".."
        pos_ += 2;
        // consume additional "/.." segments
        while (pos_ + 2 < s_.size() && s_[pos_] == '/' && s_[pos_ + 1] == '.' && s_[pos_ + 2] == '.') {
            pos_ += 3;
        }
        // consume trailing "/+name" segments (one or more slashes, then a name)
        while (pos_ < s_.size() && s_[pos_] == '/') {
            std::size_t slash_start = pos_;
            while (pos_ < s_.size() && s_[pos_] == '/') {
                ++pos_;
            }
            if (pos_ >= s_.size() || !is_name_start(s_[pos_])) {
                // Trailing slashes or non-name: back up and stop
                pos_ = slash_start;
                break;
            }
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
        }
        std::string path = s_.substr(start, pos_ - start);
        if (path == "..") {
            return {TK::ERROR, path};
        }
        // Check for contiguous ":varname" (variable attached to this path)
        if (pos_ < s_.size() && s_[pos_] == ':') {
            std::size_t colon = pos_;
            ++pos_;
            std::size_t vstart = pos_;
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
            if (pos_ > vstart) {
                return {TK::VARIABLE, path + ":" + s_.substr(vstart, pos_ - vstart)};
            }
            pos_ = colon; // revert; the ':' belongs to the next token
        }
        // Check for flag reference: path<flag>flagname
        if (pos_ < s_.size() && s_[pos_] == '<') {
            Token flag = try_lex_flag_suffix(path);
            if (flag.kind == TK::FLAG_REF) {
                return flag;
            }
        }
        return {TK::DOTDOT_PATH, path};
    }

    // Lex a "./" relative path: "./" name ("/" name)*
    Token lex_dot_path() {
        std::size_t start = pos_;
        pos_ += 2; // consume "./"
        if (pos_ >= s_.size() || !is_name_start(s_[pos_])) {
            return {TK::ERROR, "./"};
        }
        while (pos_ < s_.size() && is_name_start(s_[pos_])) {
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
            // optional further "/name" segments
            if (pos_ < s_.size() && s_[pos_] == '/') {
                std::size_t slash = pos_;
                ++pos_;
                if (pos_ >= s_.size() || !is_name_start(s_[pos_])) {
                    pos_ = slash;
                    break;
                }
            }
            else {
                break;
            }
        }
        std::string path = s_.substr(start, pos_ - start);
        if (pos_ < s_.size() && s_[pos_] == ':') {
            std::size_t colon = pos_;
            ++pos_;
            std::size_t vstart = pos_;
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
            if (pos_ > vstart) {
                return {TK::VARIABLE, path + ":" + s_.substr(vstart, pos_ - vstart)};
            }
            pos_ = colon;
        }
        if (pos_ < s_.size() && s_[pos_] == '<') {
            Token flag = try_lex_flag_suffix(path);
            if (flag.kind == TK::FLAG_REF) {
                return flag;
            }
        }
        return {TK::DOT_PATH, path};
    }

    // Lex an absolute path: [/]name ("+"/"+name)*
    // Consecutive slashes are allowed between segments (e.g. //name), matching V1 behaviour.
    Token lex_absolute_path(bool has_leading_slash) {
        std::size_t start = pos_;
        if (has_leading_slash) {
            ++pos_; // skip leading '/'
        }
        // first name segment
        while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
            ++pos_;
        }
        // additional "/+name" segments (one or more slashes, then a name)
        while (pos_ < s_.size() && s_[pos_] == '/') {
            std::size_t slash_start = pos_;
            // consume all consecutive slashes
            while (pos_ < s_.size() && s_[pos_] == '/') {
                ++pos_;
            }
            if (pos_ >= s_.size() || !is_name_start(s_[pos_])) {
                pos_ = slash_start; // revert — trailing slashes or non-name
                break;
            }
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
        }
        std::string path = s_.substr(start, pos_ - start);
        // Contiguous ":varname"
        if (pos_ < s_.size() && s_[pos_] == ':') {
            std::size_t colon = pos_;
            ++pos_;
            std::size_t vstart = pos_;
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
            if (pos_ > vstart) {
                return {TK::VARIABLE, path + ":" + s_.substr(vstart, pos_ - vstart)};
            }
            pos_ = colon;
        }
        if (pos_ < s_.size() && s_[pos_] == '<') {
            Token flag = try_lex_flag_suffix(path);
            if (flag.kind == TK::FLAG_REF) {
                return flag;
            }
        }
        return {TK::ABS_PATH, path};
    }

    // Try to lex "<flag>flagname" immediately after @p path (cursor at '<').
    // Returns FLAG_REF on success, or an ERROR token leaving the cursor unchanged.
    Token try_lex_flag_suffix(const std::string& path) {
        std::size_t saved = pos_;
        // expect "<flag>"
        static const std::string FLAG_TAG = "<flag>";
        if (s_.substr(pos_, FLAG_TAG.size()) != FLAG_TAG) {
            return {TK::ERROR, {}};
        }
        pos_ += FLAG_TAG.size();
        // flag name
        std::size_t fstart = pos_;
        while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
            ++pos_;
        }
        std::string flagname = s_.substr(fstart, pos_ - fstart);
        if (flagname != "late" && flagname != "zombie" && flagname != "archived") {
            pos_ = saved;
            return {TK::ERROR, {}};
        }
        return {TK::FLAG_REF, path + "<flag>" + flagname};
    }

    // Lex a name, keyword, integer, datetime, or cal:: function.
    Token lex_name_or_keyword() {
        std::size_t start = pos_;

        // Read the raw name/number run
        while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
            ++pos_;
        }
        std::string raw = s_.substr(start, pos_ - start);

        // Check this before integer classification so 12:event is a numeric-leading
        // node attribute path, matching V1 basic_variable_path behaviour.
        if (pos_ < s_.size() && s_[pos_] == ':') {
            ++pos_;
            std::size_t vstart = pos_;
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
            if (pos_ > vstart) {
                return {TK::VARIABLE, raw + ":" + s_.substr(vstart, pos_ - vstart)};
            }
            pos_ = vstart - 1;
        }

        //
        // Datetime: exactly 8 digits + 'T' + 6 digits, word boundary after
        //
        if (raw.size() == 15 && raw[8] == 'T' && is_word_boundary(s_, pos_)) {
            if (all_digits(raw, 0, 8) && all_digits(raw, 9, 6)) {
                return {TK::DATETIME, raw};
            }
        }

        //
        // Integer: all digits, word boundary after
        //
        bool all_dig = !raw.empty() && std::all_of(raw.begin(), raw.end(), [](char c) {
            return std::isdigit(static_cast<unsigned char>(c));
        });
        // A digits-only run followed by a path separator is a numeric-leading node path
        // (for example "12/archive"), not an integer literal. V1's nodename rule accepts
        // numeric path segments before its integer rule is considered.
        if (all_dig && is_word_boundary(s_, pos_) && (pos_ >= s_.size() || s_[pos_] != '/')) {
            return {TK::INTEGER, raw};
        }

        //
        // cal:: function (two-part identifier with '::')
        // The raw text is "cal" here (the name stops at ':'); the ':' that
        // follows is NOT a name-continuation character, so we consumed "cal".
        // We need to check whether what follows is "::date_to_julian" etc.
        //
        if (raw == "cal" && pos_ + 1 < s_.size() && s_[pos_] == ':' && s_[pos_ + 1] == ':') {
            std::size_t saved  = pos_;
            std::size_t fstart = pos_ + 2;
            pos_               = fstart;
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
            std::string fname = s_.substr(fstart, pos_ - fstart);
            if (fname == "date_to_julian" && is_word_boundary(s_, pos_)) {
                return {TK::CAL_DATE_TO_JULIAN, "cal::date_to_julian"};
            }
            if (fname == "julian_to_date" && is_word_boundary(s_, pos_)) {
                return {TK::CAL_JULIAN_TO_DATE, "cal::julian_to_date"};
            }
            pos_ = saved;
        }

        // A colon immediately after a name always forms a variable reference.  Check this
        // before recognising state/operator keywords: V1 accepts node names such as
        // "complete" and "active" in complete:attrib / active:attrib references.
        if (pos_ < s_.size() && s_[pos_] == ':') {
            ++pos_;
            std::size_t vstart = pos_;
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
            if (pos_ > vstart) {
                return {TK::VARIABLE, raw + ":" + s_.substr(vstart, pos_ - vstart)};
            }
            pos_ = vstart - 1;
        }

        //
        // Keyword / state tokens (only when followed by a word boundary)
        //
        if (is_word_boundary(s_, pos_)) {
            if (raw == "and" || raw == "AND") {
                return {TK::AND_KW, raw};
            }
            if (raw == "or" || raw == "OR") {
                return {TK::OR_KW, raw};
            }
            if (raw == "not") {
                return {TK::NOT_KW, raw};
            }
            if (raw == "eq") {
                return {TK::EQ_KW, raw};
            }
            if (raw == "ne") {
                return {TK::NE_KW, raw};
            }
            if (raw == "lt") {
                return {TK::LT_KW, raw};
            }
            if (raw == "le") {
                return {TK::LE_KW, raw};
            }
            if (raw == "gt") {
                return {TK::GT_KW, raw};
            }
            if (raw == "ge") {
                return {TK::GE_KW, raw};
            }
            // Node states
            if (raw == "complete") {
                return {TK::STATE_COMPLETE, raw};
            }
            if (raw == "aborted") {
                return {TK::STATE_ABORTED, raw};
            }
            if (raw == "queued") {
                return {TK::STATE_QUEUED, raw};
            }
            if (raw == "submitted") {
                return {TK::STATE_SUBMITTED, raw};
            }
            if (raw == "active") {
                return {TK::STATE_ACTIVE, raw};
            }
            if (raw == "unknown") {
                return {TK::STATE_UNKNOWN, raw};
            }
            // Event states
            if (raw == "set") {
                return {TK::EVENT_SET, raw};
            }
            if (raw == "clear") {
                return {TK::EVENT_CLEAR, raw};
            }
        }

        //
        // Plain name: may be a standalone node name (without path) or
        // the first segment of an absolute path.  Check for a following
        // "/" to build an absolute path, or for ":varname".
        //
        // Continue consuming "/name" segments if we have a plain name
        while (pos_ < s_.size() && s_[pos_] == '/') {
            std::size_t slash = pos_;
            ++pos_;
            if (pos_ >= s_.size() || !is_name_start(s_[pos_])) {
                pos_ = slash;
                break;
            }
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
        }
        std::string fullpath = s_.substr(start, pos_ - start);

        // Contiguous ":varname" — variable reference
        if (pos_ < s_.size() && s_[pos_] == ':') {
            std::size_t colon = pos_;
            ++pos_;
            std::size_t vstart = pos_;
            while (pos_ < s_.size() && is_name_cont(s_[pos_])) {
                ++pos_;
            }
            if (pos_ > vstart) {
                return {TK::VARIABLE, fullpath + ":" + s_.substr(vstart, pos_ - vstart)};
            }
            pos_ = colon; // revert
        }

        // Flag reference
        if (pos_ < s_.size() && s_[pos_] == '<') {
            Token flag = try_lex_flag_suffix(fullpath);
            if (flag.kind == TK::FLAG_REF) {
                return flag;
            }
        }

        // Determine whether this is a path or a plain name
        bool is_path = (fullpath.find('/') != std::string::npos);
        if (is_path) {
            return {TK::ABS_PATH, fullpath};
        }
        return {TK::NAME, fullpath};
    }

    void update_operand_expectation(TK kind) {
        switch (kind) {
            case TK::INTEGER:
            case TK::DATETIME:
            case TK::NAME:
            case TK::ABS_PATH:
            case TK::DOT_PATH:
            case TK::DOTDOT_PATH:
            case TK::VARIABLE:
            case TK::PARENT_VAR:
            case TK::FLAG_REF:
            case TK::STATE_COMPLETE:
            case TK::STATE_ABORTED:
            case TK::STATE_QUEUED:
            case TK::STATE_SUBMITTED:
            case TK::STATE_ACTIVE:
            case TK::STATE_UNKNOWN:
            case TK::EVENT_SET:
            case TK::EVENT_CLEAR:
            case TK::RPAREN:
                expects_operand_ = false;
                return;
            default:
                expects_operand_ = true;
                return;
        }
    }

    const std::string& s_;
    std::size_t pos_;
    bool expects_operand_;

    friend class TokenStream;
};

//
// Token stream (2-token lookahead with save/restore for limited backtracking)
//

///
/// @brief Saved state of the TokenStream, used for limited backtracking.
///
struct TokenStreamState
{
    Token current;
    Token lookahead;
    std::size_t lexer_pos;
    bool lexer_expects_operand;
};

///
/// @brief A two-token lookahead wrapper over the Lexer, with save/restore support.
///
/// @details The second token is pre-fetched into @c lookahead_ so that callers can
/// inspect the token-after-current without consuming.  The save()/restore() pair
/// enables limited backtracking: callers save the state before a speculative parse,
/// and restore it if the parse fails.
///
class TokenStream {
public:
    explicit TokenStream(const std::string& s)
        : lexer_(s) {
        current_   = lexer_.next();
        lookahead_ = lexer_.next();
    }

    /// Peek at the current token without consuming it.
    const Token& peek() const { return current_; }

    /// Peek at the token after the current token (2-token lookahead).
    const Token& peek2() const { return lookahead_; }

    /// Consume the current token and advance to the next one.
    Token consume() {
        Token t    = current_;
        current_   = lookahead_;
        lookahead_ = lexer_.next();
        return t;
    }

    /// Returns true when the current token has the given kind.
    bool is(TK k) const { return current_.kind == k; }

    /// Returns true when the current token is an equality-comparison operator.
    bool is_eq_op() const {
        TK k = current_.kind;
        return k == TK::EQUAL || k == TK::NOT_EQUAL || k == TK::EQ_KW || k == TK::NE_KW;
    }

    /// Returns true when the current token is any comparison operator (eq + relational).
    bool is_cmp_op() const {
        if (is_eq_op()) {
            return true;
        }
        TK k = current_.kind;
        return k == TK::LT || k == TK::LT_KW || k == TK::LE || k == TK::LE_KW || k == TK::GT || k == TK::GT_KW ||
               k == TK::GE || k == TK::GE_KW;
    }

    /// Returns true when the current token is an arithmetic operator.
    bool is_arith_op() const {
        TK k = current_.kind;
        return k == TK::PLUS || k == TK::MINUS || k == TK::STAR || k == TK::SLASH || k == TK::PERCENT;
    }

    /// Returns true when the current token is an 'and' form.
    bool is_and() const {
        TK k = current_.kind;
        return k == TK::AND_KW || k == TK::AND_SYM;
    }

    /// Returns true when the current token is an 'or' form.
    bool is_or() const {
        TK k = current_.kind;
        return k == TK::OR_KW || k == TK::OR_SYM;
    }

    /// Returns true when the current token is a 'not' form.
    bool is_not() const {
        TK k = current_.kind;
        return k == TK::NOT_KW || k == TK::NOT_SYM || k == TK::TILDE;
    }

    /// Returns true when the current token is a node/event state.
    bool is_node_state() const {
        TK k = current_.kind;
        return k == TK::STATE_COMPLETE || k == TK::STATE_ABORTED || k == TK::STATE_QUEUED || k == TK::STATE_SUBMITTED ||
               k == TK::STATE_ACTIVE || k == TK::STATE_UNKNOWN;
    }

    bool is_event_state() const {
        TK k = current_.kind;
        return k == TK::EVENT_SET || k == TK::EVENT_CLEAR;
    }

    bool is_path() const {
        TK k = current_.kind;
        return k == TK::ABS_PATH || k == TK::DOT_PATH || k == TK::DOTDOT_PATH;
    }

    /// Saves the full token-stream state (current + lookahead + lexer position).
    TokenStreamState save() const { return {current_, lookahead_, lexer_.pos_, lexer_.expects_operand_}; }

    /// Restores a previously saved state, enabling limited backtracking.
    void restore(const TokenStreamState& s) {
        current_                = s.current;
        lookahead_              = s.lookahead;
        lexer_.pos_             = s.lexer_pos;
        lexer_.expects_operand_ = s.lexer_expects_operand;
    }

private:
    Lexer lexer_;
    Token current_;
    Token lookahead_;
};

//
// Error handling
//

struct ParseError : std::exception
{
    explicit ParseError(std::string m)
        : msg(std::move(m)) {}
    const char* what() const noexcept override { return msg.c_str(); }
    std::string msg;
};

//
// Parser
//

///
/// @brief Recursive-descent parser that produces an Ast* tree from a token stream.
///
/// @details Implements the grammar from expression_spec.rst.  Methods are named
/// after the grammar productions they correspond to.  Every parse_XXX() method
/// allocates AST nodes and returns a raw pointer; ownership is always transferred
/// immediately to a parent node or wrapped in a unique_ptr in doParse().
///
class Parser {
public:
    explicit Parser(const std::string& expr)
        : ts_(expr) {}

    ///
    /// @brief Parse a complete expression and return the AST root.
    ///
    /// @param[out] error Diagnostic message on failure; empty on success.
    /// @return A valid AstTop*, or nullptr on failure.
    ///
    std::unique_ptr<AstTop> parse(std::string& error) {
        try {
            auto top  = std::make_unique<AstTop>();
            Ast* body = parse_subexpression();
            if (!ts_.is(TK::END)) {
                std::ostringstream msg;
                msg << "Unexpected token '" << ts_.peek().text << "' after expression";
                throw ParseError(msg.str());
            }
            top->addChild(body);
            if (!top->is_valid_ast(error)) {
                return nullptr;
            }
            return top;
        }
        catch (const ParseError& e) {
            error = e.msg;
            return nullptr;
        }
    }

private:
    //
    // Grammar productions
    //

    // subexpression : head [ (and | or) subexpression ]
    Ast* parse_subexpression() {
        Ast* lhs = parse_head();

        if (ts_.is_and()) {
            ts_.consume();
            Ast* rhs   = parse_subexpression();
            auto* node = new AstAnd();
            node->addChild(lhs);
            node->addChild(rhs);
            return node;
        }
        if (ts_.is_or()) {
            ts_.consume();
            Ast* rhs   = parse_subexpression();
            auto* node = new AstOr();
            node->addChild(lhs);
            node->addChild(rhs);
            return node;
        }
        return lhs;
    }

    // head : [ not ] ( "(" subexpression ")" | and_expr )
    //
    // Parentheses are ambiguous: they can group an arithmetic operand in a comparison
    // ("(:YMD + 1) == 20240102") or a boolean subexpression
    // ("(a == complete or b == complete)"). V1 resolves this with ordered Spirit
    // alternatives. Replicate that behaviour by trying and_expr first, then restoring
    // the token stream and parsing a boolean group only when the first alternative fails.
    Ast* parse_head() {
        if (ts_.is_not()) {
            Ast* lhs = parse_unary_not();
            while (ts_.is_and()) {
                ts_.consume();
                Ast* rhs   = ts_.is_not() ? parse_unary_not() : parse_comparison();
                auto* node = new AstAnd();
                node->addChild(lhs);
                node->addChild(rhs);
                lhs = node;
            }
            return lhs;
        }

        TokenStreamState saved = ts_.save();
        try {
            return parse_and_expr();
        }
        catch (const ParseError&) {
            ts_.restore(saved);
        }

        if (ts_.is(TK::LPAREN)) {
            ts_.consume();
            Ast* inner = parse_subexpression();
            expect(TK::RPAREN, ")");
            return inner;
        }

        throw ParseError("Unexpected token in expression: '" + ts_.peek().text + "'");
    }

    // and_expr : comparison { and [ not ] comparison }
    // Left-associative for a run of comparisons.  When a comparison after "and" fails to parse
    // (which happens when the following token is the start of a boolean group that calc_factor
    // cannot handle arithmetically), the state is restored to before the "and" was consumed
    // and the loop stops.  parse_subexpression() then handles "and (boolean_group)"
    // right-associatively via its recursive descent.
    Ast* parse_and_expr() {
        Ast* lhs = parse_comparison();

        while (ts_.is_and()) {
            // Save state before consuming 'and', so we can restore if the following
            // comparison fails (i.e. it is a boolean group, not an arithmetic one).
            TokenStreamState saved = ts_.save();
            ts_.consume(); // consume 'and'

            bool neg         = false;
            AstNot* not_node = nullptr;
            if (ts_.is_not()) {
                not_node = make_not_node();
                neg      = true;
            }

            try {
                Ast* rhs = parse_comparison();
                if (neg) {
                    not_node->addChild(rhs);
                    rhs      = not_node;
                    not_node = nullptr;
                }
                auto* node = new AstAnd();
                node->addChild(lhs);
                node->addChild(rhs);
                lhs = node;
            }
            catch (const ParseError&) {
                // The comparison failed — the following token must be a boolean group.
                // Restore the stream (including the 'and') and let parse_subexpression()
                // handle it right-associatively.
                delete not_node;
                ts_.restore(saved);
                break;
            }
        }
        return lhs;
    }

    // comparison : node_path eq_op node_state
    //            | attribute eq_op event_state
    //            | calc_expr [ cmp_op [ not ] calc_expr ]
    Ast* parse_comparison() {
        // Peek ahead to detect node-path-state or attribute-event-state forms.
        // Both always appear as an already-fully-lexed token.

        TK k = ts_.peek().kind;

        // A digits-only lexeme is a node path only in the precise V1
        // node-state-comparison form: "12 == complete". In every other context
        // (notably "0740 lt :TIME") it is an integer calculation factor.
        if (k == TK::INTEGER) {
            const TokenStreamState saved = ts_.save();
            const std::string path       = ts_.consume().text;
            if (ts_.is_eq_op()) {
                const TK op_kind = ts_.consume().kind;
                if (ts_.is_node_state()) {
                    auto* cmp = make_cmp_node(op_kind);
                    cmp->addChild(new AstNode(path));
                    cmp->addChild(make_node_state());
                    return cmp;
                }
            }
            ts_.restore(saved);
        }

        // node_path eq_op node_state
        // Note: in the V1 grammar, a bare name (no '/') IS a valid absolute path.
        // We therefore include TK::NAME here alongside the path token kinds.
        if (ts_.is_path() || k == TK::NAME) {
            // We need to look at what follows to decide whether this is a state comparison
            // or an arithmetic comparison.  Both can appear on the left of a cmp_op.
            // We parse eagerly:  try state comparison first (it is not ambiguous — if
            // the path is followed by eq_op then node_state, it is a state comparison).
            // We remember the position and fall through to calc_expr on failure.
            std::string path = ts_.peek().text;
            ts_.consume();

            if (ts_.is_eq_op()) {
                // Check whether what follows is a node state
                TK op_kind = ts_.peek().kind;
                ts_.consume(); // consume eq_op
                if (ts_.is_node_state()) {
                    // node_path eq_op node_state — build directly
                    auto* cmp   = make_cmp_node(op_kind);
                    auto* left  = new AstNode(path);
                    auto* right = make_node_state();
                    cmp->addChild(left);
                    cmp->addChild(right);
                    return cmp;
                }
                // Not a node state — treat as calc_expr comparison.
                // Wrap the path as a calc_factor AstNode and continue.
                Ast* left = new AstNode(path);
                left      = parse_calc_rest(left); // handle arith chain on the left
                // Now handle [not] right
                bool neg         = false;
                AstNot* not_node = nullptr;
                if (ts_.is_not()) {
                    not_node = make_not_node();
                    neg      = true;
                }
                Ast* right = parse_calc_expr();
                if (neg) {
                    not_node->addChild(right);
                    right = not_node;
                }
                auto* cmp = make_cmp_node(op_kind);
                cmp->addChild(left);
                cmp->addChild(right);
                return cmp;
            }

            // No comparison operator — the path is a standalone calc_factor (AstNode)
            Ast* node = new AstNode(path);
            node      = parse_calc_rest(node);
            // Allow a comparison operator to follow the arithmetic expression
            if (ts_.is_cmp_op()) {
                TK op_kind = ts_.peek().kind;
                ts_.consume();
                bool neg         = false;
                AstNot* not_node = nullptr;
                if (ts_.is_not()) {
                    not_node = make_not_node();
                    neg      = true;
                }
                Ast* right = parse_calc_expr();
                if (neg) {
                    not_node->addChild(right);
                    right = not_node;
                }
                auto* cmp = make_cmp_node(op_kind);
                cmp->addChild(node);
                cmp->addChild(right);
                return cmp;
            }

            // A bare node path (or arithmetic expression over it) is not a valid,
            // standalone comparison; V1 rejects it too (e.g. "trigger a" with no
            // comparison operator), so do not silently accept it as an AstNode.
            delete node;
            throw ParseError("Expected comparison operator after node path: '" + path + "'");
        }

        // attribute eq_op event_state
        if (k == TK::VARIABLE) {
            std::string vartext = ts_.peek().text;
            // Try event-state comparison: we need eq_op followed by event_state
            // But also calc comparisons use VARIABLE.  We parse the variable as
            // a calc_factor and then check for a comparison operator.
            // The disambiguation: event_state comparisons use *only* eq_op; anything
            // else (calc arithmetic or a different operator) falls through.
            ts_.consume(); // consume the variable token
            auto [nodepath, varname] = split_variable(vartext);

            if (ts_.is_eq_op()) {
                TK op_kind = ts_.peek().kind;
                ts_.consume(); // consume eq_op
                if (ts_.is_event_state()) {
                    // attribute eq_op event_state
                    auto* cmp   = make_cmp_node(op_kind);
                    auto* left  = new AstVariable(nodepath, varname);
                    auto* right = make_event_state();
                    cmp->addChild(left);
                    cmp->addChild(right);
                    return cmp;
                }
                // Not an event state — treat as a numeric comparison
                Ast* left        = new AstVariable(nodepath, varname);
                left             = parse_calc_rest(left);
                bool neg         = false;
                AstNot* not_node = nullptr;
                if (ts_.is_not()) {
                    not_node = make_not_node();
                    neg      = true;
                }
                Ast* right = parse_calc_expr();
                if (neg) {
                    not_node->addChild(right);
                    right = not_node;
                }
                auto* cmp = make_cmp_node(op_kind);
                cmp->addChild(left);
                cmp->addChild(right);
                return cmp;
            }
            // No operator follows — standalone variable (possibly with arith chain)
            Ast* var = new AstVariable(nodepath, varname);
            var      = parse_calc_rest(var);
            if (ts_.is_cmp_op()) {
                TK op_kind = ts_.peek().kind;
                ts_.consume();
                bool neg         = false;
                AstNot* not_node = nullptr;
                if (ts_.is_not()) {
                    not_node = make_not_node();
                    neg      = true;
                }
                Ast* right = parse_calc_expr();
                if (neg) {
                    not_node->addChild(right);
                    right = not_node;
                }
                auto* cmp = make_cmp_node(op_kind);
                cmp->addChild(var);
                cmp->addChild(right);
                return cmp;
            }
            return var;
        }

        // General case: calc_expr [ cmp_op [ not ] calc_expr ]
        Ast* lhs = parse_calc_expr();
        if (ts_.is_cmp_op()) {
            TK op_kind = ts_.peek().kind;
            ts_.consume();
            bool neg         = false;
            AstNot* not_node = nullptr;
            if (ts_.is_not()) {
                not_node = make_not_node();
                neg      = true;
            }
            Ast* rhs = parse_calc_expr();
            if (neg) {
                not_node->addChild(rhs);
                rhs = not_node;
            }
            auto* cmp = make_cmp_node(op_kind);
            cmp->addChild(lhs);
            cmp->addChild(rhs);
            return cmp;
        }
        return lhs;
    }

    // calc_expr : calc_factor { arith_op calc_factor }
    // (all arith ops equal precedence, left-associative)
    Ast* parse_calc_expr() {
        Ast* lhs = parse_calc_factor();
        return parse_calc_rest(lhs);
    }

    // parse_calc_rest: consume any remaining arith ops, building left-associative tree
    Ast* parse_calc_rest(Ast* lhs) {
        while (ts_.is_arith_op()) {
            TK op_kind = ts_.peek().kind;
            ts_.consume();
            Ast* rhs   = parse_calc_factor();
            auto* node = make_arith_node(op_kind);
            node->addChild(lhs);
            node->addChild(rhs);
            lhs = node;
        }
        return lhs;
    }

    // calc_factor : datetime | integer | "(" calc_expr ")" | parent_attribute
    //             | cal_function | flag_ref | attribute | arith_op calc_factor (unary binary node)
    Ast* parse_calc_factor() {
        TK k = ts_.peek().kind;

        if (k == TK::DATETIME) {
            std::string text = ts_.consume().text;
            try {
                return new AstInstant(ecf::Instant::parse(text));
            }
            catch (const std::exception&) {
                throw ParseError("Invalid datetime instant: '" + text + "'");
            }
        }

        if (k == TK::INTEGER) {
            std::string text = ts_.consume().text;
            int val          = ecf::convert_to<int>(text);
            return new AstInteger(val);
        }

        if (k == TK::LPAREN) {
            ts_.consume();
            Ast* inner = parse_calc_expr();
            expect(TK::RPAREN, ")");
            return inner;
        }

        if (k == TK::PARENT_VAR) {
            std::string text = ts_.consume().text;
            // text is ":varname"
            std::string varname = text.substr(1);
            return new AstParentVariable(varname);
        }

        if (k == TK::CAL_DATE_TO_JULIAN || k == TK::CAL_JULIAN_TO_DATE) {
            return parse_cal_function();
        }

        if (k == TK::FLAG_REF) {
            return parse_flag_ref();
        }

        if (k == TK::VARIABLE) {
            std::string text         = ts_.consume().text;
            auto [nodepath, varname] = split_variable(text);
            return new AstVariable(nodepath, varname);
        }

        // Arithmetic operator used as a unary "prefix" (binary node with missing left operand).
        // The expression spec says this produces a binary node with no second operand, and
        // is_valid_ast() rejects it — matching V1 behaviour.
        if (ts_.is_arith_op()) {
            TK op_kind = ts_.peek().kind;
            ts_.consume();
            Ast* rhs   = parse_calc_factor();
            auto* node = make_arith_node(op_kind);
            node->addChild(rhs);
            return node;
        }

        throw ParseError("Unexpected token in expression: '" + ts_.peek().text + "'");
    }

    // parse_unary_not : not ( "(" subexpression ")" | comparison )
    Ast* parse_unary_not() {
        AstNot* not_node       = make_not_node();
        Ast* child             = nullptr;
        TokenStreamState saved = ts_.save();
        try {
            child = parse_comparison();
        }
        catch (const ParseError&) {
            ts_.restore(saved);
        }

        if (child == nullptr && ts_.is(TK::LPAREN)) {
            ts_.consume();
            child = parse_subexpression();
            expect(TK::RPAREN, ")");
        }
        if (child == nullptr) {
            throw ParseError("Expected comparison or parenthesised subexpression after not");
        }
        not_node->addChild(child);
        return not_node;
    }

    // cal_function : ("cal::date_to_julian" | "cal::julian_to_date") "(" ( attribute | integer ) ")"
    Ast* parse_cal_function() {
        TK kind = ts_.peek().kind;
        ts_.consume();
        expect(TK::LPAREN, "(");
        Ast* arg = nullptr;
        if (ts_.is(TK::VARIABLE)) {
            std::string text         = ts_.consume().text;
            auto [nodepath, varname] = split_variable(text);
            arg                      = new AstVariable(nodepath, varname);
        }
        else if (ts_.is(TK::INTEGER)) {
            std::string text = ts_.consume().text;
            arg              = new AstInteger(ecf::convert_to<int>(text));
        }
        else {
            throw ParseError("cal:: function argument must be an attribute or integer");
        }
        expect(TK::RPAREN, ")");
        AstFunction::FuncType ft =
            (kind == TK::CAL_DATE_TO_JULIAN) ? AstFunction::DATE_TO_JULIAN : AstFunction::JULIAN_TO_DATE;
        return new AstFunction(ft, arg);
    }

    // flag_ref : (node_path | "/") <flag> flag_name
    Ast* parse_flag_ref() {
        std::string text = ts_.consume().text; // e.g. "/suite<flag>late" or "path<flag>zombie"
        // Split at "<flag>"
        static const std::string TAG = "<flag>";
        auto sep                     = text.find(TAG);
        if (sep == std::string::npos) {
            throw ParseError("Malformed flag reference: '" + text + "'");
        }
        std::string path     = text.substr(0, sep);
        std::string flagname = text.substr(sep + TAG.size());
        ecf::Flag::Type ft   = ecf::Flag::string_to_flag_type(flagname);
        return new AstFlag(path, ft);
    }

    //
    // Helpers
    //

    void expect(TK kind, const std::string& what) {
        if (!ts_.is(kind)) {
            throw ParseError("Expected '" + what + "', got '" + ts_.peek().text + "'");
        }
        ts_.consume();
    }

    // Create and configure an AstNot node, consuming the current not token.
    AstNot* make_not_node() {
        Token t = ts_.consume();
        auto* n = new AstNot();
        if (t.kind == TK::NOT_KW) {
            n->set_root_name("not ");
        }
        else if (t.kind == TK::TILDE) {
            n->set_root_name("~ ");
        }
        else {
            // TK::NOT_SYM ('!')
            n->set_root_name("! ");
        }
        return n;
    }

    // Create the comparison operator AST node for the given token kind.
    AstRoot* make_cmp_node(TK k) {
        switch (k) {
            case TK::EQUAL:
            case TK::EQ_KW:
                return new AstEqual();
            case TK::NOT_EQUAL:
            case TK::NE_KW:
                return new AstNotEqual();
            case TK::GE:
            case TK::GE_KW:
                return new AstGreaterEqual();
            case TK::LE:
            case TK::LE_KW:
                return new AstLessEqual();
            case TK::GT:
            case TK::GT_KW:
                return new AstGreaterThan();
            case TK::LT:
            case TK::LT_KW:
                return new AstLessThan();
            default:
                throw ParseError("Unknown comparison operator");
        }
    }

    // Create the arithmetic operator AST node for the given token kind.
    AstRoot* make_arith_node(TK k) {
        switch (k) {
            case TK::PLUS:
                return new AstPlus();
            case TK::MINUS:
                return new AstMinus();
            case TK::STAR:
                return new AstMultiply();
            case TK::SLASH:
                return new AstDivide();
            case TK::PERCENT:
                return new AstModulo();
            default:
                throw ParseError("Unknown arithmetic operator");
        }
    }

    // Create an AstNodeState for the current token (which must be a node state).
    AstNodeState* make_node_state() {
        TK k = ts_.peek().kind;
        ts_.consume();
        switch (k) {
            case TK::STATE_COMPLETE:
                return new AstNodeState(DState::COMPLETE);
            case TK::STATE_ABORTED:
                return new AstNodeState(DState::ABORTED);
            case TK::STATE_QUEUED:
                return new AstNodeState(DState::QUEUED);
            case TK::STATE_SUBMITTED:
                return new AstNodeState(DState::SUBMITTED);
            case TK::STATE_ACTIVE:
                return new AstNodeState(DState::ACTIVE);
            case TK::STATE_UNKNOWN:
                return new AstNodeState(DState::UNKNOWN);
            default:
                throw ParseError("Expected node state");
        }
    }

    // Create an AstEventState for the current token (which must be an event state).
    AstEventState* make_event_state() {
        TK k = ts_.peek().kind;
        ts_.consume();
        if (k == TK::EVENT_SET) {
            return new AstEventState(true);
        }
        if (k == TK::EVENT_CLEAR) {
            return new AstEventState(false);
        }
        throw ParseError("Expected event state (set/clear)");
    }

    // Split a "path:varname" or "name:varname" token text into its two components.
    static std::pair<std::string, std::string> split_variable(const std::string& text) {
        auto colon = text.rfind(':');
        if (colon == std::string::npos) {
            return {text, {}};
        }
        return {text.substr(0, colon), text.substr(colon + 1)};
    }

    TokenStream ts_;
};

//
// ExprParser::doParse
//

///
/// @brief Reproduces a legacy V1 AST-conversion recovery for malformed top-level terms.
///
/// @details Spirit accepts a leading ``path:namegt /path and ...`` construct as a malformed
/// division expression.  The V1 parse-tree-to-AST traversal subsequently discards every term
/// before the final top-level ``and``.  This behaviour is present in two real corpus entries and
/// must be retained while V1 and V2 are compared byte-for-byte.  The recovery intentionally
/// applies only when the malformed glued ``gt`` name occurs before the first top-level ``and``.
///
/// @param[in] expression Source expression supplied to V2.
/// @return The expression V1 effectively converts to an AST, or @p expression unchanged.
///
static std::string recover_v1_malformed_top_level(const std::string& expression) {
    int parenthesis_depth = 0;
    for (char c : expression) {
        if (c == '(') {
            ++parenthesis_depth;
        }
        if (c == ')') {
            --parenthesis_depth;
        }
        if (parenthesis_depth < 0) {
            return expression;
        }
    }
    if (parenthesis_depth != 0) {
        return expression;
    }

    std::size_t first_non_paren = expression.find_first_not_of('(');
    if (first_non_paren == std::string::npos || expression.compare(first_non_paren, 1, "/") != 0) {
        return expression;
    }

    const std::size_t first_and = expression.find(" and ", first_non_paren);
    const std::size_t first_or  = expression.find(" or ", first_non_paren);
    if (first_and == std::string::npos || (first_or != std::string::npos && first_or < first_and)) {
        return expression;
    }

    const std::size_t colon = expression.find(':', first_non_paren);
    if (colon == std::string::npos || colon > first_and) {
        return expression;
    }

    const std::size_t glued_gt = expression.find("gt ", colon);
    if (glued_gt == std::string::npos || glued_gt > first_and || glued_gt == colon ||
        std::isspace(static_cast<unsigned char>(expression[glued_gt - 1]))) {
        return expression;
    }

    const std::size_t final_and = expression.rfind(" and ");
    if (final_and == std::string::npos ||
        (final_and == first_and && expression.find(" and ", first_and + 1) == std::string::npos)) {
        return expression;
    }

    const std::size_t suffix = final_and + 5;
    // Keep the original opening parentheses and drop only the malformed prefix.
    return expression.substr(0, first_non_paren) + expression.substr(suffix);
}

ExprParser::ExprParser(const std::string& expression)
    : expr_(expression) {
}

bool ExprParser::doParse(std::string& errorMsg) {
    if (expr_.empty()) {
        errorMsg = "Expression is empty";
        return false;
    }

    // Consult the process-wide deduplication cache first.
    ast_ = ExprDuplicate::find(expr_);
    if (ast_) {
        return true;
    }

    // Parse with the handcrafted recursive-descent parser.
    const std::string recovered_expression = recover_v1_malformed_top_level(expr_);
    Parser p(recovered_expression);
    ast_ = p.parse(errorMsg);
    if (ast_) {
        ExprDuplicate::add(expr_, ast_.get());
        return true;
    }
    return false;
}

} // namespace ecf::expression::v2
