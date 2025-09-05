/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VLateAttr.hpp"

#include "VAttributeType.hpp"
#include "VNode.hpp"
#include "ecflow/attribute/NodeAttr.hpp"

//================================
// VLateAttrType
//================================

VLateAttrType::VLateAttrType() : VAttributeType("late") {
    dataCount_                    = 2;
    searchKeyToData_["late_name"] = NameIndex;
    searchKeyToData_["late_type"] = TypeIndex;
    searchKeyToData_["name"]      = NameIndex;
    scanProc_                     = VLateAttr::scan;
}

QString VLateAttrType::toolTip(QStringList d) const {
    QString t = "<b>Type:</b> Late<br>";
    if (d.count() == dataCount_) {
        t += "<b>Name:</b> " + d[NameIndex];
    }
    return t;
}

QString VLateAttrType::definition(QStringList d) const {
    QString t = "late";
    if (d.count() == dataCount_) {
        t += " " + d[NameIndex];
    }
    return t;
}

void VLateAttrType::encode(ecf::LateAttr* late, QStringList& data) const {
    if (late) {
        data << qName_ << QString::fromStdString(late->name());
    }
}

//=====================================================
//
// VLateAttr
//
//=====================================================

VLateAttr::VLateAttr(VNode* parent, const std::string& /*name*/) : VAttribute(parent, 0) {
}

VAttributeType* VLateAttr::type() const {
    static VAttributeType* atype = VAttributeType::find("late");
    return atype;
}

QStringList VLateAttr::data(bool /*firstLine*/) const {
    static auto* atype = static_cast<VLateAttrType*>(type());
    QStringList s;
    if (node_ptr node = parent_->node()) {
        ecf::LateAttr* late = node->get_late();
        atype->encode(late, s);
    }
    return s;
}

void VLateAttr::scan(VNode* vnode, std::vector<VAttribute*>& vec) {
    if (node_ptr node = vnode->node()) {
        ecf::LateAttr* late = node->get_late();
        if (late) {
            vec.push_back(new VLateAttr(vnode, late->name()));
        }
    }
}
