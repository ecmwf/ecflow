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
/// @brief This tests checks the observable behaviour of the trigger/complete expression parser.
///
/// The validation is initially based on parsing each corpus expression and rendering the resulting
/// abstract syntax tree as a fully-bracketed flat string. That rendering is then pinned so that any
/// change altering it is caught as a regression.
///
/// The corpus is *external* (``data/corpus.json``) and is produced by the companion Python script
/// ``extract_expressions``, which retrieves every trigger/complete expression from real definition
/// (``.def``) and checkpoint (``.check``) files. This script takes the expression textually, combines
/// multi-part expressions and deduplicates by AST structure. It also derives a set of invalid expressions
/// by mutating the valid ones in ways that break their structure (e.g., dropping a parenthesis, swapping
/// an operator, etc.).
///
/// Workflow:
///
///   1. Generate/refresh the expressions with ``extract_expressions``.
///
///   2. Fill/refresh the golden ``expected`` values and classify the rejection candidates by running
///      this test with the environment variable ``ECF_GOLDEN_CORPUS=1`` set.
///      This (re)parses every expression with the current parser and rewrites ``data/corpus.json`` in place.
///
///   3. Adopt/commit ``data/corpus.json``.
///
///      Normal test runs then assert that the parser:
///       - reproduces every golden value byte-for-byte
///       - rejects every invalid expression.
///
/// The ``render_expression`` function below is the single point that talks to the parser,
/// so the corpus is checked against one well-defined rendering of the AST.
///

#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/ExprDuplicate.hpp"
#include "ecflow/node/ExprParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

// Preserve the corpus key order (metadata, valid_expressions, invalid_expressions)
// when the recording step rewrites the file.
using json = nlohmann::ordered_json;

BOOST_AUTO_TEST_SUITE(U_Expressions)

BOOST_AUTO_TEST_SUITE(T_GoldenCorpus)

namespace {

std::string corpus_path() {
    return ecf::File::test_data("libs/node/test/expressions/data/corpus.json", "expressions");
}

json load_corpus(const std::string& path) {
    std::ifstream in(path);
    BOOST_REQUIRE_MESSAGE(in.good(), "Cannot open corpus file: " << path);
    json j;
    in >> j;
    return j;
}

///
/// @brief Parses an expression and renders its AST as a fully-bracketed flat string.
///
/// @param[in]  expr      Trigger/complete expression to parse.
/// @param[out] bracketed Fully-bracketed flat rendering of the resulting AST (the golden form).
/// @param[out] error     Diagnostic message, set only when parsing fails.
/// @return true when @p expr is accepted and rendered; false otherwise.
///
bool render_expression(const std::string& expr, std::string& bracketed, std::string& error) {
    // Clear the process-wide duplicate-AST cache so every call is a deterministic
    // "first parse" (a cached hit would return a clone, and AstNot::clone() does
    // not preserve the 'not'/'~'/'!' spelling).
    ExprDuplicate reclaim;

    ExprParser parser(expr);
    if (!parser.doParse(error)) {
        return false;
    }
    AstTop* ast = parser.getAst();
    if (!ast) {
        error = "parser returned no AST";
        return false;
    }
    std::ostringstream ss;
    ast->print_flat(ss, true /*add_brackets*/);
    bracketed = ss.str();
    return true;
}

} // namespace

//
// (Re)Generate corpus values.
//
// This is only activated if ECF_GOLDEN_CORPUS=1.
//
// It effectively iterates over all valid expressions and records the fully-bracketed flat rendering of its AST.
//
BOOST_AUTO_TEST_CASE(regenerate_golden_corpus) {
    ECF_NAME_THIS_TEST();

    auto regenerate_requested = []() {
        const char* flag = ::getenv("ECF_GOLDEN_CORPUS");
        return flag != nullptr && std::string(flag) != "0";
    };

    if (!regenerate_requested()) {
        BOOST_TEST_MESSAGE("Set ECF_GOLDEN_CORPUS=1 to (re)generate the golden 'expected' values");
        return;
    }

    const std::string path = corpus_path();
    json corpus            = load_corpus(path);

    auto make_valid_entry = [](const std::string& expression, const std::string& expected) {
        json entry          = json::object();
        entry["expression"] = expression;
        entry["expected"]   = expected;
        return entry;
    };

    json valid_out = json::array();
    std::set<std::string> valid_seen;
    std::size_t valid_dropped = 0;
    std::vector<std::string> dropped_samples;

    // Re-record the golden rendering of every valid expression, dropping any the
    // parser no longer accepts.
    for (const auto& entry : corpus.at("valid_expressions")) {
        const std::string expression = entry.at("expression").get<std::string>();

        std::string bracketed;
        std::string error;
        if (!render_expression(expression, bracketed, error)) {
            ++valid_dropped;
            if (dropped_samples.size() < 20) {
                dropped_samples.push_back(expression + "  [" + error + "]");
            }
            continue;
        }
        if (valid_seen.insert(expression).second) {
            valid_out.push_back(make_valid_entry(expression, bracketed));
        }
    }

    // Classify the automatically-derived invalid-expression candidates. Those the
    // parser rejects are enforced rejections. Those the parser accepts are not, in
    // fact, invalid: move them into valid_expressions with their golden rendering, so
    // the behaviour is documented and locked rather than silently tolerated.
    json invalid_out     = json::array();
    std::size_t enforced = 0;
    std::size_t moved    = 0;
    std::vector<std::string> moved_samples;

    if (corpus.contains("invalid_expressions")) {
        for (const auto& entry : corpus.at("invalid_expressions")) {
            const std::string expression = entry.at("expression").get<std::string>();

            std::string bracketed;
            std::string error;
            if (render_expression(expression, bracketed, error)) {
                // Accepted: this is a valid expression -> lock it in valid_expressions.
                if (valid_seen.insert(expression).second) {
                    valid_out.push_back(make_valid_entry(expression, bracketed));
                    ++moved;
                    if (moved_samples.size() < 20) {
                        moved_samples.push_back(expression);
                    }
                }
            }
            else {
                // Rejected as expected: keep it as an enforced rejection.
                json out = entry;
                out.erase("skip");
                out.erase("reason");
                invalid_out.push_back(out);
                ++enforced;
            }
        }
    }

    corpus["valid_expressions"]                                = valid_out;
    corpus["invalid_expressions"]                              = invalid_out;
    corpus["metadata"]["recorded"]                             = json::object();
    corpus["metadata"]["recorded"]["valid_expressions"]        = valid_out.size();
    corpus["metadata"]["recorded"]["valid_dropped"]            = valid_dropped;
    corpus["metadata"]["recorded"]["valid_moved_from_invalid"] = moved;
    corpus["metadata"]["recorded"]["invalid_expressions"]      = invalid_out.size();

    {
        std::ofstream os(path);
        BOOST_REQUIRE_MESSAGE(os.good(), "Cannot write corpus file: " << path);
        os << corpus.dump(2) << "\n";
    }

    BOOST_TEST_MESSAGE("Valid: " << valid_out.size() << " (dropped " << valid_dropped << ", moved " << moved
                                 << " from invalid); Invalid enforced: " << enforced);
    for (const auto& sample : dropped_samples) {
        BOOST_TEST_MESSAGE("  dropped: " << sample);
    }
    for (const auto& sample : moved_samples) {
        BOOST_TEST_MESSAGE("  moved to valid (accepted by parser): " << sample);
    }
}

