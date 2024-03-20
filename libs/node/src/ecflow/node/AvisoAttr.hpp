/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_AvisoAttr_HPP
#define ecflow_node_AvisoAttr_HPP

#include <cstdint>
#include <iostream>
#include <string>

#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"

namespace cereal {
class access;
}

class Node;

namespace ecf {

///
/// \brief AvisoAttr represents an attribute, attached to a \ref Node.
///
/// The Aviso attribute effectively acts as a trigger for the Node, granting
/// the Node to be (re)queued as soon as a related notification is received.
///
/// \see https://github.com/ecmwf/aviso
///

class AvisoAttr {
public:
    using path_t     = std::string;
    using name_t     = std::string;
    using listener_t = std::string;
    using revision_t = std::uint64_t;

    /**
     * Creates a(n invalid) Aviso
     *
     * Note: this is required by Cereal serialization
     *       Cereal invokes the default ctor to create the object and only then proceeds to member-wise serialization.
     */
    AvisoAttr() = default;
    AvisoAttr(Node* parent, std::string name, listener_t handle, revision_t revision);
    AvisoAttr(const AvisoAttr& rhs) = default;

    AvisoAttr& operator=(const AvisoAttr& rhs) = default;

    [[nodiscard]] inline const std::string& name() const { return name_; }
    [[nodiscard]] inline const std::string& listener() const { return listener_; }
    [[nodiscard]] revision_t revision() const { return revision_; }
    [[nodiscard]] std::string path() const;

    void set_listener(std::string_view listener);
    void set_revision(revision_t revision);

    unsigned int state_change_no() const { return state_change_no_; }

    bool why(std::string& theReasonWhy) const;

    void reset();

    [[nodiscard]] bool isFree() const;

    void start() const;
    void finish() const;

    template <class Archive>
    friend void serialize(Archive& ar, AvisoAttr& aviso, std::uint32_t version);

private:
    Node* parent_{nullptr};
    path_t path_;
    name_t name_;
    listener_t listener_;
    // The following are mutable as they are modified by the const method isFree()
    mutable revision_t revision_;
    mutable unsigned int state_change_no_{0}; // *not* persisted, only used on server side
};

template <class Archive>
void serialize(Archive& ar, AvisoAttr& aviso, [[maybe_unused]] std::uint32_t version) {
    ar & aviso.path_;
    ar & aviso.name_;
    ar & aviso.listener_;
    ar & aviso.revision_;
}

} // namespace ecf

#endif /* ecflow_node_AvisoAttr_HPP */
