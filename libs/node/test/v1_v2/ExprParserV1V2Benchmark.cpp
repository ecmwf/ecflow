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
/// @brief Benchmarks V1 and V2 expression parsers using checkpoint-derived JSON datasets.
///
/// The extractor extract_checkpoint_expression_sets.py records raw expression attribute parts
/// in checkpoint source order. This tool compares V1 and V2 in two modes for every JSON file:
///
///   - full: the original ordered sequence, with ExprDuplicate active. This measures realistic
///     load-time behaviour including duplicate-expression cache hits.
///   - unique: first occurrence of each expression only, with no cache hits. This measures the
///     cost of parsing distinct expressions.
///
/// AST equivalence is validated before timing. Every timed V1/V2 pass starts with an empty cache,
/// ensuring that neither implementation uses entries inserted by the other.
///

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/ExprDuplicate.hpp"
#include "ecflow/node/ExprParserV1.hpp"
#include "ecflow/node/ExprParserV2.hpp"

namespace fs = std::filesystem;
using json   = nlohmann::json;

namespace {

///
/// @brief Captures one parser invocation's acceptance status and AST renderings.
///
struct ParseResult
{
    bool accepted;
    std::string flat;
    std::string bracketed;
    std::string error;
};

///
/// @brief Captures elapsed parser-pass time and the number of accepted expressions parsed.
///
struct Timing
{
    double seconds;
    std::size_t parsed;
};

///
/// @brief Clears the process-global duplicate expression cache.
///
void clear_expression_cache() {
    ExprDuplicate reclaim;
}

///
/// @brief Parses and renders one expression with the given parser type.
///
/// @tparam ParserT V1 or V2 expression parser type.
/// @param[in] expression Expression part to parse.
/// @return Parse outcome and both flat AST renderings.
///
template <typename ParserT>
ParseResult parse_expression(const std::string& expression) {
    ParseResult result;
    ParserT parser(expression);
    result.accepted = parser.doParse(result.error);
    if (!result.accepted) {
        return result;
    }

    AstTop* ast = parser.getAst();
    if (ast == nullptr) {
        result.accepted = false;
        result.error    = "parser returned no AST";
        return result;
    }

    std::ostringstream flat;
    ast->print_flat(flat, false);
    result.flat = flat.str();

    std::ostringstream bracketed;
    ast->print_flat(bracketed, true);
    result.bracketed = bracketed.str();
    return result;
}

///
/// @brief Filters expressions accepted identically by V1 and V2 outside timed regions.
///
/// @param[in] expressions Dataset expression parts in source order.
/// @param[in] dataset Dataset path used in diagnostics.
/// @param[out] accepted Expression parts accepted by both parsers, preserving duplicates and order.
/// @param[out] rejected Count of expression parts rejected by both parsers.
/// @return true when V1 and V2 agree on every expression; false on an acceptance or AST mismatch.
///
bool filter_accepted_expressions(const std::vector<std::string>& expressions,
                                 const fs::path& dataset,
                                 std::vector<std::string>& accepted,
                                 std::size_t& rejected) {
    accepted.clear();
    accepted.reserve(expressions.size());
    rejected = 0;
    for (const auto& expression : expressions) {
        clear_expression_cache();
        const ParseResult v1 = parse_expression<ecf::expression::v1::ExprParser>(expression);
        clear_expression_cache();
        const ParseResult v2 = parse_expression<ecf::expression::v2::ExprParser>(expression);

        if (v1.accepted != v2.accepted || (v1.accepted && (v1.flat != v2.flat || v1.bracketed != v2.bracketed))) {
            std::cerr << "V1/V2 mismatch in " << dataset << "\n  expression: " << expression
                      << "\n  V1 accepted=" << v1.accepted << " flat='" << v1.flat << "' error='" << v1.error
                      << "'\n  V2 accepted=" << v2.accepted << " flat='" << v2.flat << "' error='" << v2.error << "'\n";
            return false;
        }
        if (v1.accepted) {
            accepted.push_back(expression);
        }
        else {
            ++rejected;
        }
    }
    return true;
}

///
/// @brief Times a complete parser pass with a newly empty expression cache.
///
/// @tparam ParserT V1 or V2 expression parser type.
/// @param[in] expressions Ordered expression parts to parse.
/// @param[out] error First diagnostic produced when parsing fails.
/// @return Elapsed time and successful parse count.
///
template <typename ParserT>
Timing time_parser(const std::vector<std::string>& expressions, std::string& error) {
    clear_expression_cache();
    ExprDuplicate cache_lifetime;

    const auto start   = std::chrono::steady_clock::now();
    std::size_t parsed = 0;
    for (const auto& expression : expressions) {
        ParserT parser(expression);
        if (!parser.doParse(error)) {
            return {0.0, parsed};
        }
        ++parsed;
    }
    const auto end       = std::chrono::steady_clock::now();
    const double seconds = std::chrono::duration<double>(end - start).count();
    return {seconds, parsed};
}

///
/// @brief Returns first-occurrence deduplicated expression parts while preserving source order.
///
/// @param[in] expressions Full checkpoint expression sequence.
/// @return Ordered sequence containing each distinct expression exactly once.
///
std::vector<std::string> unique_expressions(const std::vector<std::string>& expressions) {
    std::set<std::string> seen;
    std::vector<std::string> unique;
    unique.reserve(expressions.size());
    for (const auto& expression : expressions) {
        if (seen.insert(expression).second) {
            unique.push_back(expression);
        }
    }
    return unique;
}

///
/// @brief Loads raw expression parts from a checkpoint JSON dataset.
///
/// @param[in] dataset JSON file emitted by extract_checkpoint_expression_sets.py.
/// @return Expression strings in the checkpoint's original source order.
///
std::vector<std::string> load_dataset(const fs::path& dataset) {
    std::ifstream input(dataset);
    if (!input.good()) {
        throw std::runtime_error("Cannot open dataset: " + dataset.string());
    }
    json document;
    input >> document;

    std::vector<std::string> expressions;
    for (const auto& entry : document.at("expressions")) {
        expressions.push_back(entry.at("expression").get<std::string>());
    }
    return expressions;
}

///
/// @brief Formats parser timing as expressions per second.
///
/// @param[in] timing Measured elapsed time and expression count.
/// @return Throughput, or zero when no time elapsed.
///
double throughput(const Timing& timing) {
    return timing.seconds == 0.0 ? 0.0 : static_cast<double>(timing.parsed) / timing.seconds;
}

///
/// @brief Prints one benchmark result row.
///
/// @param[in] dataset Dataset path.
/// @param[in] mode Full ordered or deduplicated input mode.
/// @param[in] extracted Raw expression count extracted from the checkpoint.
/// @param[in] parsed Accepted expression count used for this benchmark mode.
/// @param[in] rejected Expression count rejected identically by V1 and V2 during preflight.
/// @param[in] unique Distinct expression count.
/// @param[in] v1 V1 timing.
/// @param[in] v2 V2 timing.
///
void print_row(const fs::path& dataset,
               const char* mode,
               std::size_t extracted,
               std::size_t parsed,
               std::size_t rejected,
               std::size_t unique,
               const Timing& v1,
               const Timing& v2) {
    const double ratio = v1.seconds == 0.0 ? 0.0 : v2.seconds / v1.seconds;
    std::cout << "| " << dataset.filename().string() << " | " << mode << " | " << extracted << " | " << parsed << " | "
              << rejected << " | " << unique << " | " << std::fixed << std::setprecision(6) << v1.seconds << " | "
              << v2.seconds << " | " << std::setprecision(0) << throughput(v1) << " | " << throughput(v2) << " | "
              << std::setprecision(3) << ratio << " |\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 3 || std::string(argv[1]) != "--data-dir") {
        std::cerr << "Usage: " << argv[0] << " --data-dir <checkpoint-expression-json-directory>\n";
        return 2;
    }