//
// The "golden" assertion.
// Every expression in the corpus must parse and reproduce the expected/golden value.
//
BOOST_AUTO_TEST_CASE(test_golden_corpus) {
    ECF_NAME_THIS_TEST();

    const std::string path = corpus_path();
    json corpus            = load_corpus(path);

    const auto& entries = corpus.at("valid_expressions");
    BOOST_REQUIRE_MESSAGE(!entries.empty(), "Golden corpus is empty: " << path);

    std::size_t checked        = 0;
    std::size_t parse_failures = 0;
    std::size_t mismatches     = 0;

    for (const auto& entry : entries) {
        const std::string expression = entry.at("expression").get<std::string>();

        BOOST_REQUIRE_MESSAGE(entry.contains("expected") && entry.at("expected").is_string(),
                              "Corpus entry has no golden 'expected' value (run with ECF_GOLDEN_CORPUS=1): '"
                                  << expression << "'");
        const std::string expected = entry.at("expected").get<std::string>();

        std::string bracketed;
        std::string error;
        if (!render_expression(expression, bracketed, error)) {
            ++parse_failures;
            BOOST_ERROR("Parser rejected golden corpus expression: '" << expression << "'  (" << error << ")");
            continue;
        }

        if (bracketed != expected) {
            ++mismatches;
            BOOST_ERROR("AST rendering changed for: '" << expression << "'\n  expected: " << expected
                                                       << "\n  actual:   " << bracketed);
        }
        ++checked;
    }

    BOOST_TEST_MESSAGE("Golden corpus: checked " << checked << " expression(s), " << parse_failures
                                                 << " parse failure(s), " << mismatches << " mismatch(es)");
    BOOST_REQUIRE_MESSAGE(parse_failures == 0 && mismatches == 0,
                          "Golden corpus regressions: " << parse_failures << " parse failure(s), " << mismatches
                                                        << " mismatch(es)");
}

//
// The rejection assertion.
//
// Every expression in the rejection corpus is invalid and must be rejected by the parser.
// A change that starts accepting one of them signals a parser that became too permissive.
//
BOOST_AUTO_TEST_CASE(test_rejected_corpus) {
    ECF_NAME_THIS_TEST();

    const std::string path = corpus_path();
    json corpus            = load_corpus(path);

    BOOST_REQUIRE_MESSAGE(corpus.contains("invalid_expressions"),
                          "Corpus has no 'invalid_expressions' (run with ECF_GOLDEN_CORPUS=1): " << path);
    const auto& rejections = corpus.at("invalid_expressions");
    BOOST_REQUIRE_MESSAGE(!rejections.empty(), "Rejection corpus is empty: " << path);

    std::size_t checked            = 0;
    std::size_t unexpected_accepts = 0;

    for (const auto& entry : rejections) {
        const std::string expression = entry.at("expression").get<std::string>();

        std::string bracketed;
        std::string error;
        if (render_expression(expression, bracketed, error)) {
            ++unexpected_accepts;
            BOOST_ERROR("Parser accepted an invalid expression: '" << expression << "'\n  rendered as: " << bracketed);
        }
        ++checked;
    }

    BOOST_TEST_MESSAGE("Rejection corpus: checked " << checked << " invalid expression(s), " << unexpected_accepts
                                                    << " unexpectedly accepted");
    BOOST_REQUIRE_MESSAGE(unexpected_accepts == 0,
                          "Rejection corpus regressions: " << unexpected_accepts
                                                           << " invalid expression(s) were accepted");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
