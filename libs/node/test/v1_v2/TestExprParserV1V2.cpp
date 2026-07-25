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
/// @brief Differential harness for the trigger/complete expression parser migration.
///
/// This test compares the V1 (Boost.Spirit) and V2 (handcrafted) expression parsers side-by-side,
/// asserting that for every expression:
///   - parse success/failure is identical;
///   - the plain flat rendering (print_flat) is byte-for-byte identical;
///   - the fully-bracketed flat rendering (print_flat with add_brackets=true) is byte-for-byte identical.
///
/// In Phase 0 (test scaffolding), V2 is not yet implemented, so the harness validates V1 against
/// itself: every expression is parsed twice (with the ExprDuplicate cache flushed between runs) to
/// confirm that the wiring, corpus loading, and cache-flush logic work correctly.
///
/// The corpus is the same corpus.json used by u_expressions (2,156+ real-world expressions).
/// In addition, a compact canonical set of expressions from the existing TestExprParser.cpp is
/// verified to ensure all edge cases are covered independently of the corpus.
///

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/ExprDuplicate.hpp"
#include "ecflow/node/ExprParser.hpp"
#include "ecflow/node/ExprParserV2.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using json = nlohmann::ordered_json;

// ============================================================================
// Types
// ============================================================================

///
/// @brief Captures the complete result of parsing one expression string.
///
struct ParseResult
{
    bool accepted;          ///< true when the expression is accepted by the parser.
    std::string flat;       ///< print_flat() result; empty when @c accepted is false.
    std::string bracketed;  ///< print_flat(add_brackets=true) result; empty when @c accepted is false.
    std::string error;      ///< diagnostic message set when @c accepted is false.
};

// ============================================================================
// Helpers
// ============================================================================

///
/// @brief Parses @p expr with the V1 ExprParser and returns a full ParseResult.
///
/// @details The ExprDuplicate cache is flushed before each call so every invocation
/// is a deterministic first-parse (a cached hit would return a clone and AstNot::clone()
/// does not preserve the original not/~/! spelling).
///
/// @param[in] expr Trigger/complete expression string to parse.
/// @return A ParseResult describing whether parsing succeeded and the rendered AST.
///
static ParseResult parse_v1(const std::string& expr) {
    // Flush the process-wide duplicate-AST cache.
    ExprDuplicate reclaim;

    ParseResult result;
    ExprParser parser(expr);
    result.accepted = parser.doParse(result.error);
    if (result.accepted) {
        AstTop* ast = parser.getAst();
        if (ast == nullptr) {
            result.accepted = false;
            result.error    = "parser returned no AST";
        }
        else {
            std::ostringstream ss_flat;
            ast->print_flat(ss_flat, false);
            result.flat = ss_flat.str();

            std::ostringstream ss_bracketed;
            ast->print_flat(ss_bracketed, true);
            result.bracketed = ss_bracketed.str();
        }
    }
    return result;
}

///
/// @brief Parses @p expr with the V2 ExprParser and returns a full ParseResult.
///
/// @details The ExprDuplicate cache is flushed before each call so every invocation is a
/// deterministic first-parse.
///
/// @param[in] expr Trigger/complete expression string to parse.
/// @return A ParseResult describing whether parsing succeeded and the rendered AST.
///
static ParseResult parse_v2(const std::string& expr) {
    ExprDuplicate reclaim;

    ParseResult result;
    ecf::expression::v2::ExprParser parser(expr);
    result.accepted = parser.doParse(result.error);
    if (result.accepted) {
        AstTop* ast = parser.getAst();
        if (ast == nullptr) {
            result.accepted = false;
            result.error    = "V2 parser returned no AST";
        }
        else {
            std::ostringstream ss_flat;
            ast->print_flat(ss_flat, false);
            result.flat = ss_flat.str();

            std::ostringstream ss_bracketed;
            ast->print_flat(ss_bracketed, true);
            result.bracketed = ss_bracketed.str();
        }
    }
    return result;
}

