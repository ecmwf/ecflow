/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_AvisoAttr_HPP
#define ecflow_attribute_AvisoAttr_HPP

#include <cstdint>
#include <iostream>
#include <string>

#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"

namespace cereal {
class access;
}

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
    using name_t     = std::string;
    using listener_t = std::string;

    /**
     * Creates a(n invalid) Aviso
     *
     * Note: this is required by Cereal serialization
     *       Cereal invokes the default ctor to create the object and only then proceeds to member-wise serialization.
     */
    AvisoAttr() = default;
    AvisoAttr(std::string name, listener_t handle);
    AvisoAttr(const AvisoAttr& rhs) = default;

    AvisoAttr& operator=(const AvisoAttr& rhs) = default;

    [[nodiscard]] inline const std::string& name() const { return name_; }
    [[nodiscard]] inline const std::string& listener() const { return listener_; }

    void set_listener(std::string_view listener) { listener_ = listener; }

    bool why(std::string& theReasonWhy) const;

    void reset();

    [[nodiscard]] bool isFree() const;

    void requeue();

    template <class Archive>
    friend void serialize(Archive& ar, AvisoAttr& aviso, std::uint32_t version);

private:
    name_t name_;
    listener_t listener_;
};

template <class Archive>
void serialize(Archive& ar, AvisoAttr& aviso, [[maybe_unused]] std::uint32_t version) {
    ar & aviso.name_;
    ar & aviso.listener_;
}

} // namespace ecf

#endif /* ecflow_attribute_AvisoAttr_HPP */
