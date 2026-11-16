// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <vector>

#include <QStringList>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"

class VNode;

namespace ecf {
class Permissions;
}

///
/// @brief Describes the Permissions attribute in ecFlowUI: its data, tooltip, definition and search keys (R6.9).
///
/// The data of an attribute are its type name, its value in the command-line form (e.g. "alice:rwxs,bob:r"), its
/// users, separated by commas, and its value in the JSON form, as written in definition files.
///
class VPermissionsAttrType : public VAttributeType {
public:
    ///
    /// @brief Registers the type "permissions", its search keys and its scan function.
    ///
    VPermissionsAttrType();

    ///
    /// @brief Returns the tooltip of an attribute: its type, then one line per entry, <user>: <letters>.
    ///
    /// @param[in] d The data of the attribute, as encoded by encode()
    /// @return The tooltip, in HTML, user names escaped; the type alone when the data are incomplete.
    ///
    QString toolTip(QStringList d) const override;

    ///
    /// @brief Returns the definition of an attribute: its line in a definition file.
    ///
    /// @param[in] d The data of the attribute, as encoded by encode()
    /// @return The line, "permissions <JSON form>"; "permissions" alone when the data are incomplete.
    ///
    QString definition(QStringList d) const override;

    ///
    /// @brief Encodes an attribute as the data of its row.
    ///
    /// @param[in] permissions The attribute
    /// @param[out] data The data, appended
    ///
    void encode(const ecf::Permissions& permissions, QStringList& data) const;

private:
    enum DataIndex { TypeIndex = 0, ValueIndex = 1, UsersIndex = 2, JsonIndex = 3 };
};

///
/// @brief Represents the Permissions attribute of a node in ecFlowUI; a node carries at most one.
///
class VPermissionsAttr : public VAttribute {
public:
    ///
    /// @brief Creates the attribute of a node; it reads the attribute of the node whenever its data are requested.
    ///
    /// @param[in] parent The node, which owns the attribute
    ///
    explicit VPermissionsAttr(VNode* parent);

    ///
    /// @brief Returns the type "permissions".
    ///
    /// @return The type, registered by VPermissionsAttrType.
    ///
    VAttributeType* type() const override;

    ///
    /// @brief Returns the data of the attribute, as encoded by VPermissionsAttrType::encode().
    ///
    /// @param[in] firstLine Ignored: the attribute is shown on one line
    /// @return The data; empty when the node no longer has the attribute, until its attributes are scanned again.
    ///
    QStringList data(bool firstLine) const override;

    ///
    /// @brief Returns the name of the attribute, "permissions": a node carries at most one.
    ///
    /// @return The name.
    ///
    std::string strName() const override;

    ///
    /// @brief Adds the attribute of a node, when it has one.
    ///
    /// @param[in] vnode The node
    /// @param[out] vec The attributes of the node, appended
    ///
    static void scan(VNode* vnode, std::vector<VAttribute*>& vec);
};
