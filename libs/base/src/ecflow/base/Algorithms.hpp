/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_Algorithms_hpp
#define ecflow_base_Algorithms_hpp

#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/core/Result.hpp"
#include "ecflow/node/Defs.hpp"

namespace ecf {

///
/// Represents a path in the server's hierarchy
///
/// A path object is always represents a valid path (even if it does not exist).
///
/// A path with 0 tokens represents the root of the hierarchy (represented by a slash, "/").
/// A path with N tokens represents a path from the root to a node.
///
/// Multiple consecutive separators (i.e. slashes, "/") are treated as a single slash.
///
struct Path
{
    static Result<Path> make(const std::string& path) {
        if (path.empty()) {
            return Result<Path>::failure("Invalid path: '" + path + "' (cannot be empty)");
        }

        if (path == "/") {
            return Result<Path>::success(Path(std::vector<std::string>()));
        }

        std::vector<std::string> tokens;
        boost::tokenizer<boost::char_separator<char>> tokenizer(path, boost::char_separator<char>("/ "));
        for (const auto& token : tokenizer) {
            tokens.push_back(token);
        }
        return Result<Path>::success(Path(std::move(tokens)));
    }

    [[nodiscard]] std::string to_string() const {
        if (tokens_.empty()) {
            return "/";
        }

        std::string result;
        for (auto&& token : tokens_) {
            result += '/';
            result += token;
        }
        return result;
    }

    [[nodiscard]] bool empty() const { return tokens_.empty(); }
    [[nodiscard]] size_t size() const { return tokens_.size(); }
    [[nodiscard]] const std::string& operator[](size_t idx) const { return tokens_[idx]; }

    auto begin() const { return tokens_.begin(); }
    auto end() const { return tokens_.end(); }

private:
    explicit Path(std::vector<std::string> tokens) : tokens_(std::move(tokens)) {}

    std::vector<std::string> tokens_;
};

template <typename PREDICATE>
void visit(const Defs& defs, const Path& path, PREDICATE& predicate) {

    // a. Visit the 'definitions' (which includes the server state)
    predicate.handle(defs);

    if (path.empty()) {
        return;
    }

    // b. Visit each one of the 'nodes' along the given path

    node_ptr current = nullptr;
    for (auto&& token : path) {
        if (current == nullptr) {
            current = defs.findSuite(token);
        }
        else {
            current = current->find_immediate_child(token);
        }
        if (current == nullptr) {
            predicate.not_found();
            return;
        }

        predicate.handle(*current);
    }
}

} // namespace ecf

#endif // ecflow_base_Algorithms_hpp