///
/// @brief Loads and returns the JSON corpus from the test-data directory.
///
/// @return The loaded JSON object.
///
static json load_corpus() {
    const std::string path =
        ecf::File::test_data("libs/node/test/expressions/data/corpus.json", "expressions");
    std::ifstream in(path);
    BOOST_REQUIRE_MESSAGE(in.good(), "Cannot open corpus file: " << path);
    json j;
    in >> j;
    return j;
}

// ============================================================================
// Canonical expression list
// ============================================================================
// These are taken from TestExprParser.cpp (test_expression_parser_basic) and
// exercise every operand type and operator form independently of the corpus.

static const std::vector<std::string> k_canonical_expressions = {
    "a == complete",
    "a != complete",
    "a:value == 10",
    "a:value != 10",
    "a:value >= 10",
    "a:value <= 10",
    "a:value > 10",
    "a:value < 10",
    "1 == 1",
    "1 == 0",
    "a:event_name == set",
    "a:event_name != set",
    "a:event_name == clear",
    "a:event_name != clear",
    "../a/b:eventname == set",
    "../a/b:eventname == clear",
    "../a/b:eventname != clear",
    "../a:event_name >= 10",
    "a == unknown and b != complete",
    "a == unknown or b != complete",
    "a == complete and b == complete or c == complete",
    "! a == unknown",
    "/mc/main:YMD <= /mc/main/ref:MC_STOP",
    "! ../../../prod2diss/operation_is_late:yes == set or ! a == complete",
    "./a:YMD - ./b:YMD < 5",
    "./a:YMD + ./b:YMD < 5",
    "./a:YMD / ./b:YMD < 5",
    "./a:YMD * ./b:YMD < 5",
    "./a:YMD % ./b:YMD < 5",
    "inigroup:YMD == ! 1",
    "inigroup:YMD == ! 0",
    "comp == complete and notready == complete",
    "comp == complete and not ready == complete",
    "comp == complete and ! ready == complete",
    "comp == complete and ~ ready == complete",
    ":VAR == 1",
    ":VAR == /mc/main/ref:MC_STOP",
    ":YMD - :YMD < 5",
    ":YMD + :YMD < 5",
    ":YMD / :YMD < 5",
    ":YMD * :YMD < 5",
    ":YMD % :YMD < 5",
    ":YYYYMMDDThhmmss == 20230101T000000",
    ":YYYYMMDDThhmmss >= 20230101T000000",
    ":YYYYMMDDThhmmss <= 20230101T000000",
    ":YYYYMMDDThhmmss + 3 <= 19700101T000000",
    ":YYYYMMDDThhmmss <= 19700101T000000 + 3",
    ":YYYYMMDDThhmmss >= 19700101T000000 + 3",
};

// ============================================================================
// Test suites
// ============================================================================

BOOST_AUTO_TEST_SUITE(U_ExprParserV1V2)

///
/// @brief Phase 0 consistency check: the canonical expression list parses reproducibly with V1.
///
/// The cache is flushed before each parse, so the test asserts that two independent parses of the
/// same expression with V1 produce byte-identical output. This validates cache-flush wiring and
/// the ParseResult capture logic. The test is the scaffold for the V1-vs-V2 comparison added in
/// Phase 2.
///
BOOST_AUTO_TEST_CASE(test_v1_canonical_consistency) {
    ECF_NAME_THIS_TEST();

    std::size_t failures = 0;
    for (const auto& expr : k_canonical_expressions) {
        const ParseResult r1 = parse_v1(expr);
        const ParseResult r2 = parse_v1(expr);

        BOOST_CHECK_MESSAGE(r1.accepted,
                            "V1 failed to parse canonical expression: '" << expr << "'  (" << r1.error << ")");
        if (!r1.accepted) {
            ++failures;
            continue;
        }

        BOOST_CHECK_MESSAGE(r1.accepted == r2.accepted,
                            "V1 parse outcome not deterministic for: '" << expr << "'");
        BOOST_CHECK_MESSAGE(r1.flat == r2.flat,
                            "V1 plain flat not deterministic for: '" << expr
                                                                     << "'\n  run1: " << r1.flat
                                                                     << "\n  run2: " << r2.flat);
        BOOST_CHECK_MESSAGE(r1.bracketed == r2.bracketed,
                            "V1 bracketed flat not deterministic for: '" << expr
                                                                         << "'\n  run1: " << r1.bracketed
                                                                         << "\n  run2: " << r2.bracketed);
        if (r1.flat != r2.flat || r1.bracketed != r2.bracketed)
            ++failures;
    }

    BOOST_REQUIRE_MESSAGE(failures == 0,
                          failures << " canonical expression(s) failed V1 consistency check");
}

