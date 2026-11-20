// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/node/NodeFwd.hpp"
#include "ecflow/python/PythonBinding.hpp"

///
/// @brief Helpers of the Python API of the Permissions attribute (D6.4, R6.5, R6.8).
///
class PermissionsUtil {
public:
    PermissionsUtil() = delete;

    ///
    /// @brief Returns the value of a Permissions attribute given in any of the forms the Python API accepts.
    ///
    /// The forms are:
    ///
    /// - a Permissions object;
    /// - a string, in the JSON form or the command-line form, read as by ecflow_client --permissions;
    /// - a list (or tuple) of strings, one entry <user>:<letters> each.
    ///
    /// @param[in] value The Python object
    /// @return The value of the attribute.
    /// @throws std::runtime_error when the value is invalid; pybind11::type_error when it has none of the forms.
    ///
    static ecf::Permissions from(const py::handle& value);

    ///
    /// @brief Returns the entry of one user, in the command-line form <user>:<letters>.
    ///
    /// @param[in] entry The entry
    /// @return The entry, e.g. "alice:rwxs".
    ///
    static std::string to_string(const ecf::Permissions::Entry& entry);

    ///
    /// @brief Adds, or replaces, the same attribute on each node of a sequence, all or none.
    ///
    /// @param[in] permissions The attribute added
    /// @param[in] nodes An iterable of nodes
    /// @param[in] replace When false, a node already carrying an attribute rejects the whole operation; when true, its
    ///            attribute is replaced
    /// @throws pybind11::type_error when an item is not a node; std::runtime_error naming the first node already
    ///         carrying an attribute, when @p replace is false. Nothing changes in either case.
    ///
    static void add_to(const ecf::Permissions& permissions, const py::iterable& nodes, bool replace);

    ///
    /// @brief Validates the attributes of definitions offline, as a load into a server with the given server attribute
    ///        would (D2.15, D2.18, D2.20, D4.6).
    ///
    /// Each suite is validated where a load would insert it, into a server holding no suite; the result lists the
    /// errors of every suite at fault.
    ///
    /// @param[in] defs The definitions
    /// @param[in] server The server attribute, which declares the users and their roles at server level
    /// @return The errors, one per line, each naming its suite; empty when the definitions would be accepted.
    ///
    static std::string check(const Defs& defs, const ecf::Permissions& server);
};
