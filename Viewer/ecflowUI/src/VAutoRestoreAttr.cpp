//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VAutoRestoreAttr.hpp"

#include "NodeAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

//================================
// VAutoRestoreAttrType
//================================

VAutoRestoreAttrType::VAutoRestoreAttrType() : VAttributeType("autorestore") {
    dataCount_                            = 2;
    searchKeyToData_["autorestore_value"] = ValueIndex;
    scanProc_                             = VAutoRestoreAttr::scan;
}

QString VAutoRestoreAttrType::toolTip(QStringList d) const {
    QString t = "<b>Type:</b> Autorestore<br>";

    if (d.count() == dataCount_) {
        t += "<b>Value:</b> " + d[ValueIndex];
    }
    return t;
}

QString VAutoRestoreAttrType::definition(QStringList d) const {
    QString t = "autorestore";
    if (d.count() == dataCount_) {
        t += " " + d[ValueIndex];
    }
    return t;
}

void VAutoRestoreAttrType::encode(ecf::AutoRestoreAttr* a, QStringList& data) const {
    if (a) {
        data << qName_;
        QString aStr("autorestore ");
        std::string v = a->toString();
        QString s     = QString::fromStdString(v);
        if (s.startsWith(aStr)) {
            data << s.mid(aStr.size());
        }
    }
}

//=====================================================
//
// VAutoRestoreAttr
//
//=====================================================

VAutoRestoreAttr::VAutoRestoreAttr(VNode* parent) : VAttribute(parent, 0) {
}

VAttributeType* VAutoRestoreAttr::type() const {
    static VAttributeType* atype = VAttributeType::find("autorestore");
    return atype;
}

QStringList VAutoRestoreAttr::data(bool /*firstLine*/) const {
    static auto* atype = static_cast<VAutoRestoreAttrType*>(type());
    QStringList s;
    if (node_ptr node = parent_->node()) {
        ecf::AutoRestoreAttr* a = node->get_autorestore();
        atype->encode(a, s);
    }
    return s;
}

void VAutoRestoreAttr::scan(VNode* vnode, std::vector<VAttribute*>& vec) {
    if (node_ptr node = vnode->node()) {
        if (ecf::AutoRestoreAttr* a = node->get_autorestore()) {
            vec.push_back(new VAutoRestoreAttr(vnode));
        }
    }
}
