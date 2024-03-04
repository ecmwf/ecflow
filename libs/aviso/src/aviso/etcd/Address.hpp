/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef aviso_etcd_Address_HPP
#define aviso_etcd_Address_HPP

#include <string>
#include <string_view>

namespace aviso::etcd {

class Address {
public:
    explicit Address(std::string_view scheme_host_port) : scheme_host_port_(scheme_host_port) {}

    bool operator==(const Address& other) const { return scheme_host_port_ == other.scheme_host_port_; }
    bool operator!=(const Address& other) const { return !(*this == other); }

    std::string_view address() const { return scheme_host_port_; }

    friend std::hash<Address>;

private:
    std::string scheme_host_port_;
};

} // namespace aviso::etcd

namespace std {
template <>
struct hash<aviso::etcd::Address>
{
    size_t operator()(const aviso::etcd::Address& addr) const noexcept {
        // We just hash the string representation of the address
        return std::hash<std::string>()(addr.scheme_host_port_);
    }
};
} // namespace std

#endif /* aviso_etcd_Address_HPP */