    const fs::path data_dir(argv[2]);
    if (!fs::is_directory(data_dir)) {
        std::cerr << "Dataset directory does not exist: " << data_dir << "\n";
        return 2;
    }

    std::vector<fs::path> datasets;
    for (const auto& entry : fs::recursive_directory_iterator(data_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            datasets.push_back(entry.path());
        }
    }
    std::sort(datasets.begin(), datasets.end());
    if (datasets.empty()) {
        std::cerr << "No JSON datasets found under " << data_dir << "\n";
        return 2;
    }

    std::cout << "| Dataset | Mode | Extracted | Parsed | Rejected | Distinct | V1 seconds | V2 seconds | V1 expr/s | "
                 "V2 expr/s | V2/V1 |\n";
    std::cout << "| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n";

    for (const auto& dataset : datasets) {
        const std::vector<std::string> full = load_dataset(dataset);
        if (full.empty()) {
            continue;
        }
        const std::vector<std::string> raw_unique = unique_expressions(full);
        std::vector<std::string> accepted_unique;
        std::size_t rejected_unique = 0;
        // Equivalence is independent of duplicate ordering, so preflight each distinct raw
        // expression once. Expressions rejected by both implementations remain in JSON for
        // auditability but cannot be part of a parser-throughput measurement.
        if (!filter_accepted_expressions(raw_unique, dataset, accepted_unique, rejected_unique)) {
            return 1;
        }

        std::set<std::string> accepted_set(accepted_unique.begin(), accepted_unique.end());
        std::vector<std::string> accepted_full;
        accepted_full.reserve(full.size());
        for (const auto& expression : full) {
            if (accepted_set.count(expression) != 0) {
                accepted_full.push_back(expression);
            }
        }

        std::string error;
        const Timing full_v1 = time_parser<ecf::expression::v1::ExprParser>(accepted_full, error);
        if (!error.empty()) {
            std::cerr << "V1 failed while timing " << dataset << ": " << error << "\n";
            return 1;
        }
        error.clear();
        const Timing full_v2 = time_parser<ecf::expression::v2::ExprParser>(accepted_full, error);
        if (!error.empty()) {
            std::cerr << "V2 failed while timing " << dataset << ": " << error << "\n";
            return 1;
        }
        print_row(dataset,
                  "full-cache-active",
                  full.size(),
                  accepted_full.size(),
                  full.size() - accepted_full.size(),
                  accepted_unique.size(),
                  full_v1,
                  full_v2);

        error.clear();
        const Timing unique_v1 = time_parser<ecf::expression::v1::ExprParser>(accepted_unique, error);
        if (!error.empty()) {
            std::cerr << "V1 failed while timing distinct expressions in " << dataset << ": " << error << "\n";
            return 1;
        }
        error.clear();
        const Timing unique_v2 = time_parser<ecf::expression::v2::ExprParser>(accepted_unique, error);
        if (!error.empty()) {
            std::cerr << "V2 failed while timing distinct expressions in " << dataset << ": " << error << "\n";
            return 1;
        }
        print_row(dataset,
                  "distinct-cache-cold",
                  full.size(),
                  accepted_unique.size(),
                  rejected_unique,
                  accepted_unique.size(),
                  unique_v1,
                  unique_v2);
    }
    return 0;
}
