/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/PTree.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace ecf {

namespace detail {

std::vector<std::string> split_path(std::string_view path) {
    std::vector<std::string> parts;
    if (path.empty()) {
        return parts;
    }

    std::string_view sv = path;
    while (!sv.empty()) {
        auto dot = sv.find('.');
        if (dot == std::string_view::npos) {
            parts.emplace_back(sv);
            break;
        }
        parts.emplace_back(sv.substr(0, dot));
        sv.remove_prefix(dot + 1);
    }
    return parts;
}

} // namespace detail

const PTree* PTree::navigate(std::string_view path) const noexcept {
    const PTree* cur = this;
    for (const auto& seg : detail::split_path(path)) {
        if (cur->kind_ != Kind::Children) {
            return nullptr;
        }
        bool found = false;
        for (const auto& child : cur->children_) {
            if (child.name_ == seg) {
                cur   = &child;
                found = true;
                break;
            }
        }
        if (!found) {
            return nullptr;
        }
    }
    return cur;
}

PTree& PTree::navigate_or_create(std::string_view path) {
    PTree* cur = this;
    for (const auto& seg : detail::split_path(path)) {
        cur = &cur->get_or_create_child(seg);
    }
    return *cur;
}

PTree& PTree::get_or_create_child(const std::string& key) {
    if (kind_ != Kind::Children) {
        kind_ = Kind::Children;
        arr_  = false;
        str_.clear();
        int_ = 0;
        children_.clear();
    }
    // Linear search — first match wins (put/navigate semantics).
    for (auto& child : children_) {
        if (child.name_ == key) {
            return child;
        }
    }
    // Not found: append a new null child with the requested name.
    children_.emplace_back();
    children_.back().name_ = key;
    return children_.back();
}

void PTree::copy_value_from(PTree src) {
    kind_     = src.kind_;
    str_      = std::move(src.str_);
    int_      = src.int_;
    arr_      = src.arr_;
    children_ = std::move(src.children_);
}

std::string PTree::get(std::string_view path, const char* dv) const {
    return get<std::string>(path, std::string(dv));
}

std::string PTree::get(std::string_view path, std::string dv) const {
    return get<std::string>(path, std::move(dv));
}

bool PTree::contains(std::string_view path) const noexcept {
    return navigate(path) != nullptr;
}

std::optional<PTree> PTree::get_child_optional(std::string_view path) const {
    const PTree* node = navigate(path);
    if (!node) {
        return std::nullopt;
    }
    return *node;
}

PTree::const_iterator_t PTree::find(std::string_view key) const {
    if (kind_ == Kind::Children) {
        for (auto it = children_.cbegin(); it != children_.cend(); ++it) {
            if (it->name_ == key) {
                return PTreeConstIterator(it);
            }
        }
    }
    return end();
}

static const PTree::children_t& sentinel_vec() {
    // A atable empty vector is used as a sentinel so that begin() == end()
    // for null/leaf nodes without undefined behaviour from cross-container comparisons.
    static const PTree::children_t sv;
    return sv;
}

PTree::const_iterator_t PTree::begin() const {
    return kind_ == Kind::Children ? PTreeConstIterator(children_.cbegin()) : PTreeConstIterator(sentinel_vec().cend());
}

PTree::const_iterator_t PTree::end() const {
    return kind_ == Kind::Children ? PTreeConstIterator(children_.cend()) : PTreeConstIterator(sentinel_vec().cend());
}

void PTree::put(std::string_view path, std::string value) {
    // Using copy_value_from so the target node's name_ is preserved even though we are replacing its value.
    navigate_or_create(path).copy_value_from(PTree(std::move(value)));
}

void PTree::put(std::string_view path, int value) {
    navigate_or_create(path).copy_value_from(PTree(value));
}

void PTree::put_child(std::string_view path, PTree child) {
    if (path.empty()) {
        // When path is empty, we append as an anonymous array element.
        push_back_array_element(std::move(child));
        return;
    }
    navigate_or_create(path).copy_value_from(std::move(child));
}

void PTree::add_child(std::string_view key, PTree child) {
    // Always appends, allowing duplicate names (to maintain boost::property_tree semantics).
    if (kind_ != Kind::Children) {
        kind_ = Kind::Children;
        arr_  = false;
        str_.clear();
        int_ = 0;
        children_.clear();
    }
    child.name_ = std::string(key);
    children_.push_back(std::move(child));
}

void PTree::push_back_array_element(PTree child) {
    if (kind_ == Kind::Null) {
        // Convert null to array on first call.
        kind_ = Kind::Children;
        arr_  = true;
    }
    else if (kind_ != Kind::Children || !arr_) {
        throw PTreeInvalidStateError("PTree::push_back_array_element: "
                                     "node is not an array (it already has named children)");
    }
    child.name_ = ""; // array elements always have an empty name
    children_.push_back(std::move(child));
}

//
// JSON I/O
//
// The JSON-parser/serializer is used exclusively in this translation unit.
//

//
// *** JSON-Parsing ***
//
// The nlohmann's SAX interface delivers one event per token without ever merging duplicate keys.
// This allows building a PTree directly from the event stream, preserving every repeated key in insertion order.
//

class PTreeSaxHandler : public nlohmann::json_sax<nlohmann::json> {
public:
    explicit PTreeSaxHandler(PTree& root)
        : root_(root) {}

    bool null() override {
        deliver(PTree{});
        return true;
    }

    bool boolean(bool val) override {
        deliver(PTree(val ? "true" : "false"));
        return true;
    }

    bool number_integer(number_integer_t val) override {
        deliver(PTree(static_cast<int>(val)));
        return true;
    }

    bool number_unsigned(number_unsigned_t val) override {
        deliver(PTree(static_cast<int>(val)));
        return true;
    }

    bool number_float(number_float_t /*val*/, const string_t& s) override {
        // This stores floats as their string representation to avoid precision loss
        // and to keep get_value<std::string>() well-behaved for callers.
        deliver(PTree(s));
        return true;
    }

    bool string(string_t& val) override {
        deliver(PTree(std::move(val)));
        return true;
    }

    bool binary(binary_t& /*val*/) override {
        deliver(PTree{});
        return true;
    }

    bool start_object(std::size_t /*elements*/) override {
        stack_.push_back({PTree{}, {}, /*is_array=*/false});
        return true;
    }

    bool key(string_t& val) override {
        stack_.back().pending_key = std::move(val);
        return true;
    }

    bool end_object() override {
        finish_frame();
        return true;
    }

    bool start_array(std::size_t /*elements*/) override {
        stack_.push_back({PTree{}, {}, /*is_array=*/true});
        return true;
    }

    bool end_array() override {
        finish_frame();
        return true;
    }

    bool parse_error(std::size_t /*pos*/,
                     const std::string& /*last_token*/,
                     const nlohmann::detail::exception& ex) override {
        throw PTreeParseError(ex.what());
    }

private:
    struct Frame
    {
        PTree tree;
        std::string pending_key;
        bool is_array;
    };

    PTree& root_;
    std::vector<Frame> stack_;

    // Attach a completed value node to the appropriate parent.
    void deliver(PTree val) {
        if (stack_.empty()) {
            // Top-level value (e.g. a file that is just a string or number).
            root_ = std::move(val);
            return;
        }
        auto& top = stack_.back();
        if (top.is_array) {
            top.tree.push_back_array_element(std::move(val));
        }
        else {
            top.tree.add_child(top.pending_key, std::move(val));
            top.pending_key.clear();
        }
    }

    // Pop the completed object/array frame and deliver it to its parent.
    void finish_frame() {
        auto frame = std::move(stack_.back());
        stack_.pop_back();
        deliver(std::move(frame.tree));
    }
};

void read_json(const std::string& filename, PTree& out) {
    std::ifstream fs(filename);
    if (!fs.is_open()) {
        throw PTreeParseError("ecf::read_json: cannot open file: " + filename);
    }
    out = PTree{};
    PTreeSaxHandler handler(out);
    try {
        bool ok = nlohmann::json::sax_parse(fs,
                                            &handler,
                                            nlohmann::json::input_format_t::json,
                                            /*strict=*/true,
                                            /*ignore_comments=*/true);
        if (!ok) {
            throw PTreeParseError("ecf::read_json: parse failed for: " + filename);
        }
    }
    catch (const PTreeParseError&) {
        throw;
    }
    catch (const nlohmann::json::parse_error& e) {
        throw PTreeParseError(std::string("ecf::read_json: parse error in '") + filename + "': " + e.what());
    }
}

//
// *** JSON-Serialising ***
//
// Nodes with repeated names lose all but the last value for each name when
// converted to nlohmann (JSON objects cannot have duplicate keys).
//

static nlohmann::ordered_json ptree_to_json(const PTree& pt) {
    if (pt.is_a<PTree::Kind::String>()) {
        return nlohmann::ordered_json(pt.get_value<std::string>());
    }
    if (pt.is_a<PTree::Kind::Int>()) {
        return nlohmann::ordered_json(pt.get_value<int>());
    }
    if (pt.is_array()) {
        auto arr = nlohmann::ordered_json::array();
        for (const auto& [k, v] : pt) {
            arr.push_back(ptree_to_json(v));
        }
        return arr;
    }
    if (pt.is_object()) {
        auto obj = nlohmann::ordered_json::object();
        for (const auto& [k, v] : pt) {
            obj[k] = ptree_to_json(v); // last-wins for repeated names
        }
        return obj;
    }
    return nlohmann::ordered_json{}; // null
}

void write_json(const std::string& filename, const PTree& in, int indent) {
    std::ofstream fs(filename);
    if (!fs.is_open()) {
        throw PTreeParseError("ecf::write_json: cannot open file: " + filename);
    }
    fs << ptree_to_json(in).dump(indent) << '\n';
    if (!fs) {
        throw PTreeParseError("ecf::write_json: write failed for: " + filename);
    }
}

} // namespace ecf
