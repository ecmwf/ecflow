//============================================================================
// Copyright 2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VQueueAttr.hpp"

#include "NodeAttr.hpp"
#include "QueueAttr.hpp"
#include "VAttributeType.hpp"
#include "VNode.hpp"

//================================
// VQueueAttrType
//================================

VQueueAttrType::VQueueAttrType() : VAttributeType("queue") {
    dataCount_                      = 5;
    searchKeyToData_["queue_name"]  = NameIndex;
    searchKeyToData_["queue_value"] = ValueIndex;
    searchKeyToData_["name"]        = NameIndex;
    scanProc_                       = VQueueAttr::scan;
}

QString VQueueAttrType::toolTip(QStringList d) const {
    QString t = "<b>Type:</b> Queue<br>";
    if (d.count() == dataCount_) {
        t += "<b>Name:</b> " + d[NameIndex] + "<br>";
        t += "<b>Value:</b> " + d[ValueIndex] + "<br>";
        t += "<b>List:</b> " + d[AllValuesIndex];
    }
    return t;
}

QString VQueueAttrType::definition(QStringList d) const {
    QString t = "queue";
    if (d.count() == dataCount_) {
        t += " " + d[NameIndex] + " " + d[AllValuesIndex] + " ";
    }
    return t;
}

void VQueueAttrType::encode(const QueueAttr& q, QStringList& data) const {
    data << qName_ << QString::fromStdString(q.name()) << QString::fromStdString(q.value());

    QStringList allvals;
    for (auto v : q.list()) {
        allvals << QString::fromStdString(v);
    }

    int idx = q.index();
    int pos = -1;
    if (allvals.count() > 0) {
        if (idx == 0)
            pos = 0;
        else if (idx > 0 && idx < allvals.count() - 1)
            pos = 1;
        else if (idx == allvals.count() - 1)
            pos = 2;
    }
    data << allvals.join(" ") << QString::number(pos);
}

//=====================================================
//
// VQueueAttr
//
//=====================================================

VQueueAttr::VQueueAttr(VNode* parent, const QueueAttr&, int index) : VAttribute(parent, index) {
    // name_=m.name();
}

VAttributeType* VQueueAttr::type() const {
    static VAttributeType* atype = VAttributeType::find("queue");
    return atype;
}

QStringList VQueueAttr::data(bool /*firstLine*/) const {
    static auto* atype = static_cast<VQueueAttrType*>(type());
    QStringList s;
    if (node_ptr node = parent_->node()) {
        const std::vector<QueueAttr>& v = node->queues();
        atype->encode(v[index_], s);
    }
    return s;
}

std::string VQueueAttr::strName() const {
    if (node_ptr node = parent_->node()) {
        const std::vector<QueueAttr>& v = node->queues();
        return v[index_].name();
    }
    return {};
}

void VQueueAttr::scan(VNode* vnode, std::vector<VAttribute*>& vec) {
    if (vnode->node()) {
        const std::vector<QueueAttr>& v = vnode->node()->queues();
        auto n                          = static_cast<int>(v.size());
        for (int i = 0; i < n; i++) {
            vec.push_back(new VQueueAttr(vnode, v[i], i));
        }
    }
}
