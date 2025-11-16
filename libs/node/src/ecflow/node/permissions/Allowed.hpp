/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_permissions_Allowed_HPP
#define ecflow_node_permissions_Allowed_HPP

#include <cstdint>
#include <stdexcept>
#include <string>

namespace ecf {

/**
 * \brief Exception thrown when an invalid permission value is encountered.
 *
 * This exception is used to indicate that a string representation of permissions
 * could not be parsed correctly (e.g. contained invalid characters).
 */
struct InvalidPermissionValue : public std::runtime_error
{
    explicit InvalidPermissionValue(const std::string& msg) : std::runtime_error(msg) {}
};

/**
 * \brief Represents the permissions a user has when performing actions on a Node and its attributes.
 *
 * The type is designed to be used in a bitwise manner, allowing for easily combining and checking of permissions.
 */
enum class Allowed : std::uint8_t {
    NONE    = 0,
    READ    = 1 << 0, // Can perform read commands on nodes and their attributes
    WRITE   = 1 << 1, // Can perform write commands on nodes and their attributes
    EXECUTE = 1 << 2, // Can perform execute commands on nodes (e.g. run a task)
    OWNER   = 1 << 3, // Can to load new suites.
    STICKY  = 1 << 4, // This is a special permission and means the permission is cannot be restricted/removed in lower
                      // hierarchy levels, i.e. if a user has sticky permissions, it can't be overridden
                      // by a new permission defined at a lower level. This is useful for defining
                      // server level permissions that should not be overridden by node level permissions.
};

inline Allowed operator|(Allowed lhs, Allowed rhs) {
    using allowed_t = std::underlying_type<Allowed>::type;

    return Allowed{static_cast<Allowed>(static_cast<allowed_t>(lhs) | static_cast<allowed_t>(rhs))};
}

inline Allowed operator&(Allowed lhs, Allowed rhs) {
    using allowed_t = std::underlying_type<Allowed>::type;

    return Allowed{static_cast<Allowed>(static_cast<allowed_t>(lhs) & static_cast<allowed_t>(rhs))};
}

inline Allowed& operator|=(Allowed& lhs, Allowed rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline Allowed& operator&=(Allowed& lhs, Allowed rhs) {
    lhs = lhs & rhs;
    return lhs;
}

/**
 * \brief Checks if lhs contains all permissions of rhs.
 *
 * @param lhs the permissions to check against
 * @param rhs the permissions to check for
 * @return true if lhs contains all permissions of rhs, false otherwise
 */
inline bool contains(Allowed lhs, Allowed rhs) {
    return (lhs & rhs) == rhs;
}

/**
 * \brief Converts Allowed enum to a string representation.
 *
 * The string representation is a concatenation of permission characters:
 * 'r' for READ, 'w' for WRITE, 'x' for EXECUTE, 'o' for OWNER, and 's' for STICKY.
 *
 * @param allowed the enum value to convert
 * @return the string representation of the allowed permissions (e.g. "rwxos")
 */
std::string allowed_to_string(Allowed allowed);

/**
 * \brief Converts a string representation of permissions to an Allowed enum.
 *
 * The string should contain characters representing permissions:
 * 'r' for READ, 'w' for WRITE, 'x' for EXECUTE, 'o' for OWNER, and 's' for STICKY.
 *  Repeated characters are allowed and considered only once, and spaces are ignored.
 *
 * @param s the string representation of permissions (e.g. "rwxos")
 * @return the Allowed enum value corresponding to the string
 * @throws InvalidPermissionValue if the string cannot be parsed correctly
 */
Allowed allowed_from_string(const std::string& s);

} // namespace ecf

#endif /* ecflow_node_permissions_Allowed_HPP */
