/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_AutoRestoreAttr_HPP
#define ecflow_node_AutoRestoreAttr_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace cereal {
class access;
}

class Node;

namespace ecf {

// Use compiler, destructor
class AutoRestoreAttr {
public:
    AutoRestoreAttr(const AutoRestoreAttr& rhs) : node_(nullptr), nodes_to_restore_(rhs.nodes_to_restore_) {}
    explicit AutoRestoreAttr(const std::vector<std::string>& nodes_to_restore)
        : node_(nullptr),
          nodes_to_restore_(nodes_to_restore) {}
    AutoRestoreAttr() = default;

    // needed by node copy constructor and persistence
    void set_node(Node* n) { node_ = n; }

    bool operator==(const AutoRestoreAttr& rhs) const;
    std::string toString() const;

    void do_autorestore();
    const std::vector<std::string>& nodes_to_restore() const { return nodes_to_restore_; }
    void check(std::string& errorMsg) const; // check auto restore can reference the nodes

public:
    void write(std::string&) const;

private:
    Node* node_{nullptr};                       // Not persisted, constructor will always set this up.
    std::vector<std::string> nodes_to_restore_; // must be suite or family

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version);
};

} // namespace ecf

#endif /* ecflow_node_AutoRestoreAttr_HPP */
