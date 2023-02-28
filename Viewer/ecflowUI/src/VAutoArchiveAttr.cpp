//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VAutoArchiveAttr.hpp"

#include "NodeAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

//================================
// VAutoArchiveAttrType
//================================

VAutoArchiveAttrType::VAutoArchiveAttrType() : VAttributeType("autoarchive") {
    dataCount_                            = 2;
    searchKeyToData_["autoarchive_value"] = ValueIndex;
    scanProc_                             = VAutoArchiveAttr::scan;
}

QString VAutoArchiveAttrType::toolTip(QStringList d) const {
    QString t = "<b>Type:</b> Autoarchive<br>";

    if (d.count() == dataCount_) {
        t += "<b>Value:</b> " + d[ValueIndex];
    }
    return t;
}

QString VAutoArchiveAttrType::definition(QStringList d) const {
    QString t = "autoarchive";
    if (d.count() == dataCount_) {
        t += " " + d[ValueIndex];
    }
    return t;
}

void VAutoArchiveAttrType::encode(ecf::AutoArchiveAttr* a, QStringList& data) const {
    if (a) {
        data << qName_;
        QString aStr("autoarchive ");
        std::string v = a->toString();
        QString s     = QString::fromStdString(v);
        if (s.startsWith(aStr)) {
            data << s.mid(aStr.size());
        }
    }
}

//=====================================================
//
// VAutoArchiveAttr
//
//=====================================================

VAutoArchiveAttr::VAutoArchiveAttr(VNode* parent) : VAttribute(parent, 0) {
}

VAttributeType* VAutoArchiveAttr::type() const {
    static VAttributeType* atype = VAttributeType::find("autoarchive");
    return atype;
}

QStringList VAutoArchiveAttr::data(bool /*firstLine*/) const {
    static auto* atype = static_cast<VAutoArchiveAttrType*>(type());
    QStringList s;
    if (node_ptr node = parent_->node()) {
        ecf::AutoArchiveAttr* a = node->get_autoarchive();
        atype->encode(a, s);
    }
    return s;
}

void VAutoArchiveAttr::scan(VNode* vnode, std::vector<VAttribute*>& vec) {
    if (node_ptr node = vnode->node()) {
        if (ecf::AutoArchiveAttr* found = node->get_autoarchive(); found) {
            vec.push_back(new VAutoArchiveAttr(vnode));
        }
    }
}
