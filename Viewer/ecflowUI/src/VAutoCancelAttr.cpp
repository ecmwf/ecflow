/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VAutoCancelAttr.hpp"

#include "NodeAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

//================================
// VAutoCancelAttrType
//================================

VAutoCancelAttrType::VAutoCancelAttrType() : VAttributeType("autocancel") {
    dataCount_                           = 2;
    searchKeyToData_["autocancel_value"] = ValueIndex;
    scanProc_                            = VAutoCancelAttr::scan;
}

QString VAutoCancelAttrType::toolTip(QStringList d) const {
    QString t = "<b>Type:</b> Autocancel<br>";

    if (d.count() == dataCount_) {
        t += "<b>Value:</b> " + d[ValueIndex];
    }
    return t;
}

QString VAutoCancelAttrType::definition(QStringList d) const {
    QString t = "autocancel";
    if (d.count() == dataCount_) {
        t += " " + d[ValueIndex];
    }
    return t;
}

void VAutoCancelAttrType::encode(ecf::AutoCancelAttr* a, QStringList& data) const {
    if (a) {
        data << qName_;
        QString aStr("autocancel ");
        std::string v = a->toString();
        QString s     = QString::fromStdString(v);
        if (s.startsWith(aStr)) {
            data << s.mid(aStr.size());
        }
    }
}

//=====================================================
//
// VAutoCancelAttr
//
//=====================================================

VAutoCancelAttr::VAutoCancelAttr(VNode* parent) : VAttribute(parent, 0) {
}

VAttributeType* VAutoCancelAttr::type() const {
    static VAttributeType* atype = VAttributeType::find("autocancel");
    return atype;
}

QStringList VAutoCancelAttr::data(bool /*firstLine*/) const {
    static auto* atype = static_cast<VAutoCancelAttrType*>(type());
    QStringList s;
    if (node_ptr node = parent_->node()) {
        ecf::AutoCancelAttr* a = node->get_autocancel();
        atype->encode(a, s);
    }
    return s;
}

void VAutoCancelAttr::scan(VNode* vnode, std::vector<VAttribute*>& vec) {
    if (node_ptr node = vnode->node()) {
        if (ecf::AutoCancelAttr* found = node->get_autocancel(); found) {
            vec.push_back(new VAutoCancelAttr(vnode));
        }
    }
}
