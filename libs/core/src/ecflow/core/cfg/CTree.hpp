// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace ecf::cfg {

///
/// @brief Holds a configuration document, as read from a file, independently of the file format.
///
/// Each node holds one of the following:
///
///   - nothing (`null`)
///   - a boolean, an integer or a floating-point number
///   - a text
///   - a sequence of nodes
///   - a mapping of keys to nodes, in document order, with unique keys
///
/// Each node also records its path in the document, as a JSON Pointer (RFC 6901), e.g. `/variables/ECF_PORT`
/// or `/list/0`; the path of the root node is empty.
///
/// The tree holds whatever the document expresses; deciding which nodes are acceptable is left to its users.
///
class CTree {
public:
    using Null     = std::monostate;
    using Sequence = std::vector<CTree>;
    using Mapping  = std::vector<std::pair<std::string, CTree>>;
    using Node     = std::variant<Null, bool, std::int64_t, double, std::string, Sequence, Mapping>;

    ///
    /// @brief Creates a node of the tree.
    ///
    /// @param[in] path The path of the node in the document, as a JSON Pointer
    /// @param[in] node The content of the node
    ///
    CTree(std::string path, Node node);

    ///
    /// @brief Returns the path of the node in the document.
    ///
    /// @return The path, as a JSON Pointer; empty for the root node.
    ///
    [[nodiscard]] const std::string& path() const { return path_; }

    ///
    /// @brief Returns the content of the node.
    ///
    /// @return The content.
    ///
    [[nodiscard]] const Node& node() const { return node_; }

    ///
    /// @brief Finds the node of the given key, in a mapping.
    ///
    /// @param[in] key The key
    /// @return The node of the key, or nullptr when this node is not a mapping or has no such key.
    ///
    [[nodiscard]] const CTree* find(std::string_view key) const;

private:
    std::string path_;
    Node node_;
};

} // namespace ecf::cfg
