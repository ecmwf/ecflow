/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_NodePathAlgorithms_hpp
#define ecflow_node_NodePathAlgorithms_hpp

#include <string>
#include <vector>

#include "ecflow/core/Result.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"

namespace ecf {

///
/// @brief Represents a valid path in the server's node hierarchy.
///
/// A path always represents a valid path, even if the referenced node does not exist.
/// A path with N tokens represents a path from the root to a node.
/// Multiple consecutive separators (i.e. slashes, "/") are treated as a single slash.
///
/// @invariant empty() is true exactly for the root path ("/").
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
        ecf::algorithm::split_at(tokens, path, "/ ");

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
    explicit Path(std::vector<std::string> tokens)
        : tokens_(std::move(tokens)) {}

    std::vector<std::string> tokens_;
};

///
/// @brief Visits the definitions and each node along the given path, in order.
///
/// The predicate's handle(const Defs&) is invoked first, followed by handle(const Node&) for
/// each node along @p path. Traversal stops early, invoking not_found(), when a token does not
/// resolve to an existing node. An empty path visits only the definitions.
///
/// @tparam PREDICATE Visitor exposing handle(const Defs&), handle(const Node&) and not_found().
/// @param[in] defs The definitions tree to traverse.
/// @param[in] path The path from the root; an empty path visits only @p defs.
/// @param[in,out] predicate The visitor invoked for each visited element.
///
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

#endif // ecflow_node_NodePathAlgorithms_hpp
