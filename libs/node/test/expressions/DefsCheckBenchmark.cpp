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
/// @brief Measures Defs::check for one parser-selected ecflow library variant.
///
/// The same source is compiled twice, once against ecflow_expr_v1 and once against
/// ecflow_expr_v2. ExprParser.hpp selects the corresponding implementation through
/// ECFLOW_USING_PARSER_V1 or ECFLOW_USING_PARSER_V2 when the library is compiled.
///
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ExprDuplicate.hpp"

namespace fs = std::filesystem;
using json   = nlohmann::json;

namespace {

struct Dataset
{
    fs::path checkpoint;
    std::size_t expression_count;
};

struct Result
{
    double seconds;
    bool checked;
    std::string error;
};

std::vector<Dataset> load_datasets(const fs::path& data_dir, const fs::path& root) {
    std::vector<Dataset> datasets;
    for (const auto& entry : fs::recursive_directory_iterator(data_dir)) {
        if (entry.path().extension() != ".json") {
            continue;
        }
        std::ifstream input(entry.path());
        json document;
        input >> document;
        const auto count = document.at("expression_count").get<std::size_t>();
        if (count == 0) {
            continue;
        }
        fs::path checkpoint = document.at("checkpoint").get<std::string>();
        if (!checkpoint.is_absolute()) {
            checkpoint = root / checkpoint;
        }
        std::set<std::string> distinct_expressions;
        for (const auto& expression : document.at("expressions")) {
            distinct_expressions.insert(expression.at("expression").get<std::string>());
        }
        if (distinct_expressions.size() < 10) {
            continue;
        }
        datasets.push_back({checkpoint, count});
    }
    std::sort(datasets.begin(), datasets.end(), [](const Dataset& lhs, const Dataset& rhs) {
        return lhs.checkpoint < rhs.checkpoint;
    });
    return datasets;
}

Result check_defs(const fs::path& checkpoint) {
    try {
        Defs defs;
        defs.restore(checkpoint.string());
        std::string error;
        std::string warning;
        const auto start   = std::chrono::steady_clock::now();
        const bool checked = defs.check(error, warning);
        const auto end     = std::chrono::steady_clock::now();
        return {std::chrono::duration<double>(end - start).count(), checked, error};
    }
    catch (const std::exception& exception) {
        return {0.0, false, exception.what()};
    }
}

void clear_cache() {
    ExprDuplicate reclaim;
}

void print_results(const std::vector<Dataset>& datasets,
                   const std::vector<Result>& cold,
                   const std::vector<Result>& warm) {
    std::cout << "| Checkpoint | Expressions | Cold seconds | Warm seconds | Warm/cold | Cold | Warm |\n";
    std::cout << "| --- | ---: | ---: | ---: | ---: | --- | --- |\n";
    for (std::size_t i = 0; i < datasets.size(); ++i) {
        const double ratio = cold[i].seconds == 0.0 ? 0.0 : warm[i].seconds / cold[i].seconds;
        std::cout << "| " << datasets[i].checkpoint.filename().string() << " | " << datasets[i].expression_count
                  << " | " << std::fixed << std::setprecision(6) << cold[i].seconds << " | " << warm[i].seconds << " | "
                  << std::setprecision(3) << ratio << " | " << (cold[i].checked ? "pass" : "fail") << " | "
                  << (warm[i].checked ? "pass" : "fail") << " |\n";
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 3 || std::string(argv[1]) != "--data-dir") {
        std::cerr << "Usage: " << argv[0] << " --data-dir <checkpoint-expression-json-directory>\n";
        return 2;
    }
    const fs::path data_dir(argv[2]);
    const auto datasets = load_datasets(data_dir, data_dir.parent_path().parent_path());
    std::vector<Result> cold;
    cold.reserve(datasets.size());
    for (const auto& dataset : datasets) {
        clear_cache();
        cold.push_back(check_defs(dataset.checkpoint));
    }

    clear_cache();
    ExprDuplicate warm_cache_lifetime;
    std::vector<Result> warm;
    warm.reserve(datasets.size());
    for (const auto& dataset : datasets) {
        warm.push_back(check_defs(dataset.checkpoint));
    }

    print_results(datasets, cold, warm);
    return 0;
}
