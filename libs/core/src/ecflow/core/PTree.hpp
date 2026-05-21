/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_PTree_HPP
#define ecflow_core_PTree_HPP

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ecf {

///
/// @brief Thrown when JSON content cannot be parsed.
///
class PTreeParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

///
/// @brief Thrown when a given path is absent.
///
class PTreePathError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

///
/// @brief Thrown when a value is not convertible to the expected type.
///
class PTreeValueError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

///
/// @brief Thrown when requesting a change in an invalid state
///        (e.g. adding array elements to a non-array PTree node).
///
class PTreeInvalidStateError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class PTree;
class PTreeConstIterator;

///
/// @brief Parse a JSON file into a property tree.
///
/// @param filename the name of the input file
/// @param out a property tree populated with the contents of the input file
/// @throws PTreeParseError if the file is not available or cannot be parsed as JSON
///
void read_json(const std::string& filename, PTree& out);

///
/// @brief Serialize a property tree to JSON and write it to a file.
///
/// @attention Writing a PTree with repeated keys WILL TRUNCATE the output
///   (see Known Limitation of PTree class).
///
/// @param filename the name of the output file
/// @param in a property tree to serialize
/// @param indent the number of spaces used for indentation
///
void write_json(const std::string& filename, const PTree& in, int indent = 4);

///
/// @brief The PTree holds an in-memory property tree.
///
/// * Design principles
///
///   1) Support repeated keys within a single node.
///      This is required to handle files such as ecflowview_gui.json, with content like:
///        "line": "a",
///        "line": "b",
///        "row" : { ... },
///        "row" : { ... }
///      PTree preserves all entries as an ordered list.  A SAX parser retains duplicate
///      keys, which technically makes the source "non-Standard JSON".
///
///   2) No JSON-parser implementation type is exposed in this header
///
///   3) Implemented as a replacement for boost::property_tree::ptree.
///
/// * Known Limitations
///
///   1) Writing a PTree with repeated keys WILL TRUNCATE the output
///      (JSON objects cannot have duplicate keys).
///
///      This is acceptable because:
///        * Files with repeated keys (e.g. ecflowview_gui.json) are never written back by ecFlow.
///        * User settings written by write_json always/only have unique keys.
///
///   2) Large number values are silently truncated to fit into an int.
///
///      This is acceptable because ecFlow settings files are not expected to contain very large numbers.
///
/// * Node structure
///
///   Each PTree node is a named value:
///     - name  : the key used by this node's parent to identify it
///               (empty string "" for array elements and for the root node)
///     - value : one of
///         - null     : default-constructed, unset
///         - string   : leaf holding a std::string
///         - integer  : leaf holding an int
///         - children : ordered list of child PTree nodes — supports repeated names
///                      Children whose name is "" are treated as array elements;
///                      children with non-empty names are treated as object fields.
///                      Mixed usage is allowed but uncommon.
///
/// * Iteration yields (std::string name, PTree value) pairs in insertion order,
///   for both object-like and array-like nodes ("" for the name when array-like).
///
/// * Path separator: '.'  (e.g. "server.notification.enabled").
///   IMPORTANT: No escaping is performed — keys MUST NOT contain '.'.
///
class PTree {
public:
    using name_t           = std::string;
    using children_t       = std::vector<PTree>;
    using const_iterator_t = PTreeConstIterator;

    enum class Kind { Null, String, Int, Children };

    ///
    /// @brief Default constructor, creates a null / empty node.
    ///
    PTree() = default;

    ///
    /// @brief Create a leaf node holding a string value.
    ///
    /// @param value the string value
    ///
    explicit PTree(std::string value)
        : kind_(Kind::String),
          str_(std::move(value)) {}

    ///
    /// @brief Create a leaf node holding a string value.
    ///
    /// @param value the string value
    ///
    explicit PTree(const char* value)
        : kind_(Kind::String),
          str_(value) {}

    ///
    /// @brief Create a leaf node holding an integer value.
    ///
    /// @param value the string value
    ///
    explicit PTree(int value)
        : kind_(Kind::Int),
          int_(value) {}

    // PTree is Copyable & Movable.
    PTree(const PTree&)            = default;
    PTree& operator=(const PTree&) = default;
    PTree(PTree&&)                 = default;
    PTree& operator=(PTree&&)      = default;

    ///
    /// @brief Verify if this node has no children.
    ///
    /// A leaf carrying a string/int value is considered "empty" because it
    /// has no child nodes.
    ///
    /// @return `true` iff this node has no children.
    ///
    bool empty() const noexcept { return kind_ != Kind::Children ? true : children_.empty(); }

    ///
    /// @brief Verify if this node is a leaf node (string or integer scalar).
    ///
    /// @return `true` iff this node holds a scalar value.
    ///
    bool is_leaf() const noexcept { return kind_ == Kind::String || kind_ == Kind::Int; }

    ///
    /// @brief Verify if this node holds the specified Kind.
    ///
    /// @tparam K the Kind to check for
    /// @return `true` iff this node holds the specified Kind
    ///
    template <PTree::Kind K>
    bool is_a() const noexcept {
        return kind_ == K;
    }

    ///
    /// @brief Verify if this node holds named children (object-like).
    ///
    /// @return `true` iff this node holds named children.
    ///
    bool is_object() const noexcept { return kind_ == Kind::Children && !arr_; }

    ///
    /// @brief Verify if this node holds anonymous children (array-like).
    ///
    /// @return `true` iff this node holds anonymous children.
    ///
    bool is_array() const noexcept { return kind_ == Kind::Children && arr_; }

    ///
    /// @brief Reset to null / empty.  The node's own name is preserved.
    ///
    void clear() noexcept {
        kind_ = Kind::Null;
        str_.clear();
        int_ = 0;
        arr_ = false;
        children_.clear();
        // name_ is intentionally NOT cleared — it is the node's identity
        // and is managed exclusively by the parent.
    }

    ///
    /// @brief Access the scalar value held directly by this node.
    ///
    /// @note Only specialisations for std::string, int, and bool are defined.
    ///
    /// @tparam T the type to convert the value to
    /// @return the scalar value held by this node, converted to T
    /// @throws PTreeValueError if conversion fails
    ///
    template <class T>
    T get_value() const;

    ///
    /// @brief Access the scalar at a dotted path, returning `default_value` if absent or if  type conversion fails.
    /// This is guaranteed never to throw.
    ///
    /// @tparam T the type to convert the value to
    /// @param path the dotted path to look up (e.g. "server.notification.enabled")
    /// @param default_value the value to return if the path is absent or if type conversion fails
    /// @return the scalar value at the dotted path, converted to T,
    ///         or `default_value` if the path is absent or if type conversion fails
    ///
    template <class T>
    T get(std::string_view path, T default_value) const;

    /// Convenience overloads for const char* / string literal defaults.
    std::string get(std::string_view path, const char* dv) const;
    std::string get(std::string_view path, std::string dv) const;

    ///
    /// @brief Access the scalar at a dotted path, throwing PTreePathError if absent.
    ///
    /// @tparam T the type to convert the value to
    /// @param path the dotted path to look up (e.g. "server.notification.enabled")
    /// @return the scalar value at the dotted path, converted to T,
    /// @throws PTreePathError if the path does not exist
    /// @throws PTreeValueError if type conversion fails
    ///
    template <class T>
    T get(std::string_view path) const;

    ///
    /// @brief Access a child of the subtree at the dotted path.
    ///
    /// This effectively copies a subtree, if it exists
    ///
    /// @param path the dotted path to look up (e.g. "server.notification.enabled")
    /// @return std::nullopt if the path does not exist, otherwise a copy of the subtree at the path
    ///
    std::optional<PTree> get_child_optional(std::string_view path) const;

    ///
    /// @brief Verify if child of the subtree at the dotted path exists.
    ///
    /// @param path the dotted path to look up (e.g. "server.notification.enabled")
    /// @return `true` iff the dotted path exists (any node type).
    ///
    bool contains(std::string_view path) const noexcept;

    ///
    /// @brief Find the first direct child whose key equals @p key.
    ///
    /// @param key the key to look up among direct children (not a dotted path)
    /// @return a valid iterator if the key exists, otherwise end().
    ///
    const_iterator_t find(std::string_view key) const;

    ///
    /// @brief Set a scalar string at a dotted path, creating intermediate objects as needed.
    ///        Important: Last-wins if the final key already exists!!!
    ///
    /// @param path the dotted path to set (e.g. "server.notification.enabled")
    /// @param value the scalar string to set
    ///
    void put(std::string_view path, std::string value);

    ///
    /// @brief Set a scalar string at a dotted path, creating intermediate objects as needed.
    ///        Important: Last-wins if the final key already exists!!!
    ///
    /// @param path the dotted path to set (e.g. "server.notification.enabled")
    /// @param value the scalar string to set
    ///
    void put(std::string_view path, const char* value) { put(path, std::string(value)); }

    ///
    /// @brief Set a scalar integer at a dotted path, creating intermediate objects as needed.
    ///        Important: Last-wins if the final key already exists!!!
    ///
    /// @param path the dotted integer to set (e.g. "server.notification.enabled")
    /// @param value the scalar string to set
    ///
    void put(std::string_view path, int value);

    ///
    /// @brief Set a subtree at a dotted path, creating intermediate objects as needed.
    ///
    /// If path is empty, appends as an anonymous array element
    ///   — this is to handle the existing pattern `tree.put_child("", value)` used in readRcFile.
    ///
    /// @param path the dotted integer to set (e.g. "server.notification.enabled")
    /// @param child the subtree
    ///
    void put_child(std::string_view path, PTree child);

    ///
    /// @brief Append a direct subtree with the given key.
    ///
    /// @note Duplicate keys are allowed — all entries are preserved in insertion order.
    ///
    /// @param key the key associated to the child (not a dotted path)
    /// @param child the subtree
    ///
    void add_child(std::string_view key, PTree child);

    ///
    /// @brief Append a child as an anonymous array element (name = "").
    ///
    /// Converts this null node to an array on first call.
    ///
    /// @param child the subtree to append as an array element
    /// @throws PTreeInvalidStateError if the node already holds non-array children.
    ///
    void push_back_array_element(PTree child);

    ///
    /// @brief The begin iterator over direct children in insertion order.
    ///   For object nodes: yields (key, subtree) pairs.
    ///   For array nodes:  yields ("", element) pairs.
    ///   For null / leaf:  empty range (begin() == end()).
    ///
    const_iterator_t begin() const;

    ///
    /// @brief The end iterator over direct children.
    ///
    const_iterator_t end() const;

private:
    name_t name_{}; ///< This node's own name (key in the parent's children list).

    Kind kind_{Kind::Null}; ///< when kind_==Children: true → array ("" names), false → object

    // Object
    std::string str_{};
    int int_{0};
    bool arr_{false};

    // Array
    children_t children_{}; ///< Each child carries its own name_ member.

    ///
    /// @brief Traverse the dotted path and return a pointer to the target node.
    ///
    /// @param path the dotted path to navigate, e.g. "foo.bar.baz". Empty path returns *this.
    /// @returns nullptr if any segment is missing or if a non-object node is encountered mid-path, otherwise the node
    /// at the end of the path.
    ///
    const PTree* navigate(std::string_view path) const noexcept;

    ///
    /// @brief Traverse the dotted path, creating intermediate nodes as needed,
    ///        and return a mutable reference to the target node.
    ///
    /// @param path the dotted path to navigate, e.g. "foo.bar.baz".
    /// @returns the node at the end of the path.
    ///
    PTree& navigate_or_create(std::string_view path);

    ///
    /// @brief Return a mutable reference to the first direct child with the given name,
    ///        creating a new null child if absent.  Converts this node to an object if needed.
    ///
    /// @param key the key associated to the child (not a dotted path)
    /// @return the node associated to the key, or a new null node if the key was not found among direct children
    /// @throws PTreeInvalidStateError if this node is an array (mixing named and anonymous children is not permitted).
    ///
    PTree& get_or_create_child(const std::string& key);

    ///
    /// @brief Copy value fields (kind, str, int, arr, children) from @p src into *this,
    ///        leaving name_ unchanged.
    ///
    /// This is the correct way to set the value of a node that already sits in a parent's
    /// children_ vector, since direct operator= would also overwrite name_.
    ///
    void copy_value_from(PTree src);

    friend class PTreeConstIterator;
};

///
/// @brief A forward const-iterator over the direct children of a PTree.
///
/// Some technicallities:
///
///   1) PTree internally stores a std::vector<PTree> (each child carrying its own name_),
///      which implies that the iterator cannot return a direct reference to a stored
///      std::pair<std::string, PTree>.
///   2) The iterator caches a lightweight view IteratorValue on first dereference, so
///      no deep copy of the child subtree is ever made, and the cache resets on each advance.
///       — this gives the same forward-iterator guarantee as a standard iterator:
///         a dereferenced value is valid until the iterator is incremented or destroyed.
///
class PTreeConstIterator {
public:
    ///
    /// @brief Lightweight non-owning view of a (name, child) pair.
    ///
    /// Both members are references directly into the parent PTree's children vector.
    ///
    struct IteratorValue
    {
        const std::string& first; ///< Reference to the child's name.
        const PTree& second;      ///< Reference to the child PTree itself.

        IteratorValue(const std::string& f, const PTree& s) noexcept
            : first(f),
              second(s) {}
        IteratorValue(const IteratorValue&)            = default;
        IteratorValue& operator=(const IteratorValue&) = delete; // reference members; no rebinding allowed
    };

    using value_type        = IteratorValue;
    using reference         = const IteratorValue&;
    using pointer           = const IteratorValue*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    reference operator*() const {
        materialise();
        return *cache_;
    }
    pointer operator->() const {
        materialise();
        return &*cache_;
    }

    PTreeConstIterator& operator++() {
        ++it_;
        cache_.reset();
        return *this;
    }
    PTreeConstIterator operator++(int) {
        auto t = *this;
        ++(*this);
        return t;
    }

    bool operator==(const PTreeConstIterator& o) const { return it_ == o.it_; }
    bool operator!=(const PTreeConstIterator& o) const { return it_ != o.it_; }

private:
    // inner_t points into a std::vector<PTree>.
    using inner_t = PTree::children_t::const_iterator;

    explicit PTreeConstIterator(inner_t it)
        : it_(std::move(it)) {}

    ///
    /// @brief Build (or reuse) the cached IteratorValue for the current position.
    ///
    /// Stores two references into the children vector, thus avoiding a copy of the subtree.
    ///
    void materialise() const {
        if (!cache_) {
            cache_.emplace(it_->name_, *it_);
        }
    }

    inner_t it_;
    mutable std::optional<value_type> cache_;

    friend class PTree;
};

template <>
inline std::string PTree::get_value<std::string>() const {
    switch (kind_) {
        case Kind::String:
            return str_;
        case Kind::Int:
            return std::to_string(int_);
        case Kind::Null:
            return {};
        case Kind::Children:
            return {};
    }
    return {};
}

template <>
inline int PTree::get_value<int>() const {
    if (kind_ == Kind::Int) {
        return int_;
    }
    if (kind_ == Kind::String) {
        try {
            return std::stoi(str_);
        }
        catch (...) {
        }
    }
    throw PTreeValueError("PTree::get_value<int>: cannot convert to int");
}

template <>
inline bool PTree::get_value<bool>() const {
    if (kind_ == Kind::String) {
        if (str_ == "true" || str_ == "1") {
            return true;
        }
        if (str_ == "false" || str_ == "0") {
            return false;
        }
    }
    throw PTreeValueError("PTree::get_value<bool>: cannot convert to bool");
}

template <class T>
T PTree::get(std::string_view path, T default_value) const {
    const PTree* node = navigate(path);
    if (!node) {
        return default_value;
    }
    try {
        return node->get_value<T>();
    }
    catch (...) {
        return default_value;
    }
}

template <class T>
T PTree::get(std::string_view path) const {
    const PTree* node = navigate(path);
    if (!node) {
        throw PTreePathError("PTree::get: path not found: " + std::string(path));
    }
    return node->get_value<T>();
}

} // namespace ecf

#endif /* ecflow_core_PTree_HPP */
