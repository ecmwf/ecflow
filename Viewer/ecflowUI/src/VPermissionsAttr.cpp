// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "VPermissionsAttr.hpp"

#include "VAttributeType.hpp"
#include "VNode.hpp"
#include "ecflow/attribute/Permissions.hpp"

//
// VPermissionsAttrType
//

VPermissionsAttrType::VPermissionsAttrType()
    : VAttributeType("permissions") {
    dataCount_                            = 4;
    searchKeyToData_["permissions_value"] = ValueIndex;
    searchKeyToData_["permissions_users"] = UsersIndex;
    scanProc_                             = VPermissionsAttr::scan;
}

QString VPermissionsAttrType::toolTip(QStringList d) const {
    QString t = "<b>Type:</b> Permissions<br>";
    if (d.count() == dataCount_) {
        // One line per entry: <user>: <letters>
        const QStringList entries = d.at(ValueIndex).split(",", Qt::SkipEmptyParts);
        bool first                = true;
        for (const QString& entry : entries) {
            const int colon = entry.lastIndexOf(':');
            t += (first ? QString() : QString("<br>")) + "<b>" + entry.left(colon).toHtmlEscaped() + ":</b> " +
                 entry.mid(colon + 1).toHtmlEscaped();
            first = false;
        }
    }
    return t;
}

QString VPermissionsAttrType::definition(QStringList d) const {
    QString t = "permissions";
    if (d.count() == dataCount_) {
        t += " " + d.at(JsonIndex);
    }
    return t;
}

void VPermissionsAttrType::encode(const ecf::Permissions& permissions, QStringList& data) const {
    QStringList users;
    for (const auto& entry : permissions.entries()) {
        users << QString::fromStdString(entry.user);
    }
    data << qName_                                         // TypeIndex
         << QString::fromStdString(permissions.to_cli())   // ValueIndex
         << users.join(",")                                // UsersIndex
         << QString::fromStdString(permissions.to_json()); // JsonIndex
}

//
// VPermissionsAttr
//

VPermissionsAttr::VPermissionsAttr(VNode* parent)
    : VAttribute(parent, 0) {
}

VAttributeType* VPermissionsAttr::type() const {
    static VAttributeType* atype = VAttributeType::find("permissions");
    return atype;
}

QStringList VPermissionsAttr::data(bool /*firstLine*/) const {
    static auto* atype = static_cast<VPermissionsAttrType*>(type());
    QStringList s;
    if (node_ptr node = parent_->node()) {
        // The attribute can be gone during an update, before the attributes of the node are scanned again: no data
        if (const auto* permissions = node->permissions(); permissions) {
            atype->encode(*permissions, s);
        }
    }
    return s;
}

std::string VPermissionsAttr::strName() const {
    return "permissions";
}

void VPermissionsAttr::scan(VNode* vnode, std::vector<VAttribute*>& vec) {
    if (node_ptr node = vnode->node()) {
        if (node->permissions()) {
            vec.push_back(new VPermissionsAttr(vnode));
        }
    }
}