///
/// @brief Phase 0 consistency check: all valid corpus expressions parse reproducibly with V1.
///
/// Mirrors test_v1_canonical_consistency but operates on the full golden corpus, proving
/// that corpus loading and the cache-flush mechanism work correctly for all 2,156+ expressions.
///
BOOST_AUTO_TEST_CASE(test_v1_corpus_valid_consistency) {
    ECF_NAME_THIS_TEST();

    json corpus             = load_corpus();
    const auto& entries     = corpus.at("valid_expressions");
    std::size_t checked     = 0;
    std::size_t failures    = 0;

    for (const auto& entry : entries) {
        const std::string expr = entry.at("expression").get<std::string>();

        const ParseResult r1 = parse_v1(expr);
        const ParseResult r2 = parse_v1(expr);

        if (!r1.accepted) {
            BOOST_ERROR("V1 failed to parse valid corpus expression: '" << expr << "'  (" << r1.error << ")");
            ++failures;
            continue;
        }

        bool ok = (r1.accepted == r2.accepted) && (r1.flat == r2.flat) && (r1.bracketed == r2.bracketed);
        if (!ok) {
            BOOST_ERROR("V1 not deterministic for corpus expression: '" << expr << "'");
            ++failures;
        }
        ++checked;
    }

    BOOST_TEST_MESSAGE("Corpus valid: checked " << checked << " expression(s), " << failures << " failure(s)");
    BOOST_REQUIRE_MESSAGE(failures == 0,
                          failures << " valid corpus expression(s) failed V1 consistency check");
}

///
/// @brief Phase 0 consistency check: all invalid corpus expressions are rejected by V1.
///
/// Verifies that the rejection corpus is loaded and that V1 correctly rejects every entry,
/// confirming the harness correctly captures parse failures.
///
BOOST_AUTO_TEST_CASE(test_v1_corpus_invalid_rejection) {
    ECF_NAME_THIS_TEST();

    json corpus                    = load_corpus();
    const auto& rejections         = corpus.at("invalid_expressions");
    std::size_t checked            = 0;
    std::size_t unexpected_accepts = 0;

    for (const auto& entry : rejections) {
        const std::string expr = entry.at("expression").get<std::string>();

        const ParseResult r = parse_v1(expr);
        if (r.accepted) {
            BOOST_ERROR("V1 accepted an invalid corpus expression: '" << expr << "'  rendered: " << r.flat);
            ++unexpected_accepts;
        }
        ++checked;
    }

    BOOST_TEST_MESSAGE("Corpus invalid: checked " << checked << " expression(s), "
                                                  << unexpected_accepts << " unexpectedly accepted");
    BOOST_REQUIRE_MESSAGE(unexpected_accepts == 0,
                          unexpected_accepts << " invalid corpus expression(s) were incorrectly accepted by V1");
}

// ============================================================================
// V1 vs V2 differential tests
// ============================================================================

///
/// @brief Differential test: V1 and V2 must produce identical results for all canonical expressions.
///
/// For each expression, both parsers are run (with the cache flushed between calls) and their
/// accept/reject outcome, plain flat rendering, and fully-bracketed flat rendering are compared
/// byte-for-byte.
///
BOOST_AUTO_TEST_CASE(test_v1_v2_canonical_differential) {
    ECF_NAME_THIS_TEST();

    std::size_t failures = 0;
    for (const auto& expr : k_canonical_expressions) {
        const ParseResult r1 = parse_v1(expr);
        const ParseResult r2 = parse_v2(expr);

        bool ok = (r1.accepted == r2.accepted);
        if (r1.accepted && r2.accepted) {
            ok = ok && (r1.flat == r2.flat) && (r1.bracketed == r2.bracketed);
        }

        if (!ok) {
            ++failures;
            BOOST_ERROR("V1/V2 mismatch for canonical expression: '" << expr << "'"
                        << "\n  V1 accepted=" << r1.accepted << " flat='" << r1.flat << "'"
                        << "\n  V2 accepted=" << r2.accepted << " flat='" << r2.flat << "'"
                        << (r1.accepted != r2.accepted ? "\n  error: " + (r1.accepted ? r2.error : r1.error) : ""));
        }
    }

    BOOST_REQUIRE_MESSAGE(failures == 0,
                          failures << " canonical expression(s) show V1/V2 mismatch");
}

///
/// @brief Differential test: V1 and V2 must produce identical results for all valid corpus expressions.
///
BOOST_AUTO_TEST_CASE(test_v1_v2_corpus_valid_differential) {
    ECF_NAME_THIS_TEST();

    json corpus             = load_corpus();
    const auto& entries     = corpus.at("valid_expressions");
    std::size_t checked     = 0;
    std::size_t failures    = 0;

    for (const auto& entry : entries) {
        const std::string expr = entry.at("expression").get<std::string>();

        const ParseResult r1 = parse_v1(expr);
        const ParseResult r2 = parse_v2(expr);

        bool ok = (r1.accepted == r2.accepted);
        if (r1.accepted && r2.accepted) {
            ok = ok && (r1.flat == r2.flat) && (r1.bracketed == r2.bracketed);
        }

        if (!ok) {
            ++failures;
            if (failures <= 10) {
                BOOST_ERROR("V1/V2 mismatch for corpus expression: '" << expr << "'"
                            << "\n  V1 accepted=" << r1.accepted << " flat='" << r1.flat << "'"
                            << "\n  V2 accepted=" << r2.accepted << " flat='" << r2.flat << "'");
            }
        }
        ++checked;
    }

    BOOST_TEST_MESSAGE("V1/V2 corpus valid: compared " << checked << " expression(s), " << failures << " mismatch(es)");
    BOOST_REQUIRE_MESSAGE(failures == 0,
                          failures << " valid corpus expression(s) show V1/V2 mismatch");
}

///
/// @brief Differential test: V1 and V2 must agree on rejecting all invalid corpus expressions.
///
BOOST_AUTO_TEST_CASE(test_v1_v2_corpus_invalid_differential) {
    ECF_NAME_THIS_TEST();

    json corpus                 = load_corpus();
    const auto& rejections      = corpus.at("invalid_expressions");
    std::size_t checked         = 0;
    std::size_t failures        = 0;

    for (const auto& entry : rejections) {
        const std::string expr = entry.at("expression").get<std::string>();

        const ParseResult r1 = parse_v1(expr);
        const ParseResult r2 = parse_v2(expr);

        // Both must reject.  A V2 acceptance when V1 rejects is a regression.
        if (r2.accepted && !r1.accepted) {
            ++failures;
            BOOST_ERROR("V2 accepted an expression that V1 rejected (invalid corpus): '" << expr
                        << "'  V2 flat: " << r2.flat);
        }
        // A V2 rejection when V1 accepts is also a mismatch (parser became too strict).
        if (!r2.accepted && r1.accepted) {
            ++failures;
            BOOST_ERROR("V2 rejected an expression that V1 accepted (invalid corpus): '" << expr
                        << "'  V2 error: " << r2.error);
        }
        ++checked;
    }

    BOOST_TEST_MESSAGE("V1/V2 corpus invalid: compared " << checked << " expression(s), "
                                                         << failures << " mismatch(es)");
    BOOST_REQUIRE_MESSAGE(failures == 0,
                          failures << " invalid corpus expression(s) show V1/V2 accept/reject mismatch");
}

BOOST_AUTO_TEST_SUITE_END()
